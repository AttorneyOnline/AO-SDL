#include "SceneTextureItem.h"

#include "RenderBridge.h"

#include <rhi/qrhi.h>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

// Wrap an existing native texture (GL name or Metal pointer) in a QSGTexture
// using Qt's RHI layer. createTextureFromRhiTexture is the supported API in
// Qt 6.6+ and works identically on all backends/platforms.
static QSGTexture* tex_from_native(uintptr_t texId, QQuickWindow* window,
                                   int renderW, int renderH) {
    auto *ri  = window->rendererInterface();
    auto *rhi = static_cast<QRhi *>(
        ri->getResource(window, QSGRendererInterface::RhiResource));
    if (!rhi)
        return nullptr;

    // Metal render target is MTLPixelFormatBGRA8Unorm; GL is GL_RGBA8.
    // Declaring the wrong format to RHI causes channel-swapped output.
    const bool isMetal =
        (ri->graphicsApi() == QSGRendererInterface::Metal);
    const QRhiTexture::Format fmt =
        isMetal ? QRhiTexture::BGRA8 : QRhiTexture::RGBA8;

    QRhiTexture *rhiTex = rhi->newTexture(fmt, QSize(renderW, renderH));
    rhiTex->createFrom({static_cast<quint64>(texId), 0});
    return window->createTextureFromRhiTexture(rhiTex);
}

// --------------------------------------------------------------------------

SceneTextureItem::SceneTextureItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QSGNode* SceneTextureItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*) {
    RenderBridge& bridge = RenderBridge::instance();

    if (!bridge.renderManager())
        return old;

    // Drive the game renderer — this pulls the latest committed game state
    // from the StateBuffer and issues draw calls to the offscreen target.
    bridge.renderFrame();

    auto* texNode = static_cast<QSGSimpleTextureNode*>(old);
    if (!texNode)
        texNode = new QSGSimpleTextureNode();

    QSGTexture* tex = tex_from_native(bridge.nativeTextureId(), window(),
                                      bridge.renderWidth(), bridge.renderHeight());
    texNode->setTexture(tex);
    texNode->setOwnsTexture(true);

    // OpenGL textures have V=0 at the bottom; flip so QML sees the scene
    // right-side up without any coordinate gymnastics in shaders.
    if (bridge.uvFlipped())
        texNode->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    else
        texNode->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);

    texNode->setRect(boundingRect());

    // Request the next frame immediately — the game runs at display refresh rate.
    update();

    return texNode;
}
