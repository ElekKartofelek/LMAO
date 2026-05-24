#include "playerwindow.h"
#include "mpvwidget.h"
#include "mpvcontroller.h"

#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDir>
#include <QProxyStyle>
#include <QApplication>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QGraphicsOpacityEffect>

// --- Theme ---

namespace Theme {
    // Accent
    const QString accent = "#e2d774";
    const QColor _c = QColor(accent);
    const int accentR = _c.red(), accentG = _c.green(), accentB = _c.blue();
    const QString accentBg = QString("rgba(%1, %2, %3, 0.1)").arg(accentR).arg(accentG).arg(accentB);

    // Text
    const QString textPrimary = "#ccc";
    const QString textHover   = "#fff";
    const QString textDim     = "#555";
    const QString textDark    = "#000";

    // Backgrounds
    const QString hoverBg = "rgba(255,255,255,0.1)";
    const QString barBg   = "rgba(255,255,255,0.15)";

    // Panels
    const QColor panelBg    = QColor(5, 5, 5, 235);
    const QColor popupBg    = QColor(5, 5, 5, 235);
    const int panelRadius   = 14;
    const int popupRadius   = 14;
    const int highlight     = 15;

    // Button states
    const QString btnHoverBg =
        "qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "  stop:0 rgba(255, 255, 255, 0.125),"
        "  stop:0.5 rgba(255, 255, 255, 0.05),"
        "  stop:1 rgba(255, 255, 255, 0.075))";
    const QString btnPressBg = QString(
        "qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "  stop:0 rgba(%1, %2, %3, 0.25),"
        "  stop:0.5 rgba(%1, %2, %3, 0.08),"
        "  stop:1 rgba(%1, %2, %3, 0.15))")
        .arg(accentR).arg(accentG).arg(accentB);

    // Layout
    const int barHeight   = 44;
    const int barMargin   = 24;
    const int barMaxWidth = 900;
    const int barSpacing  = 10;
    const int popupGap    = 12;
    const int btnSize     = 32;

    // Timing
    const int notifDuration = 3000;
}

// --- Static helpers ---

// Force white media icons (system icons are black on Windows)
static QIcon whiteIcon(QStyle *style, QStyle::StandardPixmap sp) {
    QPixmap px = style->standardIcon(sp).pixmap(20, 20);
    QPainter p(&px);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(px.rect(), Qt::white);
    p.end();
    return QIcon(px);
}

// Menu button with right-aligned shortcut label
static QPushButton* makeMenuButton(const QString &text, const QString &shortcut, QWidget *parent) {
    auto *btn = new QPushButton(parent);
    auto *lay = new QHBoxLayout(btn);
    lay->setContentsMargins(12, 6, 12, 6);
    lay->setSpacing(16);

    auto *label = new QLabel(text, btn);
    label->setStyleSheet("background: transparent; padding: 0;");

    auto *key = new QLabel(shortcut, btn);
    key->setStyleSheet(QString("color: %1; background: transparent; padding: 0; font-weight: bold;").arg(Theme::textDim));

    lay->addWidget(label);
    lay->addStretch();
    lay->addWidget(key);

    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setMinimumHeight(28);
    btn->setMinimumWidth(280);
    return btn;
}

// Update first QLabel child text in a makeMenuButton
static void setMenuButtonText(QPushButton *btn, const QString &text) {
    auto labels = btn->findChildren<QLabel *>();
    if (!labels.isEmpty()) labels.first()->setText(text);
}

// Menu separator line
static QFrame* makeSeparator(QWidget *parent) {
    auto *sep = new QFrame(parent);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: rgba(255,255,255,0.08); max-height: 1px; border: none; margin: 4px 8px;");
    return sep;
}

// Shared popup stylesheet
static QString popupStyleSheet() {
    return QString(
        "QPushButton {"
        "  color: %1; background: transparent; border: none;"
        "  text-align: left; padding: 6px 12px; border-radius: 10px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { color: %2; background: %3; }"
        "QPushButton:pressed { color: %4; background: %5; }"
    ).arg(Theme::textPrimary, Theme::textHover, Theme::btnHoverBg,
          Theme::accent, Theme::btnPressBg);
}

// Style override: make QSlider jump to click position
class SliderDirectJumpStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return Qt::LeftButton;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

// --- Construction ---

PlayerWindow::PlayerWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("LMAO");
    resize(1280, 720);
    setAcceptDrops(true);
    setMinimumSize(480, 320);
    setMouseTracking(true);

    mpvWidget = new MpvWidget(this);
    controller = new MpvController(mpvWidget, this);

    buildLayout();
    connectSignals();

    // Notification
    notifPanel = new GlassPanel(this);
    notifPanel->setRadius(10);
    notifPanel->setBackgroundColor(Theme::popupBg);
    notifPanel->setHighlightStrength(Theme::highlight);

    notifLabel = new QLabel(notifPanel);
    notifLabel->setAlignment(Qt::AlignCenter);
    notifLabel->setStyleSheet(QString(
        "color: %1; font-size: 13px; font-family: monospace;"
        "padding: 0; background: transparent;"
    ).arg(Theme::textHover));

    auto *notifLayout = new QHBoxLayout(notifPanel);
    notifLayout->setContentsMargins(16, 8, 16, 8);
    notifLayout->addWidget(notifLabel);
    notifPanel->hide();

    notifTimer = new QTimer(this);
    notifTimer->setSingleShot(true);
    notifTimer->setInterval(Theme::notifDuration);
    connect(notifTimer, &QTimer::timeout, notifPanel, &QWidget::hide);
}

