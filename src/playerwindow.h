#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QTimer>
#include "widgets/customslider.h"
#include "widgets/glasspanel.h"

class MpvWidget;
class MpvController;
class QVBoxLayout;

class PlayerWindow : public QMainWindow {
    Q_OBJECT
#ifdef USE_DBUS
    Q_CLASSINFO("D-Bus Interface", "dev.elek.LMAO")
#endif

public:
    explicit PlayerWindow(QWidget *parent = nullptr);

public slots:
#ifdef USE_DBUS
    Q_SCRIPTABLE
#endif
    void openFile(const QString &path);
#ifdef USE_DBUS
    Q_SCRIPTABLE
#endif
    void raiseWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // --- Core ---
    MpvWidget *mpvWidget;
    MpvController *controller;

    // --- Controls bar ---
    GlassPanel *controlsBar;
    QPushButton *playBtn;
    CustomSlider *seekBar;
    QLabel *timeLabel;
    QPushButton *muteBtn;
    CustomSlider *volumeSlider;
    QPushButton *audioTrackBtn;
    QPushButton *settingsBtn;

    // --- Popups ---
    QWidget *audioPopup = nullptr;
    QWidget *contextPopup = nullptr;
    QWidget *optionsPopup = nullptr;
    QWidget *aboutPanel = nullptr;

    // --- Info overlay ---
    GlassPanel *infoOverlay = nullptr;
    QLabel *infoLabel = nullptr;
    QTimer *infoUpdateTimer = nullptr;
    bool infoVisible = false;

    // --- Notification ---
    GlassPanel *notifPanel;
    QLabel *notifLabel;
    QTimer *notifTimer;

    // --- Auto-hide ---
    QTimer *hideTimer;
    bool controlsVisible = true;

    // --- State ---
    double currentSpeed = 1.0;
    double cachedDuration = 0.0;
    bool seeking = false;
    bool frameStepping = false;
    bool pauseStateBeforeClick = false;
    bool pauseStateBeforeSeek = false;
    int lastVolume = 100;

    // --- Setup ---
    void buildLayout();
    void buildControlsBar();
    void buildMenuItems(QVBoxLayout *layout, QWidget *parent, bool fromContextMenu);
    void connectSignals();

    // --- UI helpers ---
    void showControls();
    void hideControls();
    void toggleFullscreen();
    void toggleAudioPopup();
    void toggleOptionsPopup();
    void toggleInfoOverlay();
    void updateInfoOverlay();
    void showAboutDialog();
    void showContextPopup(const QPoint &pos);
    void showNotification(const QString &text);
    void promptOpenFile();
    void refreshOptionsPopup();
    void closePopup(QWidget *&popup, QPushButton *btn = nullptr, const QString &tooltip = "");
    void setPlaybackSpeed(double speed);

    // --- Formatting ---
    static QString formatTime(double seconds, double total = 0);
};