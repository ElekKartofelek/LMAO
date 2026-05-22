#include "glasspanel.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

GlassPanel::GlassPanel(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void GlassPanel::setRadius(int r) { m_radius = r; update(); }
void GlassPanel::setBackgroundColor(const QColor &c) { m_background = c; update(); }
void GlassPanel::setHighlightStrength(int alpha) { m_highlightAlpha = alpha; update(); }

void GlassPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    double w = width();
    double h = height();
    double rad = m_radius;
    int a = m_highlightAlpha;

    QRectF outer(0.5, 0.5, w - 1.0, h - 1.0);

    // 1. Background fill
    QPainterPath bgPath;
    bgPath.addRoundedRect(outer, rad, rad);
    p.fillPath(bgPath, m_background);

    // 2. Multi-pass border glow (vertical: top and bottom bright)
    p.save();
    p.setClipPath(bgPath);

    int passes = 6;
    for (int i = 0; i < passes; ++i) {
        double inset = i * 1.5;
        double alpha = a * (1.0 - (double)i / passes);
        if (alpha < 1) break;

        QRectF strokeRect = outer.adjusted(inset, inset, -inset, -inset);
        double strokeRad = qMax(1.0, rad - inset);

        // Vertical gradient - bright at top and bottom, transparent in middle
        QLinearGradient glow(0, strokeRect.top(), 0, strokeRect.bottom());
        glow.setColorAt(0.0,  QColor(255, 255, 255, (int)alpha));
        glow.setColorAt(0.25, QColor(255, 255, 255, 0));
        glow.setColorAt(0.75, QColor(255, 255, 255, 0));
        glow.setColorAt(1.0,  QColor(255, 255, 255, (int)(alpha * 0.6)));

        QPen glowPen(QBrush(glow), 1.5);
        p.setPen(glowPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(strokeRect, strokeRad, strokeRad);
    }

    p.restore();

    // 3. Outer border (vertical gradient)
    {
        QLinearGradient borderGrad(0, outer.top(), 0, outer.bottom());
        borderGrad.setColorAt(0.0,  QColor(255, 255, 255, a));
        borderGrad.setColorAt(0.2,  QColor(255, 255, 255, a / 4));
        borderGrad.setColorAt(0.5,  QColor(255, 255, 255, a / 10));
        borderGrad.setColorAt(0.8,  QColor(255, 255, 255, a / 4));
        borderGrad.setColorAt(1.0,  QColor(255, 255, 255, (int)(a * 0.6)));

        QPen borderPen(QBrush(borderGrad), 1.5);
        p.setPen(borderPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(outer, rad, rad);
    }
}