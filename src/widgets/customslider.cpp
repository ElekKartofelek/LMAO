#include "customslider.h"
#include "glasspanel.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QHBoxLayout>

// Minimum clamp gap in slider units (1 second * 10 = 10 units)
static const int kMinClampGap = 10;

// Label layout
static const int kLabelGap = 6;
static const int kLabelAbove = 8;

static const QString kLabelTextStyle =
    "color: #ccc; background: transparent; padding: 0;"
    "font-size: 12px; font-family: monospace;";

static const QString kLabelTextHidden =
    "color: transparent; background: transparent; padding: 0;"
    "font-size: 12px; font-family: monospace;";

// --- Construction ---

CustomSlider::CustomSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
    setMouseTracking(true);
}

// --- Groove property setters ---

void CustomSlider::setGrooveHeight(int h) { m_grooveHeight = h; update(); }
void CustomSlider::setGrooveColor(const QColor &c) { m_grooveColor = c; update(); }
void CustomSlider::setFillColor(const QColor &c) { m_fillColor = c; update(); }
void CustomSlider::setHandleSize(int s) { m_handleSize = s; update(); }
void CustomSlider::setHandleColor(const QColor &c) { m_handleColor = c; update(); }
void CustomSlider::setRadius(int r) { m_radius = r; update(); }

// --- Clamp property setters ---

void CustomSlider::setClampsVisible(bool visible) {
    m_clampsVisible = visible;
    if (visible) {
        if (m_clampOut <= m_clampIn)
            m_clampOut = maximum();
        ensureLabels();
        updateLabelText();
        updateLabelPositions();
    } else {
        cancelClampEdit();
        if (m_inPanel) m_inPanel->hide();
        if (m_outPanel) m_outPanel->hide();
    }
    update();
}

void CustomSlider::setClampIn(int value) {
    m_clampIn = qBound(minimum(), value, m_clampOut - kMinClampGap);
    emit clampInChanged(m_clampIn);
    updateLabelText();
    updateLabelPositions();
    update();
}

void CustomSlider::setClampOut(int value) {
    m_clampOut = qBound(m_clampIn + kMinClampGap, value, maximum());
    emit clampOutChanged(m_clampOut);
    updateLabelText();
    updateLabelPositions();
    update();
}

void CustomSlider::setClampColor(const QColor &c) { m_clampColor = c; update(); }
void CustomSlider::setClampLabelColor(const QColor &c) { m_clampLabelColor = c; update(); }
void CustomSlider::setDimColor(const QColor &c) { m_dimColor = c; update(); }

// --- Floating labels (GlassPanel) ---

void CustomSlider::ensureLabels() {
    QWidget *host = window();
    if (!host) return;

    auto makePanel = [&](GlassPanel *&panel, QLabel *&label) {
        if (panel) { panel->show(); panel->raise(); return; }

        panel = new GlassPanel(host);
        panel->setRadius(8);
        panel->setBackgroundColor(QColor(5, 5, 5, 235));
        panel->setHighlightStrength(15);
        panel->setCursor(Qt::IBeamCursor);
        panel->installEventFilter(this);

        label = new QLabel(panel);
        label->setStyleSheet(kLabelTextStyle);
        label->setAlignment(Qt::AlignCenter);

        auto *layout = new QHBoxLayout(panel);
        layout->setContentsMargins(7, 2, 7, 2);
        layout->addWidget(label);

        panel->show();
        panel->raise();
    };

    makePanel(m_inPanel, m_inLabel);
    makePanel(m_outPanel, m_outLabel);
}

void CustomSlider::updateLabelText() {
    if (!m_inLabel || !m_outLabel) return;
    m_inLabel->setText(formatClampTime(m_clampIn / 10.0));
    m_outLabel->setText(formatClampTime(m_clampOut / 10.0));
    m_inPanel->adjustSize();
    m_outPanel->adjustSize();
}

