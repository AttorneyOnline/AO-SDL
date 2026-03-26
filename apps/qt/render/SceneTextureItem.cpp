#include "SceneTextureItem.h"

#include "RenderBridge.h"

#include <QSGSimpleTextureNode>
#include <QQuickWindow>

// On Apple the Metal texture must be wrapped via Objective-C++.
// tex_from_native() is defined in SceneTextureItem_apple.mm on Apple,
// and inline below on all other platforms.
#if defined(Q_OS_APPLE)
QSGTexture* tex_from_native(uintptr_t texId, QQuickWindow* window,
                             int renderW, int renderH);
#else
static QSGTexture* tex_from_native(uintptr_t texId, QQuickWindow* window,
                                   int renderW, int renderH) {
    // Qt6: createTextureFromId was removed; use createTextureFromNativeObject.
    // nativeObjectPtr is a pointer to the GL texture ID (uint).
    unsigned int gl_id = static_cast<unsigned int>(texId);
    return window->createTextureFromNativeObject(
        QQuickWindow::NativeObjectTexture,
        &gl_id, 0,
        QSize(renderW, renderH));
}
#endif

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
