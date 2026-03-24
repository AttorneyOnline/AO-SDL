#include "qtgamewindow.h"

#include "CharIconProvider.h"
#include "EmoteIconProvider.h"
#include "GameViewport.h"
#include "QmlUIBridge.h"
#include "asset/MediaManager.h"
#include "render/RenderManager.h"
#include "utils/Log.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>

QtGameWindow::QtGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend)
    : window_(nullptr), ui_manager_(ui_manager), gpu_(std::move(backend)) {

    window_ = new QQuickView(nullptr);
    if (window_->status() == QQuickView::Status::Error) {
        QString error_string;
        for (const auto& error : window_->errors())
            error_string.append(error.toString());
        Log::log_print(LogLevel::FATAL, "Failed to create QQuickView: %s", error_string.toStdString().c_str());
    }

    gpu_->create_context(window_);

    window_->setResizeMode(QQuickView::SizeRootObjectToView);
    window_->setTitle("AO-SDL (Qt)");
    window_->resize(1280, 720);
}

QtGameWindow::~QtGameWindow() {
    gpu_->shutdown();
    delete window_;
}

void QtGameWindow::start_rendering(RendererFactory factory, QmlUIBridge& bridge, int render_w, int render_h) {
    // Expose the bridge to QML before loading the source.
    window_->rootContext()->setContextProperty("uiBridge", &bridge);

    // Image providers — Qt takes ownership of the provider instances.
    // Character icons: QML uses "image://charicon/<index>"
    window_->engine()->addImageProvider("charicon", new CharIconProvider(*bridge.char_list_model()));
    // Emote icons: QML uses "image://emoteicon/<index>"
    window_->engine()->addImageProvider("emoteicon", new EmoteIconProvider(bridge));

    // Load the QML UI.
    window_->setSource(QUrl("qrc:/qml/main.qml"));
    if (window_->status() == QQuickView::Status::Error) {
        for (const auto& error : window_->errors())
            Log::log_print(LogLevel::ERR, "QML error: %s", error.toString().toStdString().c_str());
        return;
    }

    // Sync engine state → QML properties after each frame completes.
    // frameSwapped fires on the GUI thread after the frame is fully rendered,
    // so Q_PROPERTY notifications and model resets propagate normally to QML.
    QObject::connect(window_, &QQuickWindow::frameSwapped, &bridge, [&bridge]() {
        bridge.sync_from_engine();
    });

    // Defer renderer creation until the GL context is ready.
    // sceneGraphInitialized fires once, after the first show(), when the
    // context is current — safe to call glewInit/glCreateProgram/etc.
    QObject::connect(
        window_, &QQuickWindow::sceneGraphInitialized, window_,
        [this, factory = std::move(factory), &bridge, &render_h, &render_w]() {
            renderer_ = factory();
            Log::log_print(INFO, "QtGameWindow: renderer created (sceneGraphInitialized)");

            MediaManager::instance().assets().set_shader_backend(
                renderer_->get_renderer().backend_name());

            // Give the GameViewport a pointer to the render manager and texture size.
            if (auto* viewport = window_->rootObject()->findChild<GameViewport*>("gameViewport")) {
                if (render_w > 0 && render_h > 0)
                    viewport->set_render_size(render_w, render_h);
                viewport->set_render_manager(renderer_.get());
            }

            // Wire the GL backend: frame callback + game render + FB reset run
            // inside beginExternalCommands/endExternalCommands each frame.
            gpu_->init_qml(window_, renderer_->get_renderer(), [this]() {
                if (frame_callback_)
                    frame_callback_();
                ui_manager_.handle_events();
                renderer_->render_frame();
                renderer_->begin_frame(); // Rebind default FB so Qt's scene graph can draw.
            });
        },
        Qt::DirectConnection);

    window_->show();
}