// --- Layout ---

void PlayerWindow::buildLayout() {
    // Video fills the entire central area
    mpvWidget->setParent(this);
    setCentralWidget(mpvWidget);
    mpvWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mpvWidget->setMouseTracking(true);
    mpvWidget->installEventFilter(this);

    // Controls bar
    buildControlsBar();
    controlsBar->setParent(this);
    controlsBar->raise();
    controlsBar->installEventFilter(this);
    controlsBar->setMouseTracking(true);

    // Auto-hide timer
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(2500);
    connect(hideTimer, &QTimer::timeout, this, &PlayerWindow::hideControls);
}

void PlayerWindow::buildControlsBar() {
    controlsBar = new GlassPanel(this);
    controlsBar->setObjectName("controlsBar");
    controlsBar->setFixedHeight(Theme::barHeight);
    controlsBar->setRadius(Theme::panelRadius);
    controlsBar->setBackgroundColor(Theme::panelBg);
    controlsBar->setHighlightStrength(Theme::highlight);

    controlsBar->setStyleSheet(QString(
        "QPushButton {"
        "  color: %1; background: transparent; border: none;"
        "  font-size: 15px; padding: 6px; border-radius: 10px;"
        "}"
        "QPushButton:hover { color: %2; background: %3; }"
        "QPushButton:pressed { color: %4; background: %5; }"
        "QLabel { color: %6; font-size: 11px; font-family: monospace; padding: 0 8px; }"
        "QSlider::groove:horizontal { height: 8px; background: %7; border-radius: 4px; }"
        "QSlider::handle:horizontal { width: 8px; height: 8px; margin: 0; background: %8; border-radius: 4px; }"
        "QSlider::sub-page:horizontal { background: %8; border-radius: 4px; margin-left: 0px; }"
    ).arg(Theme::textPrimary, Theme::textHover, Theme::btnHoverBg,
          Theme::accent, Theme::btnPressBg,
          Theme::textDim, Theme::barBg, Theme::accent));

    auto *layout = new QHBoxLayout(controlsBar);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(Theme::barSpacing);

    // Play button
    playBtn = new QPushButton(controlsBar);
    playBtn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playBtn->setFixedSize(Theme::btnSize, Theme::btnSize);
    playBtn->setCursor(Qt::PointingHandCursor);
    playBtn->setFocusPolicy(Qt::NoFocus);
    playBtn->setToolTip("Play/Pause (Space)");
    layout->addWidget(playBtn);

    // Seek bar
    seekBar = new CustomSlider(Qt::Horizontal, controlsBar);
    seekBar->setMinimum(0);
    seekBar->setMaximum(0);
    seekBar->setCursor(Qt::PointingHandCursor);
    seekBar->setStyle(new SliderDirectJumpStyle(seekBar->style()));
    seekBar->setFillColor(QColor(Theme::accent));
    seekBar->setHandleColor(QColor(Theme::accent));
    seekBar->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(seekBar, 1);

    // Time label
    timeLabel = new QLabel("0:00 / 0:00", controlsBar);
    timeLabel->setStyleSheet(QString(
        "color: %1; font-weight: bold; font-size: 13px; padding-left: 8px; padding-right: 0px; margin: 0;"
    ).arg(Theme::textPrimary));
    layout->addWidget(timeLabel);

    // Volume
    auto *volLayout = new QHBoxLayout();
    volLayout->setSpacing(8);

    muteBtn = new QPushButton("🔊", controlsBar);
    muteBtn->setFixedSize(Theme::btnSize, Theme::btnSize);
    muteBtn->setCursor(Qt::PointingHandCursor);
    muteBtn->setFocusPolicy(Qt::NoFocus);
    muteBtn->setToolTip("Mute (M)");

    volumeSlider = new CustomSlider(Qt::Horizontal, controlsBar);
    volumeSlider->setMinimum(0);
    volumeSlider->setMaximum(100);
    volumeSlider->setValue(100);
    volumeSlider->setFixedWidth(80);
    volumeSlider->setCursor(Qt::PointingHandCursor);
    volumeSlider->setStyle(new SliderDirectJumpStyle(volumeSlider->style()));
    volumeSlider->setFillColor(QColor(Theme::accent));
    volumeSlider->setHandleColor(QColor(Theme::accent));
    volumeSlider->setFocusPolicy(Qt::NoFocus);

    volLayout->addWidget(muteBtn, 0, Qt::AlignVCenter);
    volLayout->addWidget(volumeSlider, 0, Qt::AlignVCenter);
    layout->addLayout(volLayout);

    // Audio track button (disabled by default until multi-track file loads)
    audioTrackBtn = new QPushButton("🎤", controlsBar);
    audioTrackBtn->setFixedSize(Theme::btnSize, Theme::btnSize);
    audioTrackBtn->setCursor(Qt::PointingHandCursor);
    audioTrackBtn->setToolTip("Audio track");
    audioTrackBtn->setFocusPolicy(Qt::NoFocus);
    auto *effect = new QGraphicsOpacityEffect(audioTrackBtn);
    effect->setOpacity(0.3);
    audioTrackBtn->setGraphicsEffect(effect);
    layout->addWidget(audioTrackBtn);

    // Settings button
    settingsBtn = new QPushButton("⚙️", controlsBar);
    settingsBtn->setFixedSize(Theme::btnSize, Theme::btnSize);
    settingsBtn->setCursor(Qt::PointingHandCursor);
    settingsBtn->setFocusPolicy(Qt::NoFocus);
    settingsBtn->setToolTip("Options (O)");
    layout->addWidget(settingsBtn);
}

