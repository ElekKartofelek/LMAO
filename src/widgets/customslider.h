#pragma once

#include <QSlider>
#include <QColor>

class CustomSlider : public QSlider {
    Q_OBJECT

public:
    explicit CustomSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    // Properties
    void setGrooveHeight(int h);
    void setGrooveColor(const QColor &color);
    void setFillColor(const QColor &color);
    void setHandleSize(int size);
    void setHandleColor(const QColor &color);
    void setRadius(int r); // 0 = square / half of grooveHeight = fully round

    int grooveHeight() const { return m_grooveHeight; }
    int handleSize() const { return m_handleSize; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_grooveHeight = 8;
    QColor m_grooveColor = QColor(255, 255, 255, 38); // rgba(255,255,255,0.15)
    QColor m_fillColor = QColor(0xda, 0xff, 0x74); // #daff74
    int m_handleSize = 8;
    QColor m_handleColor = QColor(0xda, 0xff, 0x74); // #daff74
    int m_radius = 4;
};