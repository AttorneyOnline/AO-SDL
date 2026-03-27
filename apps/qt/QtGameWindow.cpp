#include "QtGameWindow.h"

// Engine
#include "asset/MountEmbedded.h"
#include "event/DisconnectRequestEvent.h"
#include "event/EventManager.h"
#include "ui/Screen.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

// Qt app
#include "AppContext.h"
#include "render/RenderBridge.h"
#include "render/RenderThread.h"

// Qt
#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QThread>
#include <QTimer>
#include <QWindow>

#include <cstring>

// --------------------------------------------------------------------------
// QQuickRenderControl subclass — returns the real display window so that
// QML coordinate mapping (mapToGlobal, popup placement, etc.) works.
// --------------------------------------------------------------------------

class DisplayRenderControl : public QQuickRenderControl {
  public:
    explicit DisplayRenderControl(QWindow* displayWindow, QObject* parent = nullptr)
        : QQuickRenderControl(parent), m_displayWindow(displayWindow) {}

    QWindow* renderWindow(QPoint* offset) override {
        if (offset)
            *offset = QPoint(0, 0);
        return m_displayWindow;
    }

  private:
    QWindow* m_displayWindow;
};

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
#if !defined(Q_OS_APPLE)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif

    initDisplay();
    initRenderControl();
    loadFonts();
    setupQml();

    // Create the render thread object (not yet started).
    m_renderThread = new RenderThread(
        m_renderControl, m_quickWindow, m_displayWindow,
        m_glSurface, m_glContext, m_buffer, m_renderW, m_renderH);

    // RHI initialisation must happen on the main (GUI) thread because Qt
    // creates platform surfaces internally.  The GL context is still on
    // this thread at this point.
    QSize initialSize = m_displayWindow->size()
                        * m_displayWindow->devicePixelRatio();
    if (initialSize.width() <= 0 || initialSize.height() <= 0)
        initialSize = QSize(1280, 720);
    m_renderThread->initializeRhi(initialSize);

    // Now hand the context to the render thread.
    m_renderControl->prepareThread(m_renderThread);
    m_glContext->moveToThread(m_renderThread);
    m_renderThread->start();
    m_renderThread->waitForInit();

    Log::log_print(INFO, "QtGameWindow: render thread started");

    // Frame timer — drives polish → sync → render at the display's cadence.
    // The actual rate is limited by swapBuffers (vsync) in the render thread.
    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(1);
    connect(m_frameTimer, &QTimer::timeout, this, &QtGameWindow::triggerSync);
}

QtGameWindow::~QtGameWindow() {
    m_frameTimer->stop();

    // Stop and join the render thread (destroys GL resources internally,
    // including m_glContext which was moved to the render thread).
    delete m_renderThread;
    m_renderThread = nullptr;
    m_glContext = nullptr;  // destroyed by RenderThread::cleanup()

    delete m_quickWindow;
    m_quickWindow = nullptr;

    delete m_renderControl;
    m_renderControl = nullptr;

    delete m_displayWindow;
    m_displayWindow = nullptr;

    delete m_glSurface;
    m_glSurface = nullptr;
}

void QtGameWindow::show() {
    m_displayWindow->show();
    m_frameTimer->start();
}

// --------------------------------------------------------------------------
// Public slots
// --------------------------------------------------------------------------

void QtGameWindow::onNavAction(IUIRenderer::NavAction action) {
    if (action == IUIRenderer::NavAction::POP_TO_ROOT) {
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

void QtGameWindow::onScreenChanged() {
    Screen* screen = m_uiMgr.active_screen();
    std::string newId = screen ? screen->screen_id() : std::string{};
    if (newId == m_activeScreenId)
        return;

    m_activeScreenId = newId;

    if (!m_rootItem || newId.empty())
        return;

    QMetaObject::invokeMethod(m_rootItem, "navigateTo",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QString::fromStdString(newId)));
}

void QtGameWindow::triggerSync() {
    // Polish any pending item geometry / animations on the main thread.
    m_renderControl->polishItems();

    // Propagate display-window size to the offscreen QQuickWindow so QML
    // layout matches the actual display area.
    QSize sz = m_displayWindow->size();
    if (QSize(m_quickWindow->width(), m_quickWindow->height()) != sz) {
        m_quickWindow->setGeometry(0, 0, sz.width(), sz.height());
        if (m_rootItem) {
            m_rootItem->setWidth(sz.width());
            m_rootItem->setHeight(sz.height());
        }
    }

    // Blocks the main thread only for the duration of sync() (~0.1 ms).
    m_renderThread->requestSync();
}

// --------------------------------------------------------------------------
// Event filter — forward input from the display window to the QQuickWindow
// --------------------------------------------------------------------------

bool QtGameWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_displayWindow) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Enter:
        case QEvent::Leave:
            QCoreApplication::sendEvent(m_quickWindow, event);
            return true;

        case QEvent::FocusIn:
            QCoreApplication::sendEvent(m_quickWindow, event);
            if (m_quickWindow->contentItem())
                m_quickWindow->contentItem()->forceActiveFocus();
            return true;

        case QEvent::FocusOut:
            QCoreApplication::sendEvent(m_quickWindow, event);
            return true;

        case QEvent::Resize:
            // Handled in triggerSync(); just let the offscreen window know.
            QCoreApplication::sendEvent(m_quickWindow, event);
            break;

        default:
            break;
        }
    }
    return QObject::eventFilter(obj, event);
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------

void QtGameWindow::initDisplay() {
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    // On-screen window.
    m_displayWindow = new QWindow();
    m_displayWindow->setSurfaceType(QSurface::OpenGLSurface);
    m_displayWindow->setFormat(fmt);
    m_displayWindow->setTitle("Attorney Online");
    m_displayWindow->resize(1280, 720);
    m_displayWindow->installEventFilter(this);

    // Offscreen surface for the render thread's GL context.
    // Must be created on the main thread (platform requirement).
    m_glSurface = new QOffscreenSurface();
    m_glSurface->setFormat(fmt);
    m_glSurface->create();

    // Create the GL context on the main thread (required by Qt).
    // It will be moved to the render thread before start().
    m_glContext = new QOpenGLContext();
    m_glContext->setFormat(fmt);
    if (!m_glContext->create())
        Log::log_print(FATAL, "QtGameWindow: failed to create GL 3.3 context");
}

void QtGameWindow::initRenderControl() {
    m_renderControl = new DisplayRenderControl(m_displayWindow, this);
    m_quickWindow   = new QQuickWindow(m_renderControl);

    // Size the offscreen window to match the display window.
    m_quickWindow->setGeometry(0, 0,
                               m_displayWindow->width(),
                               m_displayWindow->height());
}

void QtGameWindow::loadFonts() {
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
            Log::log_print(WARNING, "QtGameWindow: failed to register NotoEmoji");
        break;
    }
}

void QtGameWindow::setupQml() {
    m_engine = new QQmlEngine(this);

    m_engine->rootContext()->setContextProperty(
        QStringLiteral("renderBridge"), &RenderBridge::instance());

    m_engine->rootContext()->setContextProperty(
        QStringLiteral("app"), &AppContext::instance());

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

    auto* obj  = m_component->create(m_engine->rootContext());
    m_rootItem = qobject_cast<QQuickItem*>(obj);
    if (!m_rootItem) {
        Log::log_print(ERR, "QtGameWindow: Main.qml root is not a QQuickItem");
        delete obj;
        return;
    }

    m_rootItem->setParentItem(m_quickWindow->contentItem());
    m_rootItem->setWidth(m_quickWindow->width());
    m_rootItem->setHeight(m_quickWindow->height());
}