// --- Signals ---

void PlayerWindow::connectSignals() {
    // Duration
    connect(controller, &MpvController::durationChanged, this, [this](double d) {
        cachedDuration = d;
        seekBar->setMaximum(static_cast<int>(d * 10));
    });

    // Position
    connect(controller, &MpvController::positionChanged, this, [this](double pos) {
        if (!seeking)
            seekBar->setValue(static_cast<int>(pos * 10));
        timeLabel->setText(formatTime(pos, cachedDuration) + " / " + formatTime(cachedDuration, cachedDuration));
    });

    // Pause state
    connect(controller, &MpvController::pauseChanged, this, [this](bool paused) {
        playBtn->setIcon(whiteIcon(style(), paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
        if (controller->hasMedia() && !frameStepping)
            showNotification(paused ? "Paused" : "Playing");
    });

    // File loaded
    connect(controller, &MpvController::fileLoaded, this, [this](const QString &filename) {
        setWindowTitle(filename + " - LMAO");
        audioTrackBtn->setStyleSheet("");

        // Delayed check for audio tracks (mpv needs time to parse metadata)
        QTimer::singleShot(300, this, [this]() {
            bool multi = controller->hasMultipleAudioTracks();
            audioTrackBtn->setEnabled(multi);
            if (!multi) {
                auto *eff = new QGraphicsOpacityEffect(audioTrackBtn);
                eff->setOpacity(0.3);
                audioTrackBtn->setGraphicsEffect(eff);
            } else {
                audioTrackBtn->setGraphicsEffect(nullptr);
            }
            updateInfoOverlay();
        });

        // Close audio popup on new file
        if (audioPopup) {
            audioPopup->close();
            audioPopup->deleteLater();
            audioPopup = nullptr;
        }
    });

    // Audio track changed
    connect(controller, &MpvController::audioTrackChanged, this, [this](int64_t) {
        updateInfoOverlay();
    });

    // Mute toggle
    connect(muteBtn, &QPushButton::clicked, this, [this]() {
        if (controller->volume() > 0) {
            lastVolume = controller->volume();
            controller->setVolume(0);
        } else {
            controller->setVolume(lastVolume > 0 ? lastVolume : 100);
        }
    });

    // Volume display
    connect(controller, &MpvController::volumeChanged, this, [this](int vol) {
        if (seeking) return;

        volumeSlider->setValue(vol);
        muteBtn->setText(vol == 0 ? "🔇" : "🔊");
        showNotification(QString("Volume: %1%").arg(vol));
    });

    // Play button
    connect(playBtn, &QPushButton::clicked, controller, &MpvController::togglePause);

    // Audio track button
    connect(audioTrackBtn, &QPushButton::clicked, this, &PlayerWindow::toggleAudioPopup);

    // Volume slider
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        controller->setVolume(value);
    });

    // Seek bar
    connect(seekBar, &QSlider::sliderPressed, this, [this]() { 
        seeking = true;

		volumeBeforeSeek = controller->volume();
        controller->setVolume(0);

        pauseStateBeforeSeek = controller->isPaused();
        controller->pause();
    });

    connect(seekBar, &QSlider::sliderReleased, this, [this]() {
        controller->setVolume(volumeBeforeSeek);

        if (!pauseStateBeforeSeek) {
            controller->play();
        }

        controller->seekTo(seekBar->value() / 10.0);
        seeking = false;
    });
    connect(seekBar, &QSlider::sliderMoved, this, [this](int value) {
        timeLabel->setText(formatTime(value / 10.0, cachedDuration) + " / " + formatTime(cachedDuration, cachedDuration));
        controller->seekTo(value / 10.0);
    });
    connect(seekBar, &QSlider::valueChanged, this, [this](int value) {
        if (!seeking && seekBar->isSliderDown()) {
            seeking = true;
            controller->seekTo(value / 10.0);
            seeking = false;
        }
    });

    // Settings button
    connect(settingsBtn, &QPushButton::clicked, this, &PlayerWindow::toggleOptionsPopup);
}

// --- Public slots ---

void PlayerWindow::openFile(const QString &path) {
    controller->openFile(path);
}

void PlayerWindow::raiseWindow() {
    show();
    QMainWindow::raise();
    activateWindow();
}

// --- Controls visibility ---

void PlayerWindow::showControls() {
    if (!controlsVisible) {
        controlsBar->show();
        controlsVisible = true;
        setCursor(Qt::ArrowCursor);
    }
}

void PlayerWindow::hideControls() {
    if (seeking || audioPopup || optionsPopup || !controller->isVideoFile())
        return;

    QPoint mousePos = controlsBar->mapFromGlobal(QCursor::pos());
    if (controlsBar->rect().contains(mousePos))
        return;

    controlsBar->hide();
    controlsVisible = false;
    setCursor(Qt::BlankCursor);

    if (audioPopup) {
        audioPopup->close();
        audioPopup->deleteLater();
        audioPopup = nullptr;
    }
}

