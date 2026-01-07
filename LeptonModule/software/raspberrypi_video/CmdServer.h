#pragma once
#include <QObject>
#include <QString>

struct AppCfg;

class CmdServer : public QObject {
    Q_OBJECT
public:
    explicit CmdServer(const QString& fifoPath,
                       const QString& cfgPath,
                       AppCfg* cfg,
                       QObject* parent = nullptr);

signals:
    void configChanged();

private slots:
    void onReadyRead();

private:
    bool applyLine(const QString& line);

    QString m_fifoPath;
    QString m_cfgPath;
    AppCfg* m_cfg = nullptr;

    int m_fd = -1;
    class QSocketNotifier* m_notifier = nullptr;
};
