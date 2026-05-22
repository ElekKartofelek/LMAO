#include "mpvcontroller.h"
#include "mpvwidget.h"
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <ctime>
#include <cstdlib>


static QString findTool(const QString &name) {
#ifdef _WIN32
    QString local = QCoreApplication::applicationDirPath() + "/" + name + ".exe";
    if (QFile::exists(local))
        return local;
#endif
    return name;
}

static bool toolExists(const QString &name) {
    QString path = findTool(name);
    QProcess test;
    test.start(path, {"-version"});
    if (test.waitForFinished(200))
        return test.exitCode() == 0;
    return false;
}


MpvController::MpvController(MpvWidget *widget, QObject *parent)
    : QObject(parent), mpv(widget)
{
    // Cache tool availability at startup
    ffmpegAvailable = toolExists("ffmpeg");
    ffprobeAvailable = toolExists("ffprobe");

    connect(mpv, &MpvWidget::positionChanged, this, &MpvController::positionChanged);
    connect(mpv, &MpvWidget::durationChanged, this, &MpvController::durationChanged);
    connect(mpv, &MpvWidget::pauseChanged, this, &MpvController::pauseChanged);
    connect(mpv, &MpvWidget::audioTrackChanged, this, &MpvController::audioTrackChanged);

    connect(mpv, &MpvWidget::fileLoaded, this, [this]() {
        QString path = mpv->getPropertyString("path");
        QFileInfo fi(path);
        emit fileLoaded(fi.fileName());
    });

    volumeLerpTimer = new QTimer(this);
    volumeLerpTimer->setInterval(16);  // ~ 60fps
    connect(volumeLerpTimer, &QTimer::timeout, this, [this]() {
        double diff = volumeTarget - volumeCurrent;
        if (qAbs(diff) < 0.3) {
            volumeCurrent = volumeTarget;
            volumeLerpTimer->stop();
        } else {
            volumeCurrent += diff * 0.05;  // lerp speed
        }
        mpv->setPropertyDouble("volume", volumeCurrent);
    });
}



// --- File ---

void MpvController::openFile(const QString &path) {
    QFileInfo fi(path);
    qint64 sizeBytes = fi.size();

    if (!hasFFprobe()) {
        // ffprobe unavailable - load directly and skip bitrate check
        loadFileInternal(path);
        return;
    }

    // async ffprobe check before loading to prevent system freezing from high (above 500Mbps) bitrate
    // vo=libmpv is used to composite mpv with Qt but this strains GPU memory bandwidth
    // the cycle is: mpv decodes -> renders to OpenGl FBO (framebuffer object) -> Qt reads FBO and composites
    // current solution is to warn users and provide option to open in external mpv player
    // TODO: research possible optimizations to allow custom UI and get back pure mpv performance
    
    auto *probe = new QProcess(this);

    connect(probe, &QProcess::errorOccurred, this, [this, path, probe](QProcess::ProcessError) {
        probe->deleteLater();
        loadFileInternal(path);
    });

    connect(probe, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this, [this, path, sizeBytes, probe](int exitCode, QProcess::ExitStatus) {
            probe->deleteLater();

            if (exitCode == 0) {
                bool ok = false;
                double dur = probe->readAllStandardOutput().trimmed().toDouble(&ok);
                if (ok && dur > 0) {
                    // calculate bitrate: file size in bits / duration in seconds / 1M = Mbps
                    double bitrateMbps = (sizeBytes * 8.0) / dur / 1000000.0;
                    // 500 Mbps threshold: 4K Blu-ray is ~100 Mbps
                    // anything above 500 is likely uncompressed or ProRes
                    if (bitrateMbps > 500) {
                        auto answer = QMessageBox::question(nullptr,
                            "High Bitrate File",
                            QString("This file has an extremely high bitrate (%1 Mbps) "
                                    "and cannot be played smoothly in the built-in renderer.\n\n"
                                    "Would you like to open it in an external mpv window instead?")
                                    .arg((int)bitrateMbps),
                            QMessageBox::Yes | QMessageBox::No);

                        if (answer == QMessageBox::Yes) {
                            QProcess::startDetached("mpv", {path});
                        }
                        return;
                    }
                }
            }

            loadFileInternal(path);
        });

    probe->start(findTool("ffprobe"), {"-v", "quiet", "-show_entries", "format=duration",
                             "-of", "default=noprint_wrappers=1:nokey=1", path});
}


void MpvController::loadFileInternal(const QString &path) {
    mpv->setPropertyString("aid", "auto");
    mpv->loadFile(path);
    mpv->setPropertyBool("pause", false);

    QTimer::singleShot(300, this, [this]() {
        if (!isVideoFile())
            mpv->clearFrame();
    });
}


// --- Playback ---

void MpvController::togglePause() {
    if (!hasMedia()) return;
    
    // If at the end - restart from beginning
    if (isPaused() && position() >= duration() - 0.5) {
        seekTo(0);
    }
    
    mpv->command({"cycle", "pause"});
}

void MpvController::seekTo(double seconds) {
    if (!hasMedia()) return;
    mpv->setPropertyDouble("time-pos", seconds);
}

void MpvController::seekRelative(double seconds) {
    if (!hasMedia()) return;
    mpv->command({"seek", QString::number(seconds)});
}

void MpvController::frameStepForward() {
    if (!hasMedia()) return;
    mpv->command({"frame-step"});
}

void MpvController::frameStepBackward() {
    if (!hasMedia()) return;
    mpv->command({"frame-back-step"});
}

void MpvController::toggleLoop() {
    QString current = mpv->getPropertyString("loop-file");
    bool looping = (current == "inf");
    mpv->setPropertyString("loop-file", looping ? "no" : "inf");
}

// --- Audio ---

void MpvController::setAudioTrack(int64_t id) {
    mpv->setPropertyString("aid", QString::number(id));
}

QList<AudioTrackInfo> MpvController::audioTracks() const {
    QList<AudioTrackInfo> tracks;
    int64_t count = mpv->getPropertyInt("track-list/count");

    for (int64_t i = 0; i < count; ++i) {
        QString prefix = QString("track-list/%1/").arg(i);
        QString type = mpv->getPropertyString(prefix + "type");

        if (type != "audio")
            continue;

        AudioTrackInfo info;
        info.id    = mpv->getPropertyInt(prefix + "id");
        info.title = mpv->getPropertyString(prefix + "title");
        info.lang  = mpv->getPropertyString(prefix + "lang");
        info.codec = mpv->getPropertyString(prefix + "codec");
        tracks.append(info);
    }

    return tracks;
}

void MpvController::cycleAudioTrack() {
    mpv->command({"cycle", "audio"});
}

int64_t MpvController::currentAudioTrack() const {
    return mpv->getPropertyInt("aid");
}

void MpvController::setSpeed(double speed) {
    mpv->setPropertyDouble("speed", speed);
}

double MpvController::speed() const {
    return mpv->getPropertyDouble("speed");
}


// --- Volume ---

void MpvController::adjustVolume(int delta) {
    int vol = qBound(0, static_cast<int>(volumeTarget) + delta, 100);
    setVolume(vol);
}

void MpvController::setVolume(int vol) {
    vol = qBound(0, vol, 100);
    volumeTarget = vol;
    emit volumeChanged(vol);
    if (!volumeLerpTimer->isActive())
        volumeLerpTimer->start();
}

int MpvController::volume() const {
    return static_cast<int>(volumeTarget);
}

// --- Screenshot ---

bool MpvController::copyFrameToClipboard() {
    if (!hasMedia()) return false;

    // Try full-resolution screenshot via ffmpeg if available
    if (hasFFmpeg()) {
        QString path = currentFilePath();
        double pos = position();
        if (!path.isEmpty() && pos >= 0) {
            QString tmpPath = QDir::tempPath() + "/lmao_frame.png";
            QFile::remove(tmpPath);

            QProcess ffmpeg;
            ffmpeg.start(findTool("ffmpeg"), {
                "-ss", QString::number(pos, 'f', 3),
                "-i", path,
                "-frames:v", "1",
                "-y",
                tmpPath
            });

            if (ffmpeg.waitForFinished(5000)) {
                QFile f(tmpPath);
                if (f.exists() && f.size() > 0 && f.open(QIODevice::ReadOnly)) {
                    QByteArray pngData = f.readAll();
                    f.close();
                    QFile::remove(tmpPath);

                    QImage frame;
                    frame.loadFromData(pngData, "PNG");

                    QMimeData *mimeData = new QMimeData();
                    mimeData->setImageData(frame);
                    mimeData->setData("image/png", pngData);
                    QApplication::clipboard()->setMimeData(mimeData);
                    return true;
                }
            }
        }
    }

    // Fallback: grab from widget (window resolution)
    QImage frame = mpv->grabFramebuffer();
    if (frame.isNull()) return false;

    QMimeData *mimeData = new QMimeData();
    mimeData->setImageData(frame);

    QByteArray pngData;
    QBuffer buffer(&pngData);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "PNG");
    buffer.close();
    mimeData->setData("image/png", pngData);

    QApplication::clipboard()->setMimeData(mimeData);
    return true;
}

// --- State ---

bool MpvController::isPaused() const {
    int flag = 0;
    mpv_get_property(mpv->handle(), "pause", MPV_FORMAT_FLAG, &flag);
    return flag != 0;
}

double MpvController::position() const {
    return mpv->getPropertyDouble("time-pos");
}

double MpvController::duration() const {
    return mpv->getPropertyDouble("duration");
}

QString MpvController::currentFilePath() const {
    return mpv->getPropertyString("path");
}

bool MpvController::hasMedia() const {
    return !mpv->getPropertyString("path").isEmpty();
}

bool MpvController::isLooping() const {
    return mpv->getPropertyString("loop-file") == "inf";
}

bool MpvController::isVideoFile() const {
    int64_t count = mpv->getPropertyInt("track-list/count");
    for (int64_t i = 0; i < count; ++i) {
        QString type = mpv->getPropertyString(QString("track-list/%1/type").arg(i));
        if (type == "video")
            return true;
    }
    return false;
}

bool MpvController::hasMultipleAudioTracks() const {
    return audioTracks().size() > 1;
}