void CustomSlider::updateLabelPositions() {
    if (!m_inPanel || !m_outPanel || !m_clampsVisible) return;

    QWidget *host = window();
    if (!host) return;

    double inPx = valueToPixel(m_clampIn);
    double outPx = valueToPixel(m_clampOut);

    QPoint sliderTopLeft = mapTo(host, QPoint(0, 0));
    int sliderX = sliderTopLeft.x();
    int sliderY = sliderTopLeft.y();

    int inW = m_inPanel->sizeHint().width();
    int outW = m_outPanel->sizeHint().width();
    int labelH = qMax(m_inPanel->sizeHint().height(), m_outPanel->sizeHint().height());

    // Y position: above the controls bar
    int labelY = sliderY - labelH - kLabelAbove;

    // X positions: centered on clamp handles
    int inX = sliderX + static_cast<int>(inPx) - inW / 2;
    int outX = sliderX + static_cast<int>(outPx) - outW / 2;

    // Anti-overlap: push apart if colliding
    int inRight = inX + inW;
    int outLeft = outX;
    if (inRight + kLabelGap > outLeft) {
        int overlap = inRight + kLabelGap - outLeft;
        int halfShift = overlap / 2;
        inX -= halfShift;
        outX += halfShift;
    }

    // Clamp to window bounds
    int hostW = host->width();
    inX = qBound(2, inX, hostW - inW - 2);
    outX = qBound(2, outX, hostW - outW - 2);

    // Ensure they still don't overlap after clamping
    if (inX + inW + kLabelGap > outX) {
        outX = inX + inW + kLabelGap;
        if (outX + outW > hostW - 2)
            outX = hostW - outW - 2;
    }

    m_inPanel->move(inX, labelY);
    m_outPanel->move(outX, labelY);
    m_inPanel->raise();
    m_outPanel->raise();

    // Keep edit field on top and aligned
    if (m_editField) {
        GlassPanel *panel = (m_editTarget == EditTarget::ClampIn) ? m_inPanel : m_outPanel;
        if (panel) {
            QRect panelRect = panel->geometry();
            int editW = panelRect.width() + 16;
            int editX = panelRect.x() + panelRect.width() / 2 - editW / 2;
            editX = qBound(2, editX, host->width() - editW - 2);
            m_editField->setGeometry(editX, panelRect.y(), editW, panelRect.height());
        }
        m_editField->raise();
    }
}

// --- Coordinate helpers ---

double CustomSlider::valueToPixel(int val) const {
    if (maximum() <= minimum()) return 0;
    double ratio = (double)(val - minimum()) / (maximum() - minimum());
    return m_radius + ratio * (width() - m_handleSize);
}

int CustomSlider::pixelToValue(double px) const {
    if (width() <= m_handleSize) return minimum();
    double ratio = (px - m_radius) / (width() - m_handleSize);
    ratio = qBound(0.0, ratio, 1.0);
    return minimum() + static_cast<int>(ratio * (maximum() - minimum()));
}

QRectF CustomSlider::clampHandleRect(int val) const {
    double cx = valueToPixel(val);
    double clampW = 4;
    double clampH = m_grooveHeight + 10;
    double grooveY = (height() - m_grooveHeight) / 2.0;
    double cy = grooveY + m_grooveHeight / 2.0 - clampH / 2.0;
    return QRectF(cx - clampW / 2.0, cy, clampW, clampH);
}

QString CustomSlider::formatClampTime(double seconds) {
    if (seconds < 0) seconds = 0;
    int h = static_cast<int>(seconds) / 3600;
    int m = (static_cast<int>(seconds) % 3600) / 60;
    int s = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 10);

    if (h > 0)
        return QString("%1:%2:%3.%4")
            .arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ms);
    else
        return QString("%1:%2.%3")
            .arg(m).arg(s, 2, 10, QChar('0')).arg(ms);
}

double CustomSlider::parseClampTime(const QString &text, bool *ok) {
    *ok = false;
    static QRegularExpression re(
        R"(^(?:(\d+):)?(\d{1,2}):(\d{2})(?:\.(\d))?$|^(\d+)(?:\.(\d))?$)");
    auto match = re.match(text.trimmed());
    if (!match.hasMatch()) return 0;

    double total = 0;
    if (match.captured(5).isEmpty()) {
        int hours = match.captured(1).isEmpty() ? 0 : match.captured(1).toInt();
        int mins = match.captured(2).toInt();
        int secs = match.captured(3).toInt();
        int tenths = match.captured(4).isEmpty() ? 0 : match.captured(4).toInt();
        total = hours * 3600.0 + mins * 60.0 + secs + tenths * 0.1;
    } else {
        int secs = match.captured(5).toInt();
        int tenths = match.captured(6).isEmpty() ? 0 : match.captured(6).toInt();
        total = secs + tenths * 0.1;
    }
    *ok = true;
    return total;
}

// --- Paint ---

