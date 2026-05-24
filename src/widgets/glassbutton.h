#pragma once

#include "glasspanel.h"
#include <QColor>
#include <QString>

class GlassButton : public GlassPanel {
    Q_OBJECT

public:
    explicit GlassButton(QWidget *parent = nullptr);
    explicit GlassButton(const QString &text, QWidget *parent = nullptr);

    void setText(const QString &text);
    void setTextColor(const QColor &color);
    void setFontSize(int px);
    void setBold(bool bold);
    void setIcon(const QIcon &icon);
    void setIconSize(int size);

    void setHighlightStrength(int alpha);
    void setBackgroundColor(const QColor &color);
    void setBgGlossStrength(double s);

    QSize sizeHint() const override;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString m_text;
    QColor m_textColor = QColor(0, 0, 0);
    int m_fontSize = 12;
    bool m_bold = true;
    bool m_hovered = false;
    bool m_pressed = false;
    QIcon m_icon;
    int m_iconSize = 16;

    // Saved base values
    QColor m_baseBg;
    int m_baseHighlight = 15;
    double m_baseGloss = 1.0;
};
