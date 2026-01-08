#include "MjpegServer.h"
#include "MyLabel.h"
#include "Config.h"

#include <QDateTime>
#include <QImage>
#include <QTimer>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QBuffer>
#include <QJsonObject>
#include <QJsonDocument>


QByteArray MjpegServer::configJson() const
{
    if (!m_cfg) return "{}";

    QJsonObject root;
    root["background"] = m_cfg->background;

    QJsonObject usb;
    usb["enabled"]  = m_cfg->usb.enabled;
    usb["device"]   = m_cfg->usb.device;
    usb["width"]    = m_cfg->usb.width;
    usb["height"]   = m_cfg->usb.height;
    usb["fps"]      = m_cfg->usb.fps;
    usb["emboss"]   = m_cfg->usb.emboss;
    usb["offset_x"] = m_cfg->usb.xform.offset_x;
    usb["offset_y"] = m_cfg->usb.xform.offset_y;
    usb["scale"]    = m_cfg->usb.xform.scale;
    usb["opacity"]  = m_cfg->usb.xform.opacity;
    usb["rotate"]   = m_cfg->usb.xform.rotate_deg;
    usb["flip_h"]   = m_cfg->usb.xform.flip_h;
    usb["flip_v"]   = m_cfg->usb.xform.flip_v;
    root["usb_cam"] = usb;

    QJsonObject th;
    th["enabled"]  = m_cfg->thermal.enabled;
    th["smooth"]   = m_cfg->thermal.smooth;
    th["offset_x"] = m_cfg->thermal.xform.offset_x;
    th["offset_y"] = m_cfg->thermal.xform.offset_y;
    th["scale"]    = m_cfg->thermal.xform.scale;
    th["opacity"]  = m_cfg->thermal.xform.opacity;
    th["rotate"]   = m_cfg->thermal.xform.rotate_deg;
    th["flip_h"]   = m_cfg->thermal.xform.flip_h;
    th["flip_v"]   = m_cfg->thermal.xform.flip_v;
    root["thermal"] = th;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray MjpegServer::loadConfigJson()
{
    QString cfgPath = QCoreApplication::applicationDirPath() + "/config.json";
    QFile f(cfgPath);
    if (!f.open(QIODevice::ReadOnly)) {
        return QByteArray("{\"ok\":false}");
    }
    return f.readAll();
}

static QByteArray mimeFor(const QString& fn)
{
    QString f = fn.toLower();
    if (f.endsWith(".html")) return "text/html; charset=utf-8";
    if (f.endsWith(".css"))  return "text/css; charset=utf-8";
    if (f.endsWith(".js"))   return "application/javascript; charset=utf-8";
    if (f.endsWith(".png"))  return "image/png";
    if (f.endsWith(".jpg") || f.endsWith(".jpeg")) return "image/jpeg";
    if (f.endsWith(".svg"))  return "image/svg+xml";
    return "application/octet-stream";
}

QByteArray MjpegServer::loadStatic(const QByteArray& urlPath, QByteArray* outContentType)
{
    // URL -> filesystem under <appdir>/webapp
    QString appDir = QCoreApplication::applicationDirPath();
    QString webRoot = QDir(appDir).filePath("webapp");

    QString p = QString::fromUtf8(urlPath);
    if (p == "/" || p.isEmpty()) p = "/index.html";
    if (p.startsWith("/")) p = p.mid(1);

    // prevent traversal
    p = QDir::cleanPath(p);
    if (p.startsWith("..")) return QByteArray();

    QString full = QDir(webRoot).filePath(p);
    QFileInfo fi(full);
    if (!fi.exists() || !fi.isFile()) return QByteArray();

    QFile f(full);
    if (!f.open(QIODevice::ReadOnly)) return QByteArray();

    if (outContentType) *outContentType = mimeFor(full);
    return f.readAll();
}

bool MjpegServer::writeFifoLine(const QByteArray& line)
{
    QFile f("/tmp/lepton_cmd");
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append)) return false;
    f.write(line);
    if (!line.endsWith("\n")) f.write("\n");
    f.flush();
    return true;
}

static QByteArray httpResponse(const QByteArray& body, const QByteArray& contentType = "text/html; charset=utf-8")
{
    QByteArray h;
    h += "HTTP/1.1 200 OK\r\n";
    h += "Connection: close\r\n";
    h += "Cache-Control: no-cache\r\n";
    h += "Pragma: no-cache\r\n";
    h += "Content-Type: " + contentType + "\r\n";
    h += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    h += "\r\n";
    return h + body;
}

