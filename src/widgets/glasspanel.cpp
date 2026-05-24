#include "glasspanel.h"

#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QtMath>

GlassPanel::GlassPanel(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void GlassPanel::setRadius(int r) { m_radius = r; update(); }
void GlassPanel::setHighlightColor(const QColor &c) { m_highlightColor = c; update(); }
void GlassPanel::setBackgroundColor(const QColor &c) { m_background = c; update(); }
void GlassPanel::setHighlightStrength(int alpha) { m_highlightAlpha = alpha; update(); }
void GlassPanel::setBgGlossStrength(double s) { m_bgGlossStrength = s; update(); }

// Build a conical gradient with stops placed at the actual corner angles.
// On a non square rect corners depend on the aspect ratio.
static QConicalGradient makeGlowGradient(const QRectF &rect, int alpha, const QColor &glowColor) {
    QConicalGradient grad(rect.center(), 0);

    double hw = rect.width() / 2.0;
    double hh = rect.height() / 2.0;

    // Calculate actual angles to each corner from center
    // atan2 gives angle in radians, convert to 0-360 range
    auto cornerAngle = [&](double dx, double dy) -> double {
        double deg = qRadiansToDegrees(qAtan2(-dy, dx));  // Qt y-axis is inverted
        if (deg < 0) deg += 360.0;
        return deg;
    };

    // Corner positions relative to center
    double aTR = cornerAngle(hw, -hh);   // top right
    double aTL = cornerAngle(-hw, -hh);  // top left
    double aBL = cornerAngle(-hw, hh);   // bottom left
    double aBR = cornerAngle(hw, hh);    // bottom right

    // Edge midpoints (cardinal directions)
    double aRight  = 0.0;
    double aTop    = 90.0;
    double aLeft   = 180.0;
    double aBottom = 270.0;

    // Brightness values
    double bTL = 1.0;    // top left corner
    double bBR = 0.85;   // bottom right corner
    double bTR = 0.45;   // top right corner
    double bBL = 0.25;   // bottom left corner

    // Edge midpoints: average of the two corners they connect
    double bTop    = (bTL + bTR) / 2.0;
    double bRight  = (bTR + bBR) / 2.0;
    double bBottom = (bBL + bBR) / 2.0;
    double bLeft   = (bTL + bBL) / 2.0;

    // Place stops at actual geometric positions
    auto stop = [&](double angle, double brightness) {
        int a = qMin(255, (int)(alpha * brightness));
        grad.setColorAt(angle / 360.0, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), a));
    };

    stop(aRight, bRight);
    stop(aTR, bTR);
    stop(aTop, bTop);
    stop(aTL, bTL);
    stop(aLeft, bLeft);
    stop(aBL, bBL);
    stop(aBottom, bBottom);
    stop(aBR, bBR);
    // Wrap: set stop at 360° same as 0°
    grad.setColorAt(1.0, QColor(glowColor.red(), glowColor.green(), glowColor.blue(), qMin(255, (int)(alpha * bRight))));

    return grad;
}

void GlassPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    double w = width();
    double h = height();
    double rad = m_radius;
    int a = m_highlightAlpha;

    QRectF outer(0.5, 0.5, w - 1.0, h - 1.0);

    // Some notes on the background: There's fill and 2 gloss layers.
    // I really spitballed here so it might not make much sense but looks ok for now.

    // 1. Background fill (diagonal gradient)
    QPainterPath bgPath;
    bgPath.addRoundedRect(outer, rad, rad);
    QLinearGradient bgGrad(outer.left(), outer.top(), outer.right(), outer.bottom());
    bgGrad.setColorAt(0.0, QColor(m_background.red(), m_background.green(), m_background.blue(),
                                   qMin(255, m_background.alpha() + (int)(10 * m_bgGlossStrength))));
    bgGrad.setColorAt(1.0, m_background);
    p.fillPath(bgPath, bgGrad);

    // 2. Gloss layer
    p.save();
    p.setClipPath(bgPath);
    QLinearGradient g1(outer.left(), outer.top(), outer.right(), outer.bottom());
    g1.setColorAt(0.0, QColor(255, 255, 255, (int)(3 * m_bgGlossStrength)));
    g1.setColorAt(0.5, QColor(255, 255, 255, 0));
    g1.setColorAt(1.0, QColor(255, 255, 255, (int)(2 * m_bgGlossStrength)));
    p.fillPath(bgPath, g1);
    p.restore();

    // 3. Another gloss layer 
    p.save();
    p.setClipPath(bgPath);
    QLinearGradient g2(outer.right(), outer.top(), outer.left(), outer.bottom());
    g2.setColorAt(0.0, QColor(255, 255, 255, (int)(3 * m_bgGlossStrength)));
    g2.setColorAt(0.4, QColor(255, 255, 255, 0));
    g2.setColorAt(0.7, QColor(255, 255, 255, (int)(1 * m_bgGlossStrength)));
    g2.setColorAt(1.0, QColor(255, 255, 255, 0));
    p.fillPath(bgPath, g2);
    p.restore();
    

    // 4. Multipass glow (conical gradient)
    p.save();
    p.setClipPath(bgPath);

    int glowAlpha = qMin(a, 100);  // cap inner glow regardless of highlight strength

    int passes = 6;
    for (int i = 0; i < passes; ++i) {
        double inset = i * 1.5;
        double fade = (1.0 - (double)i / passes);
        fade = fade * fade;
        int passAlpha = (int)(glowAlpha * fade);
        if (passAlpha < 1) break;

        QRectF strokeRect = outer.adjusted(inset, inset, -inset, -inset);
        double strokeRad = qMax(1.0, rad - inset);

        QConicalGradient grad = makeGlowGradient(strokeRect, passAlpha, m_highlightColor);

        QPen glowPen(QBrush(grad), 1.5);
        p.setPen(glowPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(strokeRect, strokeRad, strokeRad);
    }

    p.restore();

    // 5. Outer border (conical gradient)
    {
        int b = qMin(255, a * 2);
        QConicalGradient grad = makeGlowGradient(outer, b, m_highlightColor);

        QPen borderPen(QBrush(grad), 1.5);
        p.setPen(borderPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(outer, rad, rad);
    }
}