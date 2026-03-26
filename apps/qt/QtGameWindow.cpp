#include "QtGameWindow.h"

// Engine
#include "asset/MountEmbedded.h"
#include "event/DisconnectRequestEvent.h"
#include "event/EventManager.h"
#include "render/RenderManager.h"
#include "ui/Screen.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Qt app
#include "render/RenderBridge.h"

// Qt
#include <QAbstractEventDispatcher>
#include <QFontDatabase>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>

#if !defined(Q_OS_APPLE)
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#endif

#include <cstring>
#include <memory>

// Factory provided by the linked render plugin (aorender_gl or aorender_metal).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

// --------------------------------------------------------------------------

QtGameWindow::QtGameWindow(UIManager& uiMgr, StateBuffer& buffer,
                           int renderW, int renderH,
                           QObject* parent)
    : QObject(parent)
    , m_uiMgr(uiMgr)
    , m_buffer(buffer)
    , m_renderW(renderW)
    , m_renderH(renderH)
{
    m_window = new QQuickWindow();
    m_window->setTitle("Attorney Online");
    m_window->resize(1280, 720);

    connect(m_window, &QQuickWindow::sceneGraphInitialized,
            this, &QtGameWindow::onSceneGraphInitialized,
            Qt::DirectConnection);

    initGraphics();
    loadFonts();
    setupQml();
}

QtGameWindow::~QtGameWindow() {
    // Disconnect the screen-change hook before any teardown.
    if (auto* d = QAbstractEventDispatcher::instance(QThread::currentThread()))
        d->disconnect(this);

#if !defined(Q_OS_APPLE)
    if (m_glContext && m_glSurface) {
        m_glContext->makeCurrent(m_glSurface);
        m_renderManager.reset(); // destroy renderer while context is current
        m_glContext->doneCurrent();
    }
    // m_glSurface and m_glContext have 'this' as QObject parent — Qt deletes them.
#endif

    if (m_window)
        m_window->deleteLater();
}

void QtGameWindow::show() {
    m_window->show();

    // Hook into the event dispatcher to detect game-screen transitions.
    // Called on every event-loop wake-up with DirectConnection so it runs
    // inline, synchronously, on the main thread — no extra latency.
    auto* dispatcher = QAbstractEventDispatcher::instance(QThread::currentThread());
    if (dispatcher) {
        connect(dispatcher, &QAbstractEventDispatcher::awake,
                this, &QtGameWindow::onScreenChanged,
                Qt::DirectConnection);
    }
}

// --------------------------------------------------------------------------
// Public slots
// --------------------------------------------------------------------------

void QtGameWindow::onNavAction(IUIRenderer::NavAction action) {
    if (action == IUIRenderer::NavAction::POP_TO_ROOT) {
        // Signal the network layer to disconnect. Session cleanup (including
        // model resets) is triggered by the resulting SessionEndEvent.
        EventManager::instance()
            .get_channel<DisconnectRequestEvent>()
            .publish(DisconnectRequestEvent());
        m_uiMgr.pop_to_root();
    } else if (action == IUIRenderer::NavAction::POP_SCREEN) {
        m_uiMgr.pop_screen();
    }
}

// --------------------------------------------------------------------------
// Private slots
// --------------------------------------------------------------------------

void QtGameWindow::onSceneGraphInitialized() {
    const char* backend = m_renderManager
        ? m_renderManager->get_renderer().backend_name()
        : "(none)";
    Log::log_print(INFO, "QtGameWindow: scene graph initialised (%s)", backend);
}