void PlayerWindow::toggleFullscreen() {
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}

// --- File dialog ---

void PlayerWindow::promptOpenFile() {
    static const QString mediaFormats =
        "*.mp4 *.mkv *.avi *.webm *.mov *.flv *.wmv *.ts *.m4v *.m2ts *.mts "
        "*.mpg *.mpeg *.vob *.3gp *.3g2 *.ogv *.asf *.f4v "
        "*.mp3 *.flac *.ogg *.opus *.wav *.aac *.m4a *.wma "
        "*.aiff *.ape *.mka *.wv *.ac3 *.dts *.spx *.mpc";

#ifdef __linux__
    // On KDE use kdialog to avoid KFilePlacesModel/libimobiledevice crash
    // caused by Apple device connected through USB
    QString desktop = qgetenv("XDG_CURRENT_DESKTOP").toLower();
    if (desktop.contains("kde") || desktop.contains("plasma")) {
        auto *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
                if (exitCode == 0) {
                    QString file = proc->readAllStandardOutput().trimmed();
                    if (!file.isEmpty() && QFile::exists(file))
                        controller->openFile(file);
                }
                proc->deleteLater();
            });
        proc->start("kdialog", {
            "--getopenfilename", QDir::homePath(),
            QString("Media Files (%1)\nAll Files (*)").arg(mediaFormats)
        });
        return;
    }
#endif

    QString file = QFileDialog::getOpenFileName(this, "Open Media",
        QDir::homePath(),
        QString("Media files (%1);;All files (*)").arg(mediaFormats));
    if (!file.isEmpty())
        controller->openFile(file);
}

// --- Notification ---

void PlayerWindow::showNotification(const QString &text) {
    notifLabel->setText(text);
    notifPanel->adjustSize();
    int notifY = height() - Theme::barMargin - Theme::barHeight - Theme::popupGap - notifPanel->height();
    notifPanel->move((width() - notifPanel->width()) / 2, notifY);
    notifPanel->raise();
    notifPanel->show();
    notifTimer->start();
}

// --- Playback speed ---

void PlayerWindow::setPlaybackSpeed(double speed) {
    currentSpeed = speed;
    controller->setSpeed(speed);
    showNotification(QString("Speed: %1x").arg(speed, 0, 'f', 2));
}

// --- Audio track popup ---

void PlayerWindow::toggleAudioPopup() {
    if (!audioTrackBtn->isEnabled())
        return;

    // Toggle off
    if (audioPopup) {
        audioPopup->close();
        audioPopup->deleteLater();
        audioPopup = nullptr;
        audioTrackBtn->setStyleSheet("");
        return;
    }

    // Close other popups
    if (optionsPopup) {
        optionsPopup->close();
        delete optionsPopup;
        optionsPopup = nullptr;
        settingsBtn->setStyleSheet("");
    }

    auto tracks = controller->audioTracks();
    if (tracks.isEmpty())
        return;

    int64_t currentId = controller->currentAudioTrack();

    auto *glass = new GlassPanel(this);
    glass->setRadius(Theme::popupRadius);
    glass->setBackgroundColor(Theme::popupBg);
    glass->setHighlightStrength(Theme::highlight);
    glass->setStyleSheet(QString(
        "QPushButton {"
        "  color: %1; background: transparent; border: none;"
        "  text-align: left; padding: 6px 12px; border-radius: 8px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:pressed { color: %3; background: %4; }"
    ).arg(Theme::textPrimary, Theme::btnHoverBg, Theme::accent, Theme::btnPressBg));

    audioPopup = glass;
    audioTrackBtn->setStyleSheet(QString(
        "color: %1; background: %2; border-radius: 10px; font-weight: bold;"
    ).arg(Theme::accent, Theme::btnPressBg));

    auto *popupLayout = new QVBoxLayout(glass);
    popupLayout->setContentsMargins(8, 8, 8, 8);
    popupLayout->setSpacing(2);

    for (const auto &track : tracks) {
        QString label = QString("Track %1").arg(track.id);
        if (!track.lang.isEmpty())  label += " - " + track.lang;
        if (!track.title.isEmpty()) label += " (" + track.title + ")";
        else if (!track.codec.isEmpty()) label += " [" + track.codec + "]";

        auto *btn = new QPushButton(label, glass);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("trackId", (qlonglong)track.id);

        if (track.id == currentId)
            btn->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::accent));

        connect(btn, &QPushButton::clicked, this, [this, id = track.id]() {
            controller->setAudioTrack(id);
            if (audioPopup) {
                for (auto *b : audioPopup->findChildren<QPushButton *>()) {
                    b->setStyleSheet(b->property("trackId").toLongLong() == id
                        ? QString("color: %1; font-weight: bold;").arg(Theme::accent) : "");
                }
            }
        });
        popupLayout->addWidget(btn);
    }

    audioPopup->adjustSize();

    // Position above controls bar, right-aligned
    int popupX = controlsBar->x() + controlsBar->width() - audioPopup->width();
    int popupY = controlsBar->y() - audioPopup->height() - Theme::popupGap;
    audioPopup->move(popupX, popupY);
    audioPopup->raise();
    audioPopup->show();

    connect(audioPopup, &QWidget::destroyed, this, [this]() { audioPopup = nullptr; });
}

