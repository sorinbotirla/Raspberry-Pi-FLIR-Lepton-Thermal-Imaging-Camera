#ifndef MYLABEL_H
#define MYLABEL_H

#include <QtCore>
#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QPaintEvent>
#include <QMutex>
#include "Config.h"

class MyLabel : public QLabel {
  Q_OBJECT;

  public:
    MyLabel(QWidget *parent = 0);
    ~MyLabel();

    void setLogo(const QString &path, int heightPx = 36, int marginPx = 6);
    void setConfig(const AppCfg& cfg);
    QImage getLastComposite() const;

  public slots:
    void setImage(QImage);
    void setCameraImage(QImage img);

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    QImage m_lastImage;     // thermal sensor
    QImage m_camImage;      // usb camera
    QPixmap m_logo;
    int m_logoHeight = 36;
    int m_logoMargin = 6;
    AppCfg m_cfg;
    mutable QMutex m_compMtx;
    QImage m_lastComposite;
};

#endif
