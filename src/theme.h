#pragma once

#include <QColor>
#include <QString>

namespace Theme {
    // Accent
    inline const QString accent = "#e2d774";
    inline const QColor _c = QColor(accent);
    inline const int accentR = _c.red(), accentG = _c.green(), accentB = _c.blue();
    inline const QString accentBg = QString("rgba(%1, %2, %3, 0.1)").arg(accentR).arg(accentG).arg(accentB);

    // Text
    inline const QString textPrimary = "#ccc";
    inline const QString textHover   = "#fff";
    inline const QString textDim     = "#555";
    inline const QString textDark    = "#000";

    // Backgrounds
    inline const QString hoverBg = "rgba(255, 255, 255, 0.1)";
    inline const QString barBg   = "rgba(255,255,255,0.15)";

    // Panels
    inline const QColor panelBg    = QColor(5, 5, 5, 235);
    inline const QColor popupBg    = QColor(5, 5, 5, 235);
    inline const int panelRadius   = 15;
    inline const int popupRadius   = 15;
    inline const int highlight     = 15;

    // Button states
    inline const QString btnHoverBg =
        "qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "  stop:0 rgba(255, 255, 255, 0.06),"
        "  stop:0.5 rgba(255, 255, 255, 0.02),"
        "  stop:1 rgba(255, 255, 255, 0.06))";
    inline const QString btnPressBg =
        "qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "  stop:0 rgba(255, 255, 255, 0.075),"
        "  stop:0.5 rgba(255, 255, 255, 0.05),"
        "  stop:1 rgba(255, 255, 255, 0.075))";

    // Popup menu button style
    inline const QString popupBtnStyle = QString(
        "QPushButton {"
        "  color: %1; background: transparent; border: none;"
        "  text-align: left; padding: 6px 12px; border-radius: %6px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { color: %2; background: %3; }"
        "QPushButton:pressed { color: %4; background: %5; }"
    ).arg(textPrimary, textHover, btnHoverBg,
          accent, btnPressBg, QString::number(panelRadius - 4));

    // Layout
    inline const int barHeight   = 44;
    inline const int barMargin   = 24;
    inline const int barMaxWidth = 1200;
    inline const int barSpacing  = 10;
    inline const int popupGap    = 12;
    inline const int btnSize     = 32;

    // Timing
    inline const int notifDuration = 3000;
}