// --- Options popup ---

void PlayerWindow::toggleOptionsPopup() {
    if (optionsPopup) {
        closePopup(optionsPopup, settingsBtn, "Options (O)");
        return;
    }

    // Close other popups
    if (audioPopup) { audioPopup->close(); delete audioPopup; audioPopup = nullptr; audioTrackBtn->setStyleSheet(""); }
    if (contextPopup) { contextPopup->close(); delete contextPopup; contextPopup = nullptr; }

    settingsBtn->setToolTip("");

    auto *glass = new GlassPanel(this);
    glass->setRadius(Theme::popupRadius);
    glass->setBackgroundColor(Theme::popupBg);
    glass->setHighlightStrength(Theme::highlight);
    glass->setStyleSheet(popupStyleSheet());

    optionsPopup = glass;
    settingsBtn->setStyleSheet(QString("color: %1; background: %2; border-radius: 10px;").arg(Theme::accent, Theme::btnPressBg));

    auto *layout = new QVBoxLayout(glass);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(2);
    buildMenuItems(layout, glass, false);

    glass->adjustSize();
    int popupX = controlsBar->x() + controlsBar->width() - glass->width();
    int popupY = controlsBar->y() - glass->height() - Theme::popupGap;
    glass->move(popupX, popupY);
    glass->raise();
    glass->show();
}

// --- Context menu (right-click) ---

void PlayerWindow::showContextPopup(const QPoint &globalPos) {
    if (contextPopup) { contextPopup->close(); delete contextPopup; contextPopup = nullptr; return; }
    if (optionsPopup) closePopup(optionsPopup, settingsBtn, "Options (O)");

    auto *glass = new GlassPanel(this);
    glass->setRadius(Theme::popupRadius);
    glass->setBackgroundColor(Theme::popupBg);
    glass->setHighlightStrength(Theme::highlight);
    glass->setStyleSheet(popupStyleSheet());

    contextPopup = glass;

    auto *layout = new QVBoxLayout(glass);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(2);
    buildMenuItems(layout, glass, true);

    glass->adjustSize();
    QPoint localPos = mapFromGlobal(globalPos);
    int x = qMin(localPos.x(), width() - glass->width() - 8);
    int y = qMin(localPos.y(), height() - glass->height() - 8);
    glass->move(x, y);
    glass->raise();
    glass->show();
}

// --- Shared menu items ---

void PlayerWindow::buildMenuItems(QVBoxLayout *layout, QWidget *parent, bool fromContextMenu) {
    // Helper to close the right popup
    auto close = [this, fromContextMenu]() {
        if (fromContextMenu)
            closePopup(contextPopup);
        else
            closePopup(optionsPopup, settingsBtn, "Options (O)");
    };

    // Open file
    auto *openBtn = makeMenuButton("Open file", "Ctrl+O", parent);
    connect(openBtn, &QPushButton::clicked, this, [this, close]() { close(); promptOpenFile(); });
    layout->addWidget(openBtn);

    layout->addWidget(makeSeparator(parent));

    // Loop
    bool looping = controller->isLooping();
    auto *loopBtn = makeMenuButton(QString("Loop: %1").arg(looping ? "ON" : "OFF"), "L", parent);
    if (looping) loopBtn->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::accent));
    connect(loopBtn, &QPushButton::clicked, this, [this, loopBtn]() {
        controller->toggleLoop();
        bool on = controller->isLooping();
        setMenuButtonText(loopBtn, QString("Loop: %1").arg(on ? "ON" : "OFF"));
        loopBtn->setStyleSheet(on ? QString("color: %1; font-weight: bold;").arg(Theme::accent) : "");
        showNotification(on ? "Loop: ON" : "Loop: OFF");
    });
    layout->addWidget(loopBtn);

    // Speed controls
    auto *speedRow = new QWidget(parent);
    auto *speedLayout = new QHBoxLayout(speedRow);
    speedLayout->setContentsMargins(12, 6, 12, 6);
    speedLayout->setSpacing(8);

    auto *speedLabel = new QLabel(QString("Speed: %1x").arg(currentSpeed, 0, 'f', 2), speedRow);
    speedLabel->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent; padding: 0;").arg(Theme::textPrimary));

    auto btnStyle = QString("color: %1; background: %2; border: none; border-radius: 6px; font-size: 11px; text-align: center; padding: 0;")
        .arg(Theme::textPrimary, Theme::btnHoverBg);

    auto *speedDown  = new QPushButton("−", speedRow);
    auto *speedReset = new QPushButton("1x", speedRow);
    auto *speedUp    = new QPushButton("+", speedRow);
    for (auto *b : {speedDown, speedReset, speedUp}) {
        b->setFixedSize(24, 24);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        b->setStyleSheet(btnStyle);
    }

    auto *speedShortcut = new QLabel("+/−", speedRow);
    speedShortcut->setStyleSheet(QString("color: %1; background: transparent; padding: 0; font-weight: bold;").arg(Theme::textDim));

    auto updateSpeed = [speedLabel, this]() {
        speedLabel->setText(QString("Speed: %1x").arg(currentSpeed, 0, 'f', 2));
    };
    connect(speedDown,  &QPushButton::clicked, this, [this, updateSpeed]() { setPlaybackSpeed(qMax(0.25, currentSpeed - 0.25)); updateSpeed(); });
    connect(speedUp,    &QPushButton::clicked, this, [this, updateSpeed]() { setPlaybackSpeed(qMin(4.0, currentSpeed + 0.25)); updateSpeed(); });
    connect(speedReset, &QPushButton::clicked, this, [this, updateSpeed]() { setPlaybackSpeed(1.0); updateSpeed(); });

    speedLayout->addWidget(speedLabel);
    speedLayout->addStretch();
    speedLayout->addWidget(speedDown);
    speedLayout->addWidget(speedReset);
    speedLayout->addWidget(speedUp);
    speedLayout->addWidget(speedShortcut);
    layout->addWidget(speedRow);

    // Fullscreen
    auto *fullscreenBtn = makeMenuButton("Toggle Fullscreen", "F", parent);
    connect(fullscreenBtn, &QPushButton::clicked, this, [this, close]() { close(); toggleFullscreen(); });
    layout->addWidget(fullscreenBtn);

    layout->addWidget(makeSeparator(parent));

    // Screenshot
    auto *screenshotBtn = makeMenuButton(
        controller->hasFFmpeg() ? "Copy frame (full resolution)" : "Copy frame (visible size)", "S", parent);
    connect(screenshotBtn, &QPushButton::clicked, this, [this, close]() {
        close();
        bool ok = controller->copyFrameToClipboard();
        showNotification(ok ? "Frame copied to clipboard" : "Screenshot failed");
    });
    layout->addWidget(screenshotBtn);

    // File info
    auto *infoBtn = makeMenuButton(QString("File Info: %1").arg(infoVisible ? "ON" : "OFF"), "I", parent);
    if (infoVisible) infoBtn->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::accent));
    connect(infoBtn, &QPushButton::clicked, this, [this, infoBtn]() {
        toggleInfoOverlay();
        setMenuButtonText(infoBtn, QString("File Info: %1").arg(infoVisible ? "ON" : "OFF"));
        infoBtn->setStyleSheet(infoVisible ? QString("color: %1; font-weight: bold;").arg(Theme::accent) : "");
    });
    layout->addWidget(infoBtn);

    layout->addWidget(makeSeparator(parent));

    // About
    auto *aboutBtn = makeMenuButton("About LMAO", "F1", parent);
    connect(aboutBtn, &QPushButton::clicked, this, [this, close]() { close(); showAboutDialog(); });
    layout->addWidget(aboutBtn);
}

