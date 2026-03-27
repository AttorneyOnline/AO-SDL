#include "SceneTextureItem.h"

#include "RenderBridge.h"
#include "asset/MediaManager.h"
#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <rhi/qrhi.h>

// Factory defined in whichever render plugin is linked (aorender_gl).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

// --------------------------------------------------------------------------
// Construction / destruction
// --------------------------------------------------------------------------

SceneTextureItem::SceneTextureItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged,
            this, &SceneTextureItem::handleWindowChanged);
}

SceneTextureItem::~SceneTextureItem() {
    // Normal path: cleanupGL() was already invoked via sceneGraphInvalidated.
    delete m_rhiTexture;
}

// --------------------------------------------------------------------------
// Window / scene-graph lifecycle
// --------------------------------------------------------------------------

void SceneTextureItem::handleWindowChanged(QQuickWindow* win) {
    if (!win)
        return;

    // DirectConnection — these slots fire on the render thread.
    connect(win, &QQuickWindow::beforeRendering,
            this, &SceneTextureItem::renderGL,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated,
            this, &SceneTextureItem::handleSceneGraphInvalidated,
            Qt::DirectConnection);
}

void SceneTextureItem::handleSceneGraphInvalidated() {
    cleanupGL();
}

// --------------------------------------------------------------------------
// GL initialisation — runs once on the render thread
// --------------------------------------------------------------------------

void SceneTextureItem::initGL() {
    if (!QOpenGLContext::currentContext())
        return;

    RenderBridge& rb = RenderBridge::instance();

    if (!rb.stateBuffer()) {
        qWarning("SceneTextureItem::initGL: RenderBridge has no StateBuffer — "
                 "call RenderBridge::setStateBuffer() before showing the window");
        return;
    }

    auto renderer = create_renderer(rb.renderWidth(), rb.renderHeight());
    if (!renderer) {
        qWarning("SceneTextureItem::initGL: create_renderer() returned null");
        return;
    }

    // Tell the asset library which shader sub-directory to use.
    MediaManager::instance().assets()
        .set_shader_backend(renderer->backend_name());

    m_renderManager = std::make_unique<RenderManager>(
        *rb.stateBuffer(), std::move(renderer));

    rb.setRenderManager(m_renderManager.get(),
                        rb.renderWidth(), rb.renderHeight());

    m_glInitialized = true;
}

// --------------------------------------------------------------------------
// Per-frame rendering — render thread, via beforeRendering
// --------------------------------------------------------------------------

void SceneTextureItem::renderGL() {
    if (!m_glInitialized)
        initGL();
    if (!m_glInitialized)
        return;

    window()->beginExternalCommands();

    RenderBridge::instance().renderFrame();

    // GLRenderer::draw() leaves its own FBO bound; restore the default so
    // Qt's RHI can render the scene graph on top.
    QOpenGLContext::currentContext()
        ->extraFunctions()
        ->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    window()->endExternalCommands();
}

// --------------------------------------------------------------------------
// Scene-graph texture node — render thread, during sync
// --------------------------------------------------------------------------

QSGNode* SceneTextureItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    uintptr_t texId = RenderBridge::instance().nativeTextureId();
    if (!texId) {
        // Renderer not ready yet — will be created on the next beforeRendering.
        update();
        return old;
    }

    auto* node = static_cast<QSGSimpleTextureNode*>(old);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setOwnsTexture(true);
    }

    // Wrap the native GL texture once (the renderer's FBO texture ID is
    // stable for its lifetime — it only changes on a resize()).
    if (texId != m_cachedTexId) {
        auto* ri  = window()->rendererInterface();
        auto* rhi = static_cast<QRhi*>(
            ri->getResource(window(), QSGRendererInterface::RhiResource));

        if (rhi) {
            delete m_rhiTexture;
            m_rhiTexture = nullptr;

            auto* rhiTex = rhi->newTexture(
                QRhiTexture::RGBA8,
                QSize(RenderBridge::instance().renderWidth(),
                      RenderBridge::instance().renderHeight()));
            if (rhiTex) {
                rhiTex->createFrom({static_cast<quint64>(texId), 0});
                m_rhiTexture  = rhiTex;
                m_cachedTexId = texId;
                node->setTexture(
                    window()->createTextureFromRhiTexture(rhiTex));
            }
        }
    }

    // GL FBOs have V=0 at the bottom; flip to match QML's top-down coordinates.
    if (RenderBridge::instance().uvFlipped())
        node->setTextureCoordinatesTransform(
            QSGSimpleTextureNode::MirrorVertically);

    node->setRect(boundingRect());

    // Keep the render loop alive.
    update();
    return node;
}

// --------------------------------------------------------------------------
// Cleanup — render thread
// --------------------------------------------------------------------------

void SceneTextureItem::cleanupGL() {
    if (!m_glInitialized)
        return;

    // Unregister from the bridge before tearing down the GL resources it
    // references, so no in-flight renderFrame() call can dereference them.
    RenderBridge::instance().setRenderManager(nullptr, 0, 0);

    // RenderManager destructor destroys the IRenderer, which calls glDelete*
    // — the GL context must be current at this point (guaranteed by
    // sceneGraphInvalidated on Qt's render thread).
    m_renderManager.reset();

    delete m_rhiTexture;
    m_rhiTexture  = nullptr;
    m_cachedTexId = 0;

    m_glInitialized = false;
}
