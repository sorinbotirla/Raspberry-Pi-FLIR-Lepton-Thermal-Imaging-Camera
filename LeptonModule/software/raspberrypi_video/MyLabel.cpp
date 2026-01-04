#include "MyLabel.h"
#include <QPainter>

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

void MyLabel::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter p(this);

  if (!m_lastImage.isNull()) {
    p.drawImage(rect(), m_lastImage);
  }

  if (!m_logo.isNull()) {
    QPixmap scaled = m_logo.scaledToHeight(m_logoHeight, Qt::SmoothTransformation);
    int x = m_logoMargin;
    int y = height() - scaled.height() - m_logoMargin;
    p.drawPixmap(x, y, scaled);
  }
}
