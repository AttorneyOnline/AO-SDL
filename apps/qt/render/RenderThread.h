#pragma once

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>

#include <memory>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLExtraFunctions;
class QQuickRenderControl;
class QQuickWindow;
class QWindow;
class RenderManager;
class StateBuffer;

/**
 * @brief Dedicated render thread for the Qt frontend.
 *
 * Owns the OpenGL context, the game IRenderer (via RenderManager), and drives
 * QQuickRenderControl::sync()/render() on every frame.  The main thread
 * remains free to process events and update QML models; it blocks only during
 * the brief sync() phase.
 *
 * Lifecycle (all called from the main thread):
 *   1. Construct, passing the render-control and offscreen QQuickWindow.
 *   2. Call start() — the thread creates the GL context, renderer, and
 *      render target, then signals readiness.
 *   3. Call waitForInit() to block until GL is ready.
 *   4. Repeatedly call requestSync() to drive frames.
 *   5. Call requestStop() + wait() to shut down.
 */
class RenderThread : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(RenderThread)

  public:
    RenderThread(QQuickRenderControl* rc,
                 QQuickWindow*        quickWindow,
                 QWindow*             displayWindow,
                 QOffscreenSurface*   surface,
                 QOpenGLContext*       glContext,
                 StateBuffer&         buffer,
                 int renderW, int renderH);
    ~RenderThread() override;

    /// Block the calling (main) thread during sync, then return.
    /// The render thread continues with render + blit independently.
    void requestSync();

    /// Signal the thread to exit after the current frame.
    void requestStop();

    /// Block until the GL context and renderer are initialised.
    void waitForInit();

    /// Initialise the RHI (render target + QQuickRenderControl::initialize).
    /// Must be called on the MAIN thread before moveToThread / start().
    void initializeRhi(const QSize& initialSize);

  protected:
    void run() override;

  private:
    void initializeGL();
    void createRenderTarget(const QSize& size);
    void destroyRenderTarget();
    void blitToDisplay();
    void cleanup();

    // Owned by QtGameWindow — we only borrow pointers.
    QQuickRenderControl* m_rc;
    QQuickWindow*        m_quickWindow;
    QWindow*             m_displayWindow;
    QOffscreenSurface*   m_surface;
    StateBuffer&         m_buffer;
    int                  m_renderW;
    int                  m_renderH;

    // Created on the render thread in initializeGL().
    QOpenGLContext*         m_glContext = nullptr;
    QOpenGLExtraFunctions* m_gl        = nullptr;
    std::unique_ptr<RenderManager> m_renderManager;

    // Render-target FBO that QQuickRenderControl renders into.
    uint  m_rtTexture = 0;
    uint  m_rtFbo     = 0;
    QSize m_rtSize;

    // Main <-> render synchronisation.
    QMutex         m_mutex;
    QWaitCondition m_condSync;   // render thread waits here for work
    QWaitCondition m_condMain;   // main thread waits here during sync
    bool           m_syncRequested = false;
    bool           m_exiting       = false;

    // One-shot init gate.
    QMutex         m_initMutex;
    QWaitCondition m_initCond;
    bool           m_initialized = false;
};