// --- Popup helpers ---

void PlayerWindow::closePopup(QWidget *&popup, QPushButton *btn, const QString &tooltip) {
    if (!popup) return;
    popup->hide();
    popup->deleteLater();
    popup = nullptr;
    if (btn) {
        btn->setStyleSheet("");
        if (!tooltip.isEmpty()) btn->setToolTip(tooltip);
    }
}

void PlayerWindow::refreshOptionsPopup() {
    if (optionsPopup) {
        closePopup(optionsPopup, settingsBtn, "Options (O)");
        toggleOptionsPopup();
    }
    if (contextPopup) {
        QPoint pos = contextPopup->pos();
        contextPopup->close();
        delete contextPopup;
        contextPopup = nullptr;
        showContextPopup(mapToGlobal(pos));
    }
}

// --- Info overlay ---

void PlayerWindow::toggleInfoOverlay() {
    infoVisible = !infoVisible;

    if (!infoOverlay) {
        infoOverlay = new GlassPanel(this);
        infoOverlay->setRadius(10);
        infoOverlay->setBackgroundColor(Theme::popupBg);
        infoOverlay->setHighlightStrength(Theme::highlight);

        infoLabel = new QLabel(infoOverlay);
        infoLabel->setStyleSheet(QString(
            "color: %1; font-size: 11px; font-family: monospace; background: transparent; padding: 0;"
        ).arg(Theme::textPrimary));

        auto *layout = new QVBoxLayout(infoOverlay);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->addWidget(infoLabel);

        infoUpdateTimer = new QTimer(this);
        infoUpdateTimer->setInterval(1000);
        connect(infoUpdateTimer, &QTimer::timeout, this, &PlayerWindow::updateInfoOverlay);
    }

    if (infoVisible) {
        updateInfoOverlay();
        infoOverlay->move(16, 16);
        infoOverlay->raise();
        infoOverlay->show();
        infoUpdateTimer->start();
    } else {
        infoOverlay->hide();
        infoUpdateTimer->stop();
    }
}

