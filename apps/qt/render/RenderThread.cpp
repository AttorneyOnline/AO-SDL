#include "RenderThread.h"

#include "RenderBridge.h"

#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "utils/Log.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QQuickGraphicsDevice>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QWindow>

// Factory provided by the linked render plugin (aorender_gl).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

// --------------------------------------------------------------------------

RenderThread::RenderThread(QQuickRenderControl* rc,
                           QQuickWindow*        quickWindow,
                           QWindow*             displayWindow,
                           QOffscreenSurface*   surface,
                           QOpenGLContext*       glContext,
                           StateBuffer&         buffer,
                           int renderW, int renderH)
    : QThread(nullptr)
    , m_rc(rc)
    , m_quickWindow(quickWindow)
    , m_displayWindow(displayWindow)
    , m_surface(surface)
    , m_buffer(buffer)
    , m_renderW(renderW)
    , m_renderH(renderH)
    , m_glContext(glContext)
{}

RenderThread::~RenderThread() {
    requestStop();
    wait();
}

// --------------------------------------------------------------------------
// Main-thread API
// --------------------------------------------------------------------------

void RenderThread::requestSync() {
    QMutexLocker locker(&m_mutex);
    m_syncRequested = true;
    m_condSync.wakeOne();
    // Block until the render thread finishes sync().
    m_condMain.wait(&m_mutex);
}

void RenderThread::requestStop() {
    QMutexLocker locker(&m_mutex);
    m_exiting = true;
    m_condSync.wakeOne();
}

void RenderThread::waitForInit() {
    QMutexLocker locker(&m_initMutex);
    while (!m_initialized)
        m_initCond.wait(&m_initMutex);
}

// --------------------------------------------------------------------------
// Thread entry point
// --------------------------------------------------------------------------

void RenderThread::run() {
    initializeGL();

    {
        QMutexLocker locker(&m_initMutex);
        m_initialized = true;
        m_initCond.wakeOne();
    }

    // ── render loop ──────────────────────────────────────────────────────
    while (true) {
        m_mutex.lock();
        while (!m_syncRequested && !m_exiting)
            m_condSync.wait(&m_mutex);

        if (m_exiting) {
            m_mutex.unlock();
            break;
        }
        m_syncRequested = false;

        m_glContext->makeCurrent(m_surface);

        // Recreate the render target when the display window is resized.
        QSize winSize = m_displayWindow->size()
                        * m_displayWindow->devicePixelRatio();
        if (winSize != m_rtSize
            && winSize.width() > 0 && winSize.height() > 0) {
            createRenderTarget(winSize);
        }

        // Game renderer draws to its own FBO → texture is ready for
        // SceneTextureItem::updatePaintNode() which runs inside sync().
        RenderBridge::instance().renderFrame();

        // Qt 6 RHI requires beginFrame() before sync().
        m_rc->beginFrame();

        // Sync the QML scene graph.  The main thread is blocked for
        // exactly this call, guaranteeing the item tree is stable.
        m_rc->sync();

        // Unblock the main thread — we no longer touch the item tree.
        m_condMain.wakeOne();
        m_mutex.unlock();

        // Render phase — main thread is free to process events.
        m_rc->render();
        m_rc->endFrame();

        blitToDisplay();

        m_glContext->doneCurrent();
    }

    // ── cleanup (context must be current) ────────────────────────────────
    m_glContext->makeCurrent(m_surface);
    cleanup();
    m_glContext->doneCurrent();
}

// --------------------------------------------------------------------------
// GL initialisation (runs on the render thread)
// --------------------------------------------------------------------------

void RenderThread::initializeRhi(const QSize& initialSize) {
    // Called from the MAIN thread before the GL context is moved.
    // Qt's RHI initialisation creates platform surfaces that require
    // the GUI thread.
    m_glContext->makeCurrent(m_surface);
    m_gl = m_glContext->extraFunctions();

    m_quickWindow->setGraphicsDevice(
        QQuickGraphicsDevice::fromOpenGLContext(m_glContext));

    createRenderTarget(initialSize);

    if (!m_rc->initialize())
        Log::log_print(FATAL, "RenderThread: QQuickRenderControl::initialize failed");

    m_glContext->doneCurrent();
}

void RenderThread::initializeGL() {
    // The GL context was created on the main thread, RHI was initialised
    // there, then the context was moved to this thread.
    if (!m_glContext->makeCurrent(m_surface)) {
        Log::log_print(FATAL, "RenderThread: failed to make GL context current");
        return;
    }

    // GLRenderer constructor calls glewInit() while a context is current.
    auto renderer = create_renderer(m_renderW, m_renderH);
    Log::log_print(INFO, "RenderThread: renderer created (%s)",
                   renderer->backend_name());

    m_renderManager =
        std::make_unique<RenderManager>(m_buffer, std::move(renderer));

    RenderBridge::instance().setRenderManager(
        m_renderManager.get(), m_renderW, m_renderH);

    m_glContext->doneCurrent();
}

// --------------------------------------------------------------------------
// Render target (offscreen FBO that QQuickRenderControl draws into)
// --------------------------------------------------------------------------

void RenderThread::createRenderTarget(const QSize& size) {
    destroyRenderTarget();

    m_gl->glGenTextures(1, &m_rtTexture);
    m_gl->glBindTexture(GL_TEXTURE_2D, m_rtTexture);
    m_gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                       size.width(), size.height(), 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_gl->glBindTexture(GL_TEXTURE_2D, 0);

    m_gl->glGenFramebuffers(1, &m_rtFbo);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, m_rtFbo);
    m_gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, m_rtTexture, 0);
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_rtSize = size;

    m_quickWindow->setRenderTarget(
        QQuickRenderTarget::fromOpenGLTexture(m_rtTexture, size));
}

void RenderThread::destroyRenderTarget() {
    if (m_rtFbo) {
        m_gl->glDeleteFramebuffers(1, &m_rtFbo);
        m_rtFbo = 0;
    }
    if (m_rtTexture) {
        m_gl->glDeleteTextures(1, &m_rtTexture);
        m_rtTexture = 0;
    }
}

// --------------------------------------------------------------------------
// Blit the offscreen render-target to the on-screen display window.
// --------------------------------------------------------------------------

void RenderThread::blitToDisplay() {
    m_glContext->makeCurrent(m_displayWindow);

    int dw = static_cast<int>(m_displayWindow->width()
                              * m_displayWindow->devicePixelRatio());
    int dh = static_cast<int>(m_displayWindow->height()
                              * m_displayWindow->devicePixelRatio());

    m_gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rtFbo);
    m_gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    m_gl->glBlitFramebuffer(0, 0, m_rtSize.width(), m_rtSize.height(),
                            0, 0, dw, dh,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);

    m_glContext->swapBuffers(m_displayWindow);
}

// --------------------------------------------------------------------------

void RenderThread::cleanup() {
    destroyRenderTarget();
    RenderBridge::instance().setRenderManager(nullptr, 0, 0);
    m_renderManager.reset();   // destroy renderer while context is current
    delete m_glContext;
    m_glContext = nullptr;
}
