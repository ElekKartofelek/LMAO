#pragma once

#include <QWidget>
#include <QColor>

class GlassPanel : public QWidget {
    Q_OBJECT

public:
    explicit GlassPanel(QWidget *parent = nullptr);

    void setRadius(int r);
    void setBackgroundColor(const QColor &color);
    void setHighlightStrength(int alpha); // 0–255
    void setHighlightColor(const QColor &color);
    void setBgGlossStrength(double strength);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius = 10;
    QColor m_background = QColor(0, 0, 0, 215);
    QColor m_highlightColor = QColor(255, 255, 255);
    int m_highlightAlpha = 25;
    double m_bgGlossStrength = 0.5;
};
