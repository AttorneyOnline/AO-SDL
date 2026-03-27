#pragma once

#include "ui/IUIRenderer.h"

#include <QObject>
#include <QSize>

#include <string>

class QOffscreenSurface;
class QOpenGLContext;
class QQmlComponent;
class QQmlEngine;
class QQuickItem;
class QQuickRenderControl;
class QQuickWindow;
class QTimer;
class QWindow;
class RenderThread;
class StateBuffer;
class UIManager;

/**
 * @brief Qt/QML equivalent of SDLGameWindow.
 *
 * Uses QQuickRenderControl so that the QML scene graph and the game renderer
 * share a single OpenGL context on a dedicated RenderThread.  The main thread
 * runs the Qt event loop, QML engine, and EngineEventBridge; it blocks only
 * for the brief sync() phase each frame.
 *
 * A plain QWindow is used as the on-screen display surface.  The render
 * thread blits the composited result from the offscreen render target into
 * the display window's default framebuffer each frame.
 */
class QtGameWindow : public QObject {
    Q_OBJECT

  public:
    QtGameWindow(UIManager& uiMgr, StateBuffer& buffer,
                 int renderW, int renderH,
                 QObject* parent = nullptr);
    ~QtGameWindow() override;

    void show();

    /// Check UIManager for screen transitions and update QML navigation.
    /// Called from the event bridge after handle_events().
    void onScreenChanged();

  public slots:
    void onNavAction(IUIRenderer::NavAction action);

  private slots:
    void triggerSync();

  private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    void initDisplay();
    void initRenderControl();
    void loadFonts();
    void setupQml();

    UIManager&    m_uiMgr;
    StateBuffer&  m_buffer;
    int           m_renderW;
    int           m_renderH;

    // On-screen display window (receives input, shows final composited frame).
    QWindow* m_displayWindow = nullptr;

    // Offscreen QQuickWindow driven by QQuickRenderControl.
    QQuickRenderControl* m_renderControl = nullptr;
    QQuickWindow*        m_quickWindow   = nullptr;

    // QML engine + loaded component.
    QQmlEngine*    m_engine    = nullptr;
    QQmlComponent* m_component = nullptr;
    QQuickItem*    m_rootItem  = nullptr;

    // GL context created on the main thread, moved to the render thread.
    // Ownership transfers to RenderThread (cleaned up in its destructor).
    QOpenGLContext* m_glContext = nullptr;

    // Offscreen surface created on the main thread (platform requirement),
    // used by the render thread for GL makeCurrent.
    QOffscreenSurface* m_glSurface = nullptr;

    // Dedicated render thread.
    RenderThread* m_renderThread = nullptr;

    // Drives the sync+render cycle from the main thread.
    QTimer* m_frameTimer = nullptr;

    // Screen transition detection.
    std::string m_activeScreenId;
};
