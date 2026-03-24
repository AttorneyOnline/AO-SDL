#include "IGPUBackend.h"

#include <QMetaEnum>
#include <QOpenGLFunctions>
#include <QSGRendererInterface>
#include <QtQuick/QQuickWindow>

#include "render/IRenderer.h"
#include "utils/Log.h"

class GLBackend : public IGPUBackend {
  public:
    uint32_t window_flags() const override {
        return static_cast<uint32_t>(window_->graphicsApi());
    }

    void create_context(QQuickWindow* window) override {
        window_ = window;
        QObject::connect(
            window_, &QQuickWindow::sceneGraphInitialized, window_,
            [this]() {
                QSGRendererInterface* rif = window_->rendererInterface();
                gl_context_ = reinterpret_cast<QOpenGLContext*>(
                    rif->getResource(window_, QSGRendererInterface::OpenGLContextResource));

                const QSurfaceFormat fmt = gl_context_->format();
                const char* profile =
                    QMetaEnum::fromType<QSurfaceFormat::OpenGLContextProfile>().valueToKey(fmt.profile());

                Log::log_print(DEBUG, "Obtained GLContext from QQuickView SceneGraph: OpenGL %i.%i - %s",
                               fmt.majorVersion(), fmt.minorVersion(), profile);
            },
            Qt::DirectConnection);
    }

    void init_qml(QQuickWindow* window, IRenderer& /*renderer*/,
                   std::function<void()> render_cb) override {
        render_cb_ = std::move(render_cb);

        QObject::connect(
            window_, &QQuickWindow::beforeRendering, window_,
            [this]() {
                window_->beginExternalCommands();
                if (render_cb_)
                    render_cb_();
                window_->endExternalCommands();
            },
            Qt::DirectConnection);

        QObject::connect(
            window_, &QQuickWindow::afterRendering, window_,
            [this]() { window_->update(); },
            Qt::DirectConnection);
    }

    void shutdown() override {
        if (window_)
            window_->disconnect();
    }

    void begin_frame() override {
        // No-op: external commands are bracketed in the beforeRendering callback.
    }

    void present() override {
        // No-op: Qt's scene graph handles the present/swap.
    }

  private:
    QQuickWindow* window_ = nullptr;
    QOpenGLContext* gl_context_ = nullptr;
    std::function<void()> render_cb_;
};

void IGPUBackend::pre_init() {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

std::unique_ptr<IGPUBackend> create_gpu_backend() {
    return std::make_unique<GLBackend>();
}
