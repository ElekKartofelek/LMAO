#include "customslider.h"

#include <QPainter>
#include <QPainterPath>

CustomSlider::CustomSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMouseTracking(true);
}

void CustomSlider::setGrooveHeight(int h) { m_grooveHeight = h; update(); }
void CustomSlider::setGrooveColor(const QColor &c) { m_grooveColor = c; update(); }
void CustomSlider::setFillColor(const QColor &c) { m_fillColor = c; update(); }
void CustomSlider::setHandleSize(int s) { m_handleSize = s; update(); }
void CustomSlider::setHandleColor(const QColor &c) { m_handleColor = c; update(); }
void CustomSlider::setRadius(int r) { m_radius = r; update(); }

void CustomSlider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Calculate groove rect (centered vertically)
    int grooveY = (height() - m_grooveHeight) / 2;
    QRectF grooveRect(0, grooveY, width(), m_grooveHeight);

    // Calculate handle position in pixels
    double ratio = (maximum() > minimum())
        ? (double)(value() - minimum()) / (maximum() - minimum())
        : 0.0;
    double handleCenter = m_radius + ratio * (width() - m_handleSize);
    double fillEnd = handleCenter + m_handleSize / 2.0;

    // Draw unfilled part
    QPainterPath groovePath;
    groovePath.addRoundedRect(grooveRect, m_radius, m_radius);
    p.fillPath(groovePath, m_grooveColor);

    // Draw fill (progress)
    if (fillEnd > 0.5) {
        // Clip the fill to the groove shape so it inherits the rounded corners
        p.save();
        p.setClipPath(groovePath);
        QRectF fillRect(0, grooveY, fillEnd, m_grooveHeight);
        // Draw fill as a rounded rect
        // the left side gets rounded naturally,
        // the right side gets clipped by the groove shape
        p.fillPath([&]() {
            QPainterPath fp;
            fp.addRoundedRect(fillRect, m_radius, m_radius);
            return fp;
        }(), m_fillColor);
        p.restore();
    }

    // Draw handle
    double handleX = handleCenter - m_handleSize / 2.0;
    QRectF handleRect(handleX, grooveY, m_handleSize, m_handleSize);
    QPainterPath handlePath;
    handlePath.addRoundedRect(handleRect, m_radius, m_radius);
    p.fillPath(handlePath, m_handleColor);
}