void PlayerWindow::updateInfoOverlay() {
    if (!infoOverlay || !infoVisible) return;

    QString info;
    info += QString("File: %1\n").arg(controller->currentFilePath());

    if (controller->isVideoFile()) {
        int64_t w = mpvWidget->getPropertyInt("video-params/w");
        int64_t h = mpvWidget->getPropertyInt("video-params/h");
        QString codec = mpvWidget->getPropertyString("video-codec");
        QString hwdec = mpvWidget->getPropertyString("hwdec-current");
        double fps = mpvWidget->getPropertyDouble("container-fps");

        // Average bitrate from file size / duration
        QString path = controller->currentFilePath();
        double dur = controller->duration();
        double bitrateMbps = 0;
        if (!path.isEmpty() && dur > 0) {
            QFileInfo fi(path);
            bitrateMbps = (fi.size() * 8.0) / dur / 1000000.0;
        }

        info += QString("Video: %1x%2 @ %3fps\n").arg(w).arg(h).arg(fps, 0, 'f', 2);
        info += QString("Codec: %1\n").arg(codec);
        info += QString("Bitrate: ~%1 Mbps\n").arg(bitrateMbps, 0, 'f', 2);
        if (!hwdec.isEmpty())
            info += QString("HW Decode: %1\n").arg(hwdec);
    }

    // Audio tracks
    int64_t totalCount = mpvWidget->getPropertyInt("track-list/count");
    for (int64_t i = 0; i < totalCount; ++i) {
        QString prefix = QString("track-list/%1/").arg(i);
        if (mpvWidget->getPropertyString(prefix + "type") != "audio")
            continue;

        int64_t id       = mpvWidget->getPropertyInt(prefix + "id");
        QString codec    = mpvWidget->getPropertyString(prefix + "codec");
        QString lang     = mpvWidget->getPropertyString(prefix + "lang");
        QString title    = mpvWidget->getPropertyString(prefix + "title");
        int64_t channels = mpvWidget->getPropertyInt(prefix + "demux-channel-count");
        int64_t rate     = mpvWidget->getPropertyInt(prefix + "demux-samplerate");
        int64_t bitrate  = mpvWidget->getPropertyInt(prefix + "demux-bitrate");

        QString trackInfo = QString("Audio %1: %2").arg(id).arg(codec);
        if (!lang.isEmpty())  trackInfo += QString(" | %1").arg(lang);
        if (!title.isEmpty()) trackInfo += QString(" | %1").arg(title);
        if (channels > 0)     trackInfo += QString(" | %1ch").arg(channels);
        if (rate > 0)         trackInfo += QString(" | %1Hz").arg(rate);
        if (bitrate > 0) {
            bool lossless = (codec == "flac" || codec == "pcm" || codec == "alac" || codec == "wav");
            trackInfo += QString(" | %1%2kbps").arg(lossless ? "" : "~").arg(bitrate / 1000);
        }
        if (id == controller->currentAudioTrack())
            trackInfo += " ◄";

        info += trackInfo + "\n";
    }

    infoLabel->setText(info.trimmed());
    infoOverlay->adjustSize();
}

// --- About dialog ---

void PlayerWindow::showAboutDialog() {
    if (aboutPanel) { aboutPanel->raise(); return; }

    auto *glass = new GlassPanel(this);
    glass->setRadius(Theme::popupRadius);
    glass->setBackgroundColor(Theme::popupBg);
    glass->setHighlightStrength(Theme::highlight);
    aboutPanel = glass;

    auto *layout = new QVBoxLayout(glass);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(8);

    auto *title = new QLabel("LMAO 🤣", glass);
    title->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold; background: transparent;").arg(Theme::accent));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *subtitle = new QLabel("Lightweight Multimedia & Audio Opener", glass);
    subtitle->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(Theme::textPrimary));
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    auto *version = new QLabel(QString("v%1").arg(APP_VERSION), glass);
    version->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent;").arg(Theme::textDim));
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);

    auto *techInfo = new QLabel(
        "Built with Qt6, libmpv, and FFmpeg\n"
        "Licensed under GPL-2.0-or-later\n",
        glass);
    techInfo->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent;").arg(Theme::textDim));
    techInfo->setAlignment(Qt::AlignCenter);
    layout->addWidget(techInfo);

    auto *githubBtn = new QPushButton("GitHub ↗", glass);
    githubBtn->setCursor(Qt::PointingHandCursor);
    githubBtn->setFocusPolicy(Qt::NoFocus);
    githubBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    githubBtn->setStyleSheet(QString(
        "color: %1; background: %2; border: none; border-radius: 10px; padding: 6px 16px; font-size: 12px;"
    ).arg(Theme::accent, Theme::btnHoverBg));
    connect(githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/ElekKartofelek/LMAO"));
    });
    layout->addWidget(githubBtn);

    auto *closeBtn = new QPushButton("Close", glass);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    closeBtn->setStyleSheet(QString(
        "color: %1; background: %2; border: none; border-radius: 10px; padding: 6px 16px; font-size: 12px;"
    ).arg(Theme::textPrimary, Theme::btnHoverBg));
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        aboutPanel->close();
        delete aboutPanel;
        aboutPanel = nullptr;
    });
    layout->addWidget(closeBtn);

    glass->adjustSize();
    glass->move((width() - glass->width()) / 2, (height() - glass->height()) / 2);
    glass->raise();
    glass->show();
}

// --- Resize ---

void PlayerWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    // Controls bar
    int barWidth = qMin(width() - (Theme::barMargin * 2), Theme::barMaxWidth);
    int barX = (width() - barWidth) / 2;
    controlsBar->setFixedWidth(barWidth);
    controlsBar->move(barX, height() - controlsBar->height() - Theme::barMargin);
    controlsBar->raise();

    // Reposition popups
    if (audioPopup) {
        audioPopup->move(controlsBar->x() + controlsBar->width() - audioPopup->width(),
                         controlsBar->y() - audioPopup->height() - Theme::popupGap);
        audioPopup->raise();
    }
    if (optionsPopup) {
        optionsPopup->move(controlsBar->x() + controlsBar->width() - optionsPopup->width(),
                           controlsBar->y() - optionsPopup->height() - Theme::popupGap);
        optionsPopup->raise();
    }
    if (notifPanel->isVisible()) {
        int notifY = height() - Theme::barMargin - Theme::barHeight - Theme::popupGap - notifPanel->height();
        notifPanel->move((width() - notifPanel->width()) / 2, notifY);
        notifPanel->raise();
    }
    if (infoOverlay && infoOverlay->isVisible())
        infoOverlay->move(16, 16);
    if (aboutPanel) {
        aboutPanel->move((width() - aboutPanel->width()) / 2, (height() - aboutPanel->height()) / 2);
        aboutPanel->raise();
    }
}