MjpegServer::MjpegServer(MyLabel* source, AppCfg* cfg, quint16 port, QObject* parent)
    : QTcpServer(parent), m_source(source), m_cfg(cfg), m_port(port)
{
    listen(QHostAddress::Any, m_port);
}

void MjpegServer::incomingConnection(qintptr socketDescriptor)
{
    auto* s = new QTcpSocket(this);
    if (!s->setSocketDescriptor(socketDescriptor)) {
        s->deleteLater();
        return;
    }
    handleClient(s);
}

void MjpegServer::handleClient(QTcpSocket* s)
{
    s->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    QObject::connect(s, &QTcpSocket::readyRead, this, [this, s]() {
        QByteArray req = s->readAll();

        // very small parser: first line only
        int eol = req.indexOf("\r\n");
        if (eol < 0) return;
        QByteArray first = req.left(eol);
        // Example: "GET /mjpeg HTTP/1.1"
        QList<QByteArray> parts = first.split(' ');
        if (parts.size() < 2) return;

        QByteArray url = parts[1];    // keep full URL including ?query
        QByteArray path = url;        // route path (no query)

        int qpos = path.indexOf('?');
        if (qpos >= 0)
            path = path.left(qpos);


        // API: /api/config
        if (path.startsWith("/api/config")) {
            QByteArray body = configJson();
            s->write(httpResponse(body, "application/json; charset=utf-8"));
            s->flush();
            s->disconnectFromHost();
            return;
        }


        // API: /api/cmd?line=...
        if (path.startsWith("/api/cmd")) {
            QByteArray line;
            int q = url.indexOf('?');
            if (q >= 0) {
                QByteArray qs = url.mid(q + 1);
                QList<QByteArray> items = qs.split('&');
                for (int i = 0; i < items.size(); i++) {
                    QByteArray kv = items[i];
                    int eq = kv.indexOf('=');
                    if (eq < 0) continue;
                    QByteArray k = kv.left(eq);
                    QByteArray v = kv.mid(eq + 1);
                    if (k == "line") {
                        line = QByteArray::fromPercentEncoding(v);
                        break;
                    }
                }
            }

            bool ok = (!line.trimmed().isEmpty()) && writeFifoLine(line);
            QByteArray body = ok ? "{\"ok\":true}\n" : "{\"ok\":false}\n";
            s->write(httpResponse(body, "application/json; charset=utf-8"));
            s->flush();
            s->disconnectFromHost();
            return;
        }


        if (path.startsWith("/mjpeg")) {
            // Start MJPEG stream
            QByteArray h;
            h += "HTTP/1.1 200 OK\r\n";
            h += "Connection: close\r\n";
            h += "Cache-Control: no-cache\r\n";
            h += "Pragma: no-cache\r\n";
            h += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n";
            h += "\r\n";
            s->write(h);
            s->flush();

            // stream timer
            auto* t = new QTimer(s);
            t->setInterval(100); // 10 fps (adjust later)
            QObject::connect(t, &QTimer::timeout, this, [this, s]() {
                if (!s->isOpen()) return;

                QImage img = m_source ? m_source->getLastComposite() : QImage();
                if (img.isNull()) return;

                QByteArray jpg;
                QBuffer buf(&jpg);
                buf.open(QIODevice::WriteOnly);
                img.convertToFormat(QImage::Format_RGB888).save(&buf, "JPG", 70);

                QByteArray part;
                part += "--frame\r\n";
                part += "Content-Type: image/jpeg\r\n";
                part += "Content-Length: " + QByteArray::number(jpg.size()) + "\r\n";
                part += "\r\n";
                s->write(part);
                s->write(jpg);
                s->write("\r\n");
                s->flush();
            });
            t->start();

            QObject::connect(s, &QTcpSocket::disconnected, t, &QTimer::deleteLater);
            return;
        }

        // 404
        // Static files from webapp
        {
            QByteArray ct;
            QByteArray fileBody = loadStatic(path, &ct);
            if (!fileBody.isEmpty()) {
                s->write(httpResponse(fileBody, ct));
                s->flush();
                s->disconnectFromHost();
                return;
            }
        }

        QByteArray notFound = "Not found";
        QByteArray h;
        h += "HTTP/1.1 404 Not Found\r\n";
        h += "Connection: close\r\n";
        h += "Content-Type: text/plain\r\n";
        h += "Content-Length: " + QByteArray::number(notFound.size()) + "\r\n";
        h += "\r\n";
        s->write(h);
        s->write(notFound);
        s->flush();
        s->disconnectFromHost();
    });

    QObject::connect(s, &QTcpSocket::disconnected, s, &QTcpSocket::deleteLater);
}