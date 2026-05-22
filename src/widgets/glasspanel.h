#pragma once

#include <QWidget>
#include <QColor>

class GlassPanel : public QWidget {
    Q_OBJECT

public:
    explicit GlassPanel(QWidget *parent = nullptr);

    void setRadius(int r);
    void setBackgroundColor(const QColor &color);
    void setHighlightStrength(int alpha);   // 0–255

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius = 14;
    QColor m_background = QColor(0, 0, 0, 215);
    int m_highlightAlpha = 25;
};
