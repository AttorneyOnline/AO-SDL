#include "SceneTextureItem.h"

#include "IQtRenderBackend.h"
#include "RenderBridge.h"
#include "render/IRenderer.h"
#include "render/RenderManager.h"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <rhi/qrhi.h>

std::unique_ptr<IRenderer> create_renderer(int width, int height);

SceneTextureItem::SceneTextureItem(QQuickItem* parent) : QQuickItem(parent), m_backend(create_qt_render_backend()) {
    setFlag(ItemHasContents, true);
    connect(this, &QQuickItem::windowChanged, this, &SceneTextureItem::handleWindowChanged);
}

SceneTextureItem::~SceneTextureItem() {
    // m_rhiTexture is owned by Qt via createTextureFromRhiTexture; nothing to free here.
}

void SceneTextureItem::handleWindowChanged(QQuickWindow* win) {
    if (!win)
        return;
    connect(win, &QQuickWindow::beforeRendering, this, &SceneTextureItem::render, Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &SceneTextureItem::handleSceneGraphInvalidated,
            Qt::DirectConnection);
}

void SceneTextureItem::handleSceneGraphInvalidated() {
    cleanup();
}

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

QSGNode* SceneTextureItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    uintptr_t texId = RenderBridge::instance().nativeTextureId();
    if (!texId) {
        update();
        return old;
    }

    auto* node = static_cast<QSGSimpleTextureNode*>(old);
    if (!node) {
        node = new QSGSimpleTextureNode();
        node->setOwnsTexture(true);
    }

    if (texId != m_cachedTexId) {
        auto* ri = window()->rendererInterface();
        auto* rhi = static_cast<QRhi*>(ri->getResource(window(), QSGRendererInterface::RhiResource));

        if (rhi) {
            // Ownership of the new QRhiTexture transfers to Qt via createTextureFromRhiTexture.
            // QSGSimpleTextureNode (ownsTexture=true) frees the old QRhiTexture when replaced.
            auto* rhiTex = rhi->newTexture(m_backend->textureFormat(), QSize(RenderBridge::instance().renderWidth(),
                                                                             RenderBridge::instance().renderHeight()));
            if (rhiTex) {
                rhiTex->createFrom({static_cast<quint64>(texId), 0});
                m_cachedTexId = texId;
                node->setTexture(window()->createTextureFromRhiTexture(rhiTex));
            }
        }
    }

    if (RenderBridge::instance().uvFlipped())
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);

    node->setRect(boundingRect());
    update();
    return node;
}

void SceneTextureItem::cleanup() {
    if (!m_initialized)
        return;

    RenderBridge::instance().setRenderManager(nullptr, 0, 0);
    m_cachedTexId = 0;
    m_renderManager.reset();
    m_initialized = false;
}
