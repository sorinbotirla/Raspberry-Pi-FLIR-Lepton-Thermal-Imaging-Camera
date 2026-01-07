#ifndef USBCAMTHREAD_H
#define USBCAMTHREAD_H

#include <QThread>
#include <QImage>
#include <QString>

class UsbCamThread : public QThread
{
    Q_OBJECT
public:
    explicit UsbCamThread(const QString &device = "/dev/video0", QObject *parent = nullptr);
    ~UsbCamThread() override;

    void setSize(int w, int h);
    void setFps(int fps);

signals:
    void updateCamera(QImage);

protected:
    void run() override;

private:
    QString m_dev;
    int m_w = 640;
    int m_h = 480;
    int m_fps = 15;
    bool m_stop = false;
};

#endif
