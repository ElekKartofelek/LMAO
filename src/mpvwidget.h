#pragma once

#include <QOpenGLWidget>
#include <mpv/client.h>
#include <mpv/render_gl.h>

class MpvWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;

    void command(const QStringList &args);
    void setPropertyDouble(const QString &name, double value);
    void setPropertyBool(const QString &name, bool value);
    void setPropertyString(const QString &name, const QString &value);
    double getPropertyDouble(const QString &name) const;
    QString getPropertyString(const QString &name) const;
    int64_t getPropertyInt(const QString &name) const;

    void loadFile(const QString &path);
    void clearFrame();
    mpv_handle *handle() const { return mpv; }

signals:
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pauseChanged(bool paused);
    void audioTrackChanged(int64_t id);
    void fileLoaded();

protected:
    void initializeGL() override;
    void paintGL() override;

private slots:
    void onMpvEvents();
    void maybeUpdate();

private:
    mpv_handle *mpv = nullptr;
    mpv_render_context *mpv_gl = nullptr;

    void handleMpvEvent(mpv_event *event);
    static void onUpdate(void *ctx);
    static void *getProcAddress(void *ctx, const char *name);
    static void wakeup(void *ctx);
};
