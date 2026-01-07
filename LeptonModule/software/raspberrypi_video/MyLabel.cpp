#include "MyLabel.h"
#include <QPainter>
#include <QTransform>

MyLabel::MyLabel(QWidget *parent) : QLabel(parent)
{
}

MyLabel::~MyLabel()
{
}

void MyLabel::setLogo(const QString &path, int heightPx, int marginPx)
{
  m_logo = QPixmap(path);
  m_logoHeight = heightPx;
  m_logoMargin = marginPx;
  update();
}

void MyLabel::setImage(QImage image)
{
  m_lastImage = image;
  update();
}

void MyLabel::setCameraImage(QImage img)
{
    m_camImage = img;
    update();
}

//void MyLabel::paintEvent(QPaintEvent *event)
//{
//  Q_UNUSED(event);
//
//  QPainter p(this);
//
//  if (!m_lastImage.isNull()) {
//    p.drawImage(rect(), m_lastImage);
//  }
//
//  if (!m_logo.isNull()) {
//    QPixmap scaled = m_logo.scaledToHeight(m_logoHeight, Qt::SmoothTransformation);
//    int x = m_logoMargin;
//    int y = height() - scaled.height() - m_logoMargin;
//    p.drawPixmap(x, y, scaled);
//  }
//}
static QImage embossImage(const QImage& src)
{
    QImage g = src.convertToFormat(QImage::Format_Grayscale8);
    QImage out(g.size(), QImage::Format_Grayscale8);
    out.fill(0); // black background

    const int thr = 35; // edge threshold (tune 10..80)

    for (int y = 1; y < g.height() - 1; ++y) {
        const uchar* p0 = g.constScanLine(y - 1);
        const uchar* p1 = g.constScanLine(y);
        const uchar* p2 = g.constScanLine(y + 1);
        uchar* d = out.scanLine(y);

        for (int x = 1; x < g.width() - 1; ++x) {
            // Sobel magnitude (approx)
            int gx = -p0[x-1] + p0[x+1]
                     -2*p1[x-1] + 2*p1[x+1]
                     -p2[x-1] + p2[x+1];

            int gy = -p0[x-1] -2*p0[x] -p0[x+1]
                     +p2[x-1] +2*p2[x] +p2[x+1];

            int mag = (qAbs(gx) + qAbs(gy)) / 8; // scale down
            d[x] = (mag >= thr) ? 255 : 0;       // white edges, black background
        }
    }
    return out;
}



void MyLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    QColor bg = (m_cfg.background.toLower() == "grey") ? QColor(128,128,128) : Qt::black;
    p.fillRect(rect(), bg);

    // 1) draw camera background
    if (!m_camImage.isNull()) {
        p.save();
        p.translate(width() / 2.0 + m_cfg.usb.xform.offset_x,
                    height() / 2.0 + m_cfg.usb.xform.offset_y);
        p.rotate(m_cfg.usb.xform.rotate_deg);
        p.scale(m_cfg.usb.xform.scale, m_cfg.usb.xform.scale);

        QImage cam = m_cfg.usb.emboss ? embossImage(m_camImage) : m_camImage;
        if (m_cfg.usb.xform.flip_h || m_cfg.usb.xform.flip_v)
            cam = cam.mirrored(m_cfg.usb.xform.flip_h, m_cfg.usb.xform.flip_v);

        QRectF target(-width() / 2.0, -height() / 2.0, width(), height());
        p.setOpacity(m_cfg.usb.xform.opacity);
        p.drawImage(target, cam);
        p.restore();
    }

    // 2) draw thermal overlay (black pixels become transparent if BLACK_BACKGROUND was used)
    if (!m_lastImage.isNull()) {
        QImage a = m_lastImage.convertToFormat(QImage::Format_ARGB32);

        // make pure-black transparent (this works only if thermal background is forced to black)
        for (int y = 0; y < a.height(); y++) {
            QRgb *line = reinterpret_cast<QRgb*>(a.scanLine(y));
            for (int x = 0; x < a.width(); x++) {
                QRgb c = line[x];
                if ((qRed(c) | qGreen(c) | qBlue(c)) == 0) {
                    line[x] = qRgba(0, 0, 0, 0);
                } else {
                    line[x] = qRgba(qRed(c), qGreen(c), qBlue(c), 255);
                }
            }
        }

        QImage scaled;

        if (m_cfg.thermal.smooth <= 0) {
            scaled = a.scaled(size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
        } else {
            // smooth 1..10 -> downscale factor ~ 0.85 .. 0.25
            double f = 1.0 - 0.06 * m_cfg.thermal.smooth;
            if (f < 0.25) f = 0.25;

            QSize downSz(qMax(1, int(width() * f)), qMax(1, int(height() * f)));
            QImage down = a.scaled(downSz, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            scaled = down.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        p.save();
        p.translate(width() / 2.0 + m_cfg.thermal.xform.offset_x,
                    height() / 2.0 + m_cfg.thermal.xform.offset_y);
        p.rotate(m_cfg.thermal.xform.rotate_deg);
        p.scale(m_cfg.thermal.xform.scale, m_cfg.thermal.xform.scale);

        QImage th = scaled;
        if (m_cfg.thermal.xform.flip_h || m_cfg.thermal.xform.flip_v)
            th = th.mirrored(m_cfg.thermal.xform.flip_h, m_cfg.thermal.xform.flip_v);

        QRectF target(-width() / 2.0, -height() / 2.0, width(), height());
        p.setOpacity(m_cfg.thermal.xform.opacity);
        p.drawImage(target, th);
        p.restore();
    }

    // 3) logo
    if (!m_logo.isNull()) {
        int w = (m_logo.width() * m_logoHeight) / std::max(1, m_logo.height());
        QRect r(m_logoMargin, height() - m_logoMargin - m_logoHeight, w, m_logoHeight);
        p.drawPixmap(r, m_logo);
    }
}

void MyLabel::setConfig(const AppCfg& cfg)
    {
        m_cfg = cfg;
        update();
    }