void QtGameWindow::onScreenChanged() {
    Screen* screen  = m_uiMgr.active_screen();
    std::string newId = screen ? screen->screen_id() : std::string{};
    if (newId == m_activeScreenId)
        return;

    m_activeScreenId = newId;

    if (!m_rootItem || newId.empty())
        return;

    // Invoke navigateTo(id) on the QML root item so the StackView transitions
    // to the matching screen. The method is defined in Main.qml.
    QMetaObject::invokeMethod(m_rootItem, "navigateTo",
                              Qt::QueuedConnection,
                              Q_ARG(QVariant, QString::fromStdString(newId)));
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------

void QtGameWindow::initGraphics() {
#if !defined(Q_OS_APPLE)
    // ── OpenGL path ──────────────────────────────────────────────────────
    // Create a context + offscreen surface so the renderer can be
    // initialised (glewInit, FBO setup) before Qt's scene graph claims
    // the context. Qt adopts the context via QQuickGraphicsDevice.

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);

    m_glContext = new QOpenGLContext(this);
    m_glContext->setFormat(fmt);
    if (!m_glContext->create()) {
        Log::log_print(FATAL, "QtGameWindow: failed to create OpenGL 3.3 context");
        return;
    }

    m_glSurface = new QOffscreenSurface(nullptr, this);
    m_glSurface->setFormat(m_glContext->format());
    m_glSurface->create();

    if (!m_glContext->makeCurrent(m_glSurface)) {
        Log::log_print(FATAL, "QtGameWindow: failed to make GL context current");
        return;
    }

    // GLRenderer constructor calls glewInit() while a context is current.
    auto renderer = create_renderer(m_renderW, m_renderH);
    Log::log_print(INFO, "QtGameWindow: renderer created (%s)",
                   renderer->backend_name());

    m_renderManager = std::make_unique<RenderManager>(m_buffer, std::move(renderer));

    // Release the context so Qt's render thread can take ownership.
    m_glContext->doneCurrent();

    m_window->setGraphicsDevice(
        QQuickGraphicsDevice::fromOpenGLContext(m_glContext));

#else
    // ── Metal path ───────────────────────────────────────────────────────
    // MetalRenderer creates its own MTLDevice and command queue.
    // We hand those pointers to Qt so both the renderer and the scene
    // graph share the same device and can exchange textures directly.

    auto renderer = create_renderer(m_renderW, m_renderH);
    Log::log_print(INFO, "QtGameWindow: renderer created (%s)",
                   renderer->backend_name());

    void* device = renderer->get_device_ptr();
    void* queue  = renderer->get_command_queue_ptr();

    m_renderManager = std::make_unique<RenderManager>(m_buffer, std::move(renderer));

    m_window->setGraphicsDevice(
        QQuickGraphicsDevice::fromDeviceAndCommandQueue(device, queue));
#endif

    RenderBridge::instance().setRenderManager(m_renderManager.get());
}

void QtGameWindow::loadFonts() {
    // Register the bundled NotoEmoji font for monochrome emoji support in
    // QML text elements. The data pointer is stable for the process lifetime
    // (it lives in the binary's read-only data segment).
    for (const auto& file : embedded_assets()) {
        if (std::strcmp(file.path, "fonts/NotoEmoji.ttf") != 0)
            continue;
        QByteArray data(reinterpret_cast<const char*>(file.data),
                        static_cast<qsizetype>(file.size));
        int id = QFontDatabase::addApplicationFontFromData(data);
        if (id >= 0)
            Log::log_print(INFO, "QtGameWindow: registered NotoEmoji (%zu bytes)",
                           file.size);
        else
            Log::log_print(WARN, "QtGameWindow: failed to register NotoEmoji");
        break;
    }
}

void QtGameWindow::setupQml() {
    m_engine = new QQmlEngine(this);

    // Expose the render bridge so SceneTextureItem can reach RenderManager.
    m_engine->rootContext()->setContextProperty(
        QStringLiteral("renderBridge"), &RenderBridge::instance());

    // Additional context properties (controllers, models) are registered
    // by their respective commits; they are added here once implemented.

    m_component = new QQmlComponent(
        m_engine,
        QUrl(QStringLiteral("qrc:/qt/qml/AO/qml/Main.qml")),
        QQmlComponent::PreferSynchronous,
        this);

    if (m_component->isError()) {
        for (const QQmlError& err : m_component->errors())
            Log::log_print(ERR, "QtGameWindow QML error: %s",
                           err.toString().toUtf8().constData());
        return;
    }

    auto* obj    = m_component->create(m_engine->rootContext());
    m_rootItem   = qobject_cast<QQuickItem*>(obj);
    if (!m_rootItem) {
        Log::log_print(ERR, "QtGameWindow: Main.qml root is not a QQuickItem");
        delete obj;
        return;
    }

    m_rootItem->setParentItem(m_window->contentItem());
    m_rootItem->setWidth(m_window->width());
    m_rootItem->setHeight(m_window->height());

    // Keep the root item sized to the window as the user resizes it.
    connect(m_window, &QQuickWindow::widthChanged, m_rootItem, [this]() {
        m_rootItem->setWidth(m_window->width());
    });
    connect(m_window, &QQuickWindow::heightChanged, m_rootItem, [this]() {
        m_rootItem->setHeight(m_window->height());
    });
}
