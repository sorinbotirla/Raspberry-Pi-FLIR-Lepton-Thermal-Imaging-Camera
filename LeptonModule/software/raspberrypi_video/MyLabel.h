#ifndef MYLABEL_H
#define MYLABEL_H

#include <QtCore>
#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QPaintEvent>

class MyLabel : public QLabel {
  Q_OBJECT;

  public:
    MyLabel(QWidget *parent = 0);
    ~MyLabel();

    void setLogo(const QString &path, int heightPx = 36, int marginPx = 6);

  public slots:
    void setImage(QImage);

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    QImage m_lastImage;
    QPixmap m_logo;
    int m_logoHeight = 36;
    int m_logoMargin = 6;
};

#endif
