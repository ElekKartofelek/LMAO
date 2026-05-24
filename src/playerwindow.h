#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QTimer>
#include "widgets/customslider.h"
#include "widgets/glasspanel.h"
#include "widgets/glassbutton.h"

class MpvWidget;
class MpvController;
class QVBoxLayout;
class QProcess;

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
    GlassButton *playBtn;
    CustomSlider *seekBar;
    QLabel *timeLabel;
    GlassButton *muteBtn;
    CustomSlider *volumeSlider;
    GlassButton *audioTrackBtn;
    GlassButton *editBtn;
    GlassButton *settingsBtn;
    GlassButton *cancelEditBtn;
    GlassButton *exportBtn;

    // --- Popups ---
    QWidget *audioPopup = nullptr;
    QWidget *optionsPopup = nullptr;
    QWidget *contextPopup = nullptr;
    QWidget *aboutPanel = nullptr;
    QWidget *keybindsPanel = nullptr;

    // --- Export overlay ---
    QWidget *exportDimmer = nullptr;
    GlassPanel *exportOverlay = nullptr;
    QLabel *exportStatusLabel = nullptr;
    CustomSlider *exportProgressBar = nullptr;
    QProcess *exportProcess = nullptr;
    bool exportWasPaused = false;

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

    // --- Playback state ---
    double currentSpeed = 1.0;
    double cachedDuration = 0.0;
    bool seeking = false;
    bool frameStepping = false;
    bool pauseStateBeforeClick = false;
    int lastVolume = 100;

    // --- Edit state ---
    bool editMode = false;
    int savedClampIn = -1;
    int savedClampOut = -1;
    QWidget *editBorder = nullptr;

    // --- Setup ---
    void buildLayout();
    void buildControlsBar();
    void connectSignals();

    // --- Controls ---
    void showControls();
    void hideControls();
    void toggleFullscreen();
    void updateResponsiveLayout();

    // --- Popups & dialogs ---
    void toggleAudioPopup();
    void toggleOptionsPopup();
    void showContextPopup(const QPoint &pos);
    void buildMenuItems(QVBoxLayout *layout, QWidget *parent, bool fromContextMenu);
    void closePopup(QWidget *&popup, QWidget *btn = nullptr, const QString &tooltip = "");
    void refreshOptionsPopup();
    void showAboutDialog();
    void showKeybindsPanel();

    // --- Edit mode ---
    void toggleEditMode();
    void updateEditModeUI();
    void clampSeekToEditRegion();

    // --- Export ---
    void exportClip();
    void cancelExport();
    void closeExportOverlay();

    // --- Info overlay ---
    void toggleInfoOverlay();
    void updateInfoOverlay();

    // --- Misc ---
    void promptOpenFile();
    void showNotification(const QString &text);
    void setPlaybackSpeed(double speed);
    static QString formatTime(double seconds, double total = 0);
};