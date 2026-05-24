#include "glassbutton.h"

#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>

GlassButton::GlassButton(const QString &text, QWidget *parent)
    : GlassPanel(parent), m_text(text)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setHighlightStrength(m_baseHighlight);
    setTextColor(QColor(Qt::white));
}

GlassButton::GlassButton(QWidget *parent)
    : GlassPanel(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setHighlightStrength(m_baseHighlight);
    setTextColor(QColor(Qt::white));
}

void GlassButton::setText(const QString &text) { m_text = text; update(); }
void GlassButton::setTextColor(const QColor &c) { m_textColor = c; update(); }
void GlassButton::setFontSize(int px) { m_fontSize = px; update(); }
void GlassButton::setBold(bool bold) { m_bold = bold; update(); }
void GlassButton::setIcon(const QIcon &icon) { m_icon = icon; update(); }
void GlassButton::setIconSize(int size) { m_iconSize = size; update(); }

void GlassButton::setHighlightStrength(int alpha) {
    m_baseHighlight = alpha;
    GlassPanel::setHighlightStrength(alpha);
}

void GlassButton::setBackgroundColor(const QColor &color) {
    m_baseBg = color;
    GlassPanel::setBackgroundColor(color);
}

void GlassButton::setBgGlossStrength(double s) {
    m_baseGloss = s;
    GlassPanel::setBgGlossStrength(s);
}

void GlassButton::paintEvent(QPaintEvent *event) {
    GlassPanel::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);

    if (!m_icon.isNull()) {
        QPixmap px = m_icon.pixmap(m_iconSize, m_iconSize);
        int x = (width() - px.width()) / 2;
        int y = (height() - px.height()) / 2;
        p.drawPixmap(x, y, px);
    } else {
        QFont f;
        f.setPixelSize(m_fontSize);
        f.setBold(m_bold);
        p.setFont(f);
        p.setPen(m_textColor);
        p.drawText(rect(), Qt::AlignCenter, m_text);
    }
}

void GlassButton::enterEvent(QEnterEvent *) {
    m_hovered = true;
    if (!m_pressed) {
        GlassPanel::setHighlightStrength(m_baseHighlight + 10);
        GlassPanel::setBgGlossStrength(m_baseGloss * 1.5);
    }
}

void GlassButton::leaveEvent(QEvent *) {
    m_hovered = false;
    m_pressed = false;
    GlassPanel::setHighlightStrength(m_baseHighlight);
    GlassPanel::setBackgroundColor(m_baseBg);
    GlassPanel::setBgGlossStrength(m_baseGloss);
}

void GlassButton::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        GlassPanel::setHighlightStrength(m_baseHighlight / 2);
        GlassPanel::setBgGlossStrength(m_baseGloss * 0.5);
        // Slightly darken background on press
        QColor pressed = m_baseBg;
        pressed.setAlpha(qMin(255, m_baseBg.alpha() + 30));
        GlassPanel::setBackgroundColor(pressed);
        update();
    }
}

void GlassButton::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        GlassPanel::setHighlightStrength(m_hovered ? m_baseHighlight + 20 : m_baseHighlight);
        GlassPanel::setBackgroundColor(m_baseBg);
        GlassPanel::setBgGlossStrength(m_hovered ? m_baseGloss * 1.5 : m_baseGloss);
        update();
        if (rect().contains(event->position().toPoint()))
            emit clicked();
    }
}

QSize GlassButton::sizeHint() const {
    QFont f;
    f.setPixelSize(m_fontSize);
    f.setBold(m_bold);
    QFontMetrics fm(f);
    int w = fm.horizontalAdvance(m_text) + 24;
    int h = fm.height() + 12;
    return QSize(w, h);
}