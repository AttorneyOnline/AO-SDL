#include "IGPUBackend.h"
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

    void init_qml(QQuickWindow* window, IRenderer& /*renderer*/) override {
        QObject::connect(
            window_, &QQuickWindow::beforeRendering, window_, [this]() { begin_frame(); }, Qt::DirectConnection);

        QObject::connect(
            window_, &QQuickWindow::afterRendering, window_, [this]() { present(); }, Qt::DirectConnection);
    }

    void shutdown() override {
        if (window_)
            window_->disconnect();
    }

    void begin_frame() override {
        window_->beginExternalCommands();
    }

    void present() override {
        window_->endExternalCommands();
        window_->update();
    }

  private:
    QQuickWindow* window_ = nullptr;
    QOpenGLContext* gl_context_ = nullptr;
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
