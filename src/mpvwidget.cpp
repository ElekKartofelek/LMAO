#include "mpvwidget.h"

#include <QOpenGLContext>
#include <QMetaObject>
#include <stdexcept>
#include <cstring>
#include <clocale>

void MpvWidget::wakeup(void *ctx) {
    // Queue event processing on the GUI thread
    QMetaObject::invokeMethod(static_cast<MpvWidget *>(ctx),
                              "onMpvEvents", Qt::QueuedConnection);
}

void *MpvWidget::getProcAddress(void *ctx, const char *name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return nullptr;
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}

void MpvWidget::onUpdate(void *ctx) {
    QMetaObject::invokeMethod(static_cast<MpvWidget *>(ctx), "maybeUpdate");
}

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("Failed to create mpv instance");

    // Use libmpv's own rendering (no wid, no separate window)
    mpv_set_option_string(mpv, "vo", "libmpv");

    // Disable mpv's own input handling — we do it ourselves
    mpv_set_option_string(mpv, "input-default-bindings", "no");
    mpv_set_option_string(mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(mpv, "osc", "no");
    mpv_set_option_string(mpv, "osd-level", "0");
    mpv_set_option_string(mpv, "keep-open", "yes");
    mpv_set_option_string(mpv, "hr-seek", "yes");
    mpv_set_option_string(mpv, "hwdec", "auto");
    

    // Performance: match standalone mpv's behavior
    mpv_set_option_string(mpv, "profile", "fast");                     // Use mpv's built-in fast profile
    mpv_set_option_string(mpv, "framedrop", "decoder+vo");             // Aggressive frame dropping
    mpv_set_option_string(mpv, "demuxer-max-bytes", "100000000");      // 100MB max buffer
    mpv_set_option_string(mpv, "demuxer-max-back-bytes", "50000000");  // 50MB back buffer
    mpv_set_option_string(mpv, "demuxer-readahead-secs", "3");         // Only read 3s ahead
    mpv_set_option_string(mpv, "video-latency-hacks", "yes");          // Reduce latency
    mpv_set_option_string(mpv, "volume-max", "100");                   // Clamp volume

    // Observe properties
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "aid", MPV_FORMAT_INT64);

    mpv_set_wakeup_callback(mpv, wakeup, this);

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("Failed to initialize mpv");
}

MpvWidget::~MpvWidget() {
    makeCurrent();
    if (mpv_gl)
        mpv_render_context_free(mpv_gl);
    mpv_terminate_destroy(mpv);
}

void MpvWidget::initializeGL() {
    mpv_opengl_init_params gl_init_params{getProcAddress, nullptr};
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
        throw std::runtime_error("Failed to initialize mpv GL context");

    mpv_render_context_set_update_callback(mpv_gl, onUpdate, this);
}

void MpvWidget::paintGL() {
    mpv_opengl_fbo mpfbo{
        static_cast<int>(defaultFramebufferObject()),
        static_cast<int>(width() * devicePixelRatio()),
        static_cast<int>(height() * devicePixelRatio()),
        0
    };
    int flip_y = 1;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(mpv_gl, params);
}

void MpvWidget::maybeUpdate() {
    // Handle minimized window — Qt skips update() when not visible,
    // which confuses mpv's render API and causes freezes.
    if (window()->isMinimized()) {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        doneCurrent();
    } else {
        update();
    }
}

void MpvWidget::onMpvEvents() {
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handleMpvEvent(event);
    }
}

void MpvWidget::handleMpvEvent(mpv_event *event) {
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        auto *prop = static_cast<mpv_event_property *>(event->data);
        if (std::strcmp(prop->name, "time-pos") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
            emit positionChanged(*static_cast<double *>(prop->data));
        } else if (std::strcmp(prop->name, "duration") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
            emit durationChanged(*static_cast<double *>(prop->data));
        } else if (std::strcmp(prop->name, "pause") == 0 && prop->format == MPV_FORMAT_FLAG) {
            emit pauseChanged(*static_cast<int *>(prop->data) != 0);
        } else if (std::strcmp(prop->name, "aid") == 0 && prop->format == MPV_FORMAT_INT64) {
            emit audioTrackChanged(*static_cast<int64_t *>(prop->data));
        }
        break;
    }
    case MPV_EVENT_FILE_LOADED:
        emit fileLoaded();
        break;
    default:
        break;
    }
}

// --- Command helpers ---

void MpvWidget::command(const QStringList &args) {
    QVector<QByteArray> utf8Args;
    QVector<const char *> cArgs;
    for (const auto &a : args) {
        utf8Args.append(a.toUtf8());
        cArgs.append(utf8Args.last().constData());
    }
    cArgs.append(nullptr);
    mpv_command(mpv, cArgs.data());
}

void MpvWidget::setPropertyDouble(const QString &name, double value) {
    mpv_set_property(mpv, name.toUtf8().constData(), MPV_FORMAT_DOUBLE, &value);
}

void MpvWidget::setPropertyBool(const QString &name, bool value) {
    int flag = value ? 1 : 0;
    mpv_set_property(mpv, name.toUtf8().constData(), MPV_FORMAT_FLAG, &flag);
}

void MpvWidget::setPropertyString(const QString &name, const QString &value) {
    QByteArray val = value.toUtf8();
    mpv_set_property_string(mpv, name.toUtf8().constData(), val.constData());
}

double MpvWidget::getPropertyDouble(const QString &name) const {
    double val = 0.0;
    mpv_get_property(mpv, name.toUtf8().constData(), MPV_FORMAT_DOUBLE, &val);
    return val;
}

QString MpvWidget::getPropertyString(const QString &name) const {
    char *val = mpv_get_property_string(mpv, name.toUtf8().constData());
    if (!val) return {};
    QString result = QString::fromUtf8(val);
    mpv_free(val);
    return result;
}

int64_t MpvWidget::getPropertyInt(const QString &name) const {
    int64_t val = 0;
    mpv_get_property(mpv, name.toUtf8().constData(), MPV_FORMAT_INT64, &val);
    return val;
}

void MpvWidget::loadFile(const QString &path) {
    command({"loadfile", path});
}

void MpvWidget::clearFrame() {
    makeCurrent();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    doneCurrent();
    update();
}