#pragma once

#include <QSlider>
#include <QColor>
#include <QLabel>
#include <QLineEdit>

class GlassPanel;

class CustomSlider : public QSlider {
    Q_OBJECT

public:
    explicit CustomSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    // Groove properties
    void setGrooveHeight(int h);
    void setGrooveColor(const QColor &color);
    void setFillColor(const QColor &color);
    void setHandleSize(int size);
    void setHandleColor(const QColor &color);
    void setRadius(int r);

    int grooveHeight() const { return m_grooveHeight; }
    int handleSize() const { return m_handleSize; }

    // Clamp properties
    void setClampsVisible(bool visible);
    bool clampsVisible() const { return m_clampsVisible; }

    void setClampIn(int value);
    void setClampOut(int value);
    int clampIn() const { return m_clampIn; }
    int clampOut() const { return m_clampOut; }

    void setClampColor(const QColor &color);
    void setClampLabelColor(const QColor &color);
    void setDimColor(const QColor &color);

signals:
    void clampInChanged(int value);
    void clampOutChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // Groove
    int m_grooveHeight = 8;
    QColor m_grooveColor = QColor(255, 255, 255, 38);
    QColor m_fillColor = QColor(0xda, 0xff, 0x74);
    int m_handleSize = 8;
    QColor m_handleColor = QColor(0xda, 0xff, 0x74);
    int m_radius = 4;

    // Clamps
    bool m_clampsVisible = false;
    int m_clampIn = 0;
    int m_clampOut = 0;
    QColor m_clampColor = QColor(0xe2, 0xd7, 0x74);
    QColor m_clampLabelColor = QColor(0xcc, 0xcc, 0xcc);
    QColor m_dimColor = QColor(0, 0, 0, 140);

    // Clamp interaction
    enum class DragTarget { None, ClampIn, ClampOut };
    DragTarget m_dragTarget = DragTarget::None;

    // Floating labels (GlassPanel + QLabel inside, parented to top-level window)
    GlassPanel *m_inPanel = nullptr;
    GlassPanel *m_outPanel = nullptr;
    QLabel *m_inLabel = nullptr;
    QLabel *m_outLabel = nullptr;

    // Editable field
    QLineEdit *m_editField = nullptr;
    enum class EditTarget { None, ClampIn, ClampOut };
    EditTarget m_editTarget = EditTarget::None;

    void ensureLabels();
    void updateLabelPositions();
    void updateLabelText();
    void showClampEditor(EditTarget target);
    void commitClampEdit();
    void cancelClampEdit();

    // Helpers
    double valueToPixel(int val) const;
    int pixelToValue(double px) const;
    QRectF clampHandleRect(int val) const;
    static QString formatClampTime(double seconds);
    static double parseClampTime(const QString &text, bool *ok);
};