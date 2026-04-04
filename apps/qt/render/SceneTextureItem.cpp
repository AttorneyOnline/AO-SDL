#include "SceneTextureItem.h"

#include "IQtRenderBackend.h"
#include "RenderBridge.h"
#include "render/IRenderer.h"
#include "render/RenderManager.h"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <rhi/qrhi.h>

// Factory defined in whichever render plugin is linked
// (aorender_metal on Apple, aorender_gl elsewhere).
std::unique_ptr<IRenderer> create_renderer(int width, int height);

// --------------------------------------------------------------------------
// Construction / destruction
// --------------------------------------------------------------------------

SceneTextureItem::SceneTextureItem(QQuickItem* parent) : QQuickItem(parent), m_backend(create_qt_render_backend()) {
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged, this, &SceneTextureItem::handleWindowChanged);
}

SceneTextureItem::~SceneTextureItem() {
    // Normal path: cleanup() was already invoked via sceneGraphInvalidated.
    delete m_rhiTexture;
}

// --------------------------------------------------------------------------
// Window / scene-graph lifecycle
// --------------------------------------------------------------------------

void SceneTextureItem::handleWindowChanged(QQuickWindow* win) {
    if (!win)
        return;

    // DirectConnection — these slots fire on the render thread.
    connect(win, &QQuickWindow::beforeRendering, this, &SceneTextureItem::render, Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &SceneTextureItem::handleSceneGraphInvalidated,
            Qt::DirectConnection);
}

void SceneTextureItem::handleSceneGraphInvalidated() {
    cleanup();
}

// --------------------------------------------------------------------------
// Renderer initialisation — runs once on the render thread
// --------------------------------------------------------------------------

void SceneTextureItem::initRenderer() {
    if (!m_backend->isContextReady())
        return;

    RenderBridge& rb = RenderBridge::instance();

    if (!rb.stateBuffer()) {
        qWarning("SceneTextureItem::initRenderer: RenderBridge has no StateBuffer — "
                 "call RenderBridge::setStateBuffer() before showing the window");
        return;
    }

    auto renderer = create_renderer(rb.renderWidth(), rb.renderHeight());
    if (!renderer) {
        qWarning("SceneTextureItem::initRenderer: create_renderer() returned null");
        return;
    }

    m_renderManager = std::make_unique<RenderManager>(*rb.stateBuffer(), std::move(renderer));

    rb.setRenderManager(m_renderManager.get(), rb.renderWidth(), rb.renderHeight());

    m_initialized = true;
}

// --------------------------------------------------------------------------
// Per-frame rendering — render thread, via beforeRendering
// --------------------------------------------------------------------------

void SceneTextureItem::render() {
    if (!m_initialized)
        initRenderer();
    if (!m_initialized)
        return;

    window()->beginExternalCommands();

    RenderBridge::instance().renderFrame();
    m_backend->restoreState();

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

    // Wrap the native texture once (the renderer's offscreen texture ID is
    // stable for its lifetime — it only changes on a resize()).
    if (texId != m_cachedTexId) {
        auto* ri = window()->rendererInterface();
        auto* rhi = static_cast<QRhi*>(ri->getResource(window(), QSGRendererInterface::RhiResource));

        if (rhi) {
            delete m_rhiTexture;
            m_rhiTexture = nullptr;

            auto* rhiTex = rhi->newTexture(m_backend->textureFormat(), QSize(RenderBridge::instance().renderWidth(),
                                                                             RenderBridge::instance().renderHeight()));
            if (rhiTex) {
                rhiTex->createFrom({static_cast<quint64>(texId), 0});
                m_rhiTexture = rhiTex;
                m_cachedTexId = texId;
                node->setTexture(window()->createTextureFromRhiTexture(rhiTex));
            }
        }
    }

    if (RenderBridge::instance().uvFlipped())
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);

    node->setRect(boundingRect());

    // Keep the render loop alive.
    update();
    return node;
}

// --------------------------------------------------------------------------
// Cleanup — render thread
// --------------------------------------------------------------------------

void SceneTextureItem::cleanup() {
    if (!m_initialized)
        return;

    // Unregister from the bridge before tearing down the resources it
    // references, so no in-flight renderFrame() call can dereference them.
    RenderBridge::instance().setRenderManager(nullptr, 0, 0);

    m_renderManager.reset();

    delete m_rhiTexture;
    m_rhiTexture = nullptr;
    m_cachedTexId = 0;

    m_initialized = false;
}
