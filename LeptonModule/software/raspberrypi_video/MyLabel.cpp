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
void MyLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    // 1) draw camera background
    if (!m_camImage.isNull()) {
        p.drawImage(rect(), m_camImage);
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

        QImage scaled = a.scaled(size(), Qt::IgnoreAspectRatio, Qt::FastTransformation);
        p.drawImage(rect(), scaled);
    }

    // 3) logo
    if (!m_logo.isNull()) {
        int w = (m_logo.width() * m_logoHeight) / std::max(1, m_logo.height());
        QRect r(m_logoMargin, height() - m_logoMargin - m_logoHeight, w, m_logoHeight);
        p.drawPixmap(r, m_logo);
    }
}