// --- Formatting ---

QString PlayerWindow::formatTime(double seconds, double total) {
    if (seconds < 0) seconds = 0;
    int h = static_cast<int>(seconds) / 3600;
    int m = (static_cast<int>(seconds) % 3600) / 60;
    int s = static_cast<int>(seconds) % 60;

    if (static_cast<int>(total) / 3600 > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    else
        return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

// --- Input handling ---

bool PlayerWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == mpvWidget) {
        switch (event->type()) {
        case QEvent::Wheel: {
            auto *we = static_cast<QWheelEvent *>(event);
            static int accumulated = 0;
            accumulated += we->angleDelta().y();
            if (qAbs(accumulated) >= 120) {
                controller->adjustVolume(accumulated > 0 ? 5 : -5);
                accumulated = 0;
            }
            return true;
        }
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            // Close context popup on click
            if (contextPopup) { contextPopup->close(); delete contextPopup; contextPopup = nullptr; return true; }
            // Single click → pause (with double-click protection)
            if (me->button() == Qt::LeftButton) {
                pauseStateBeforeClick = controller->isPaused();
                controller->togglePause();
            }
            return true;
        }
        case QEvent::MouseButtonDblClick: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                // Restore pause state, then fullscreen
                if (controller->isPaused() != pauseStateBeforeClick)
                    controller->togglePause();
                toggleFullscreen();
            }
            return true;
        }
        case QEvent::MouseMove:
            showControls();
            hideTimer->start();
            return false;
        case QEvent::ContextMenu: {
            auto *ce = static_cast<QContextMenuEvent *>(event);
            QContextMenuEvent forwarded(QContextMenuEvent::Mouse, ce->pos(), ce->globalPos());
            contextMenuEvent(&forwarded);
            return true;
        }
        default: break;
        }
    }

    // Keep controls visible while hovering
    if (obj == controlsBar) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
            showControls();
            hideTimer->stop();
            return false;
        }
        if (event->type() == QEvent::Leave) {
            hideTimer->start();
            return false;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void PlayerWindow::keyPressEvent(QKeyEvent *event) {
    showControls();
    hideTimer->start();

    switch (event->key()) {
    case Qt::Key_Space:  controller->togglePause(); break;
    case Qt::Key_Right:  controller->seekRelative(5); showNotification("Skip 5s ►"); break;
    case Qt::Key_Left:   controller->seekRelative(-5); showNotification("◄ Skip 5s"); break;
    case Qt::Key_Up:     controller->adjustVolume(5); break;
    case Qt::Key_Down:   controller->adjustVolume(-5); break;
    case Qt::Key_F:      toggleFullscreen(); break;
    case Qt::Key_S: {
        bool ok = controller->copyFrameToClipboard();
        showNotification(ok ? "Frame copied to clipboard" : "Screenshot failed");
        break;
    }
    case Qt::Key_Period:
        frameStepping = true;
        controller->frameStepForward();
        showNotification("Next Frame ►");
        QTimer::singleShot(100, this, [this]() { frameStepping = false; });
        break;
    case Qt::Key_Comma:
        frameStepping = true;
        controller->frameStepBackward();
        showNotification("◄ Previous Frame");
        QTimer::singleShot(100, this, [this]() { frameStepping = false; });
        break;
    case Qt::Key_Escape:
        if (isFullScreen()) showNormal();
        break;
    case Qt::Key_O:
        if (event->modifiers() & Qt::ControlModifier) promptOpenFile();
        else toggleOptionsPopup();
        break;
    case Qt::Key_M:
        if (controller->volume() > 0) { lastVolume = controller->volume(); controller->setVolume(0); }
        else controller->setVolume(lastVolume > 0 ? lastVolume : 100);
        break;
    case Qt::Key_L:
        controller->toggleLoop();
        showNotification(controller->isLooping() ? "Loop: ON" : "Loop: OFF");
        refreshOptionsPopup();
        break;
    case Qt::Key_Minus:
        setPlaybackSpeed(qMax(0.25, currentSpeed - 0.25));
        refreshOptionsPopup();
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        setPlaybackSpeed(qMin(4.0, currentSpeed + 0.25));
        refreshOptionsPopup();
        break;
    case Qt::Key_I:
        toggleInfoOverlay();
        refreshOptionsPopup();
        break;
    case Qt::Key_F1:
        showAboutDialog();
        break;
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void PlayerWindow::mouseMoveEvent(QMouseEvent *event) {
    showControls();
    hideTimer->start();
    QMainWindow::mouseMoveEvent(event);
}

void PlayerWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void PlayerWindow::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty())
        controller->openFile(urls.first().toLocalFile());
}

void PlayerWindow::contextMenuEvent(QContextMenuEvent *event) {
    showContextPopup(event->globalPos());
}