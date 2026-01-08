#pragma once
#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

class MyLabel;
struct AppCfg;

class MjpegServer : public QTcpServer {
    Q_OBJECT
public:
    explicit MjpegServer(MyLabel* source,
                         AppCfg* cfg,
                         quint16 port = 8080,
                         QObject* parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    MyLabel* m_source = nullptr;
    AppCfg*  m_cfg    = nullptr;
    quint16  m_port   = 8080;

    QByteArray loadStatic(const QByteArray& urlPath, QByteArray* outContentType);
    bool writeFifoLine(const QByteArray& line);
    QByteArray configJson() const;
    QByteArray loadConfigJson();
    void handleClient(QTcpSocket* s);
};
