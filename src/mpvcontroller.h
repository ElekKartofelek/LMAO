#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>

class MpvWidget;

struct AudioTrackInfo {
    int64_t id;
    QString title;
    QString lang;
    QString codec;
};

class MpvController : public QObject {
    Q_OBJECT

public:
    explicit MpvController(MpvWidget *widget, QObject *parent = nullptr);

    // File
    void openFile(const QString &path);

    // Playback
    void togglePause();
    void seekTo(double seconds);
    void seekRelative(double seconds);
    void frameStepForward();
    void frameStepBackward();
    void toggleLoop();
    void setSpeed(double speed);
    double speed() const;


    // Audio
    void cycleAudioTrack();
    void setAudioTrack(int64_t id);
    int64_t currentAudioTrack() const;
    QList<AudioTrackInfo> audioTracks() const;

    // Volume
    void adjustVolume(int delta);
    void setVolume(int vol);
    int volume() const;

    // Screenshot
    bool copyFrameToClipboard();

    // State
    bool isPaused() const;
    double position() const;
    double duration() const;
    QString currentFilePath() const;
    bool hasMedia() const;
    bool isLooping() const;
    bool hasMultipleAudioTracks() const;
    bool isVideoFile() const;

    // Tool availability
    bool hasFFmpeg() const { return ffmpegAvailable; }
    bool hasFFprobe() const { return ffprobeAvailable; }
    
signals:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pauseChanged(bool paused);
    void audioTrackChanged(int64_t id);
    void volumeChanged(int volume);
    void fileLoaded(const QString &filename);

private:
    MpvWidget *mpv;
    
    void loadFileInternal(const QString &path);

    bool ffmpegAvailable = false;
    bool ffprobeAvailable = false;
    
    // Volume smoothing
    QTimer *volumeLerpTimer;
    double volumeTarget = 100;
    double volumeCurrent = 100;
};