void CustomSlider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int grooveY = (height() - m_grooveHeight) / 2;
    QRectF grooveRect(0, grooveY, width(), m_grooveHeight);

    double ratio = (maximum() > minimum())
        ? (double)(value() - minimum()) / (maximum() - minimum())
        : 0.0;
    double handleCenter = m_radius + ratio * (width() - m_handleSize);
    double fillEnd = handleCenter + m_handleSize / 2.0;

    // Draw groove background
    QPainterPath groovePath;
    groovePath.addRoundedRect(grooveRect, m_radius, m_radius);
    p.fillPath(groovePath, m_grooveColor);

    // Draw fill (progress)
    if (fillEnd > 0.5) {
        p.save();
        p.setClipPath(groovePath);
        QRectF fillRect(0, grooveY, fillEnd, m_grooveHeight);
        p.fillPath([&]() {
            QPainterPath fp;
            fp.addRoundedRect(fillRect, m_radius, m_radius);
            return fp;
        }(), m_fillColor);
        p.restore();
    }

    // Draw playback handle
    double handleX = handleCenter - m_handleSize / 2.0;
    QRectF handleRect(handleX, grooveY, m_handleSize, m_handleSize);
    QPainterPath handlePath;
    handlePath.addRoundedRect(handleRect, m_radius, m_radius);
    p.fillPath(handlePath, m_handleColor);

    // Clamps
    if (!m_clampsVisible)
        return;

    double inPx  = valueToPixel(m_clampIn);
    double outPx = valueToPixel(m_clampOut);

    // Dim regions outside clamps
    p.save();
    p.setClipPath(groovePath);
    if (inPx > 0) {
        QRectF leftDim(0, grooveY, inPx, m_grooveHeight);
        p.fillRect(leftDim, m_dimColor);
    }
    if (outPx < width()) {
        QRectF rightDim(outPx, grooveY, width() - outPx, m_grooveHeight);
        p.fillRect(rightDim, m_dimColor);
    }
    p.restore();

    // Draw clamp handles
    auto drawClamp = [&](int val, bool isHovered) {
        QRectF rect = clampHandleRect(val);
        QColor color = isHovered ? m_clampColor.lighter(130) : m_clampColor;
        QPainterPath path;
        path.addRoundedRect(rect, 2, 2);
        p.fillPath(path, color);
        p.setPen(QPen(QColor(0, 0, 0, 80), 0.5));
        p.drawPath(path);
        p.setPen(Qt::NoPen);
    };

    drawClamp(m_clampIn, m_dragTarget == DragTarget::ClampIn);
    drawClamp(m_clampOut, m_dragTarget == DragTarget::ClampOut);

    // Update floating label positions
    QMetaObject::invokeMethod(this, &CustomSlider::updateLabelPositions, Qt::QueuedConnection);
}

// --- Editable labels ---

void CustomSlider::showClampEditor(EditTarget target) {
    cancelClampEdit();

    m_editTarget = target;
    int val = (target == EditTarget::ClampIn) ? m_clampIn : m_clampOut;
    GlassPanel *panel = (target == EditTarget::ClampIn) ? m_inPanel : m_outPanel;
    QLabel *label = (target == EditTarget::ClampIn) ? m_inLabel : m_outLabel;
    if (!panel || !label) return;

    QWidget *host = window();
    if (!host) return;

    // Accent
    panel->setHighlightStrength(75);
    panel->setHighlightColor(QColor(226, 215, 116, 255));

    // Make label text invisible but keep it for sizing
    label->setStyleSheet(kLabelTextHidden);

    // Create edit field over the panel
    m_editField = new QLineEdit(host);
    m_editField->setStyleSheet(
        "QLineEdit {"
        "  color: #fff; background: transparent;"
        "  border: none; padding: 2px 7px;"
        "  font-size: 12px; font-family: monospace;"
        "  selection-background-color: rgba(226,215,116,0.3);"
        "}"
    );
    m_editField->setAlignment(Qt::AlignCenter);
    m_editField->setText(formatClampTime(val / 10.0));
    m_editField->selectAll();

    // Position over the panel, wider for cursor room
    QRect panelRect = panel->geometry();
    int editW = panelRect.width() + 16;
    int editX = panelRect.x() + panelRect.width() / 2 - editW / 2;
    editX = qBound(2, editX, host->width() - editW - 2);
    m_editField->setGeometry(editX, panelRect.y(), editW, panelRect.height());

    m_editField->show();
    m_editField->raise();
    m_editField->setFocus();

    connect(m_editField, &QLineEdit::returnPressed, this, &CustomSlider::commitClampEdit);
    m_editField->installEventFilter(this);
}

void CustomSlider::commitClampEdit() {
    if (!m_editField) return;

    bool ok = false;
    double seconds = parseClampTime(m_editField->text(), &ok);
    if (ok) {
        int val = static_cast<int>(seconds * 10);
        if (m_editTarget == EditTarget::ClampIn)
            setClampIn(val);
        else if (m_editTarget == EditTarget::ClampOut)
            setClampOut(val);
    }

    cancelClampEdit();
}

void CustomSlider::cancelClampEdit() {
    if (m_editField) {
        m_editField->hide();
        m_editField->deleteLater();
        m_editField = nullptr;
    }
    if (m_editTarget != EditTarget::None) {
        GlassPanel *panel = (m_editTarget == EditTarget::ClampIn) ? m_inPanel : m_outPanel;
        QLabel *label = (m_editTarget == EditTarget::ClampIn) ? m_inLabel : m_outLabel;
        if (panel) {
            panel->setHighlightStrength(15);
            panel->setBackgroundColor(QColor(5, 5, 5, 235));
        }
        if (label) label->setStyleSheet(kLabelTextStyle);
    }
    m_editTarget = EditTarget::None;
}

// --- Event filter ---

bool CustomSlider::eventFilter(QObject *obj, QEvent *event) {
    // Escape in edit field
    if (obj == m_editField && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            cancelClampEdit();
            return true;
        }
    }
    // Focus lost on edit field
    if (obj == m_editField && event->type() == QEvent::FocusOut) {
        commitClampEdit();
        return true;
    }
    // Open editor when panel is clicked
    if (m_clampsVisible && event->type() == QEvent::MouseButtonPress) {
        if (obj == m_inPanel) {
            showClampEditor(EditTarget::ClampIn);
            return true;
        }
        if (obj == m_outPanel) {
            showClampEditor(EditTarget::ClampOut);
            return true;
        }
    }
    return QSlider::eventFilter(obj, event);
}

// --- Mouse interaction ---

void CustomSlider::mousePressEvent(QMouseEvent *event) {
    if (m_clampsVisible && event->button() == Qt::LeftButton) {
        QRectF inRect  = clampHandleRect(m_clampIn).adjusted(-6, -4, 6, 4);
        QRectF outRect = clampHandleRect(m_clampOut).adjusted(-6, -4, 6, 4);
        double mx = event->position().x();

        // If both handles overlap, pick based on which side of the midpoint
        if (inRect.intersects(outRect) || 
            qAbs(valueToPixel(m_clampIn) - valueToPixel(m_clampOut)) < 20) {
            double midPx = (valueToPixel(m_clampIn) + valueToPixel(m_clampOut)) / 2.0;
            
            if (mx <= midPx) { m_dragTarget = DragTarget::ClampIn;
            } else { m_dragTarget = DragTarget::ClampOut; }

            event->accept();
            update();
            return;
        }

        if (inRect.contains(event->position())) {
            cancelClampEdit();
            m_dragTarget = DragTarget::ClampIn;
            event->accept();
            update();
            return;
        }
        if (outRect.contains(event->position())) {
            cancelClampEdit();
            m_dragTarget = DragTarget::ClampOut;
            event->accept();
            update();
            return;
        }
    }

    QSlider::mousePressEvent(event);
}

void CustomSlider::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragTarget != DragTarget::None) {
        int val = pixelToValue(event->position().x());

        if (m_dragTarget == DragTarget::ClampIn) {
            val = qBound(minimum(), val, m_clampOut - kMinClampGap);
            if (val != m_clampIn) {
                m_clampIn = val;
                emit clampInChanged(m_clampIn);
                updateLabelText();
                update();
            }
        } else if (m_dragTarget == DragTarget::ClampOut) {
            val = qBound(m_clampIn + kMinClampGap, val, maximum());
            if (val != m_clampOut) {
                m_clampOut = val;
                emit clampOutChanged(m_clampOut);
                updateLabelText();
                update();
            }
        }
        event->accept();
        return;
    }

    // Update cursor
    if (m_clampsVisible) {
        QRectF inHandle  = clampHandleRect(m_clampIn).adjusted(-6, -4, 6, 4);
        QRectF outHandle = clampHandleRect(m_clampOut).adjusted(-6, -4, 6, 4);
        QPointF pos = event->position();

        if (inHandle.contains(pos) || outHandle.contains(pos))
            setCursor(Qt::SizeHorCursor);
        else
            setCursor(Qt::PointingHandCursor);
    }

    QSlider::mouseMoveEvent(event);
}

void CustomSlider::mouseReleaseEvent(QMouseEvent *event) {
    if (m_dragTarget != DragTarget::None) {
        m_dragTarget = DragTarget::None;
        event->accept();
        update();
        return;
    }
    QSlider::mouseReleaseEvent(event);
}