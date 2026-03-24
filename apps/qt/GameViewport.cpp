#include "GameViewport.h"

#include "render/IRenderer.h"
#include "render/RenderManager.h"

#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include <algorithm>
#include <cmath>

GameViewport::GameViewport(QQuickItem* parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

void GameViewport::set_render_manager(RenderManager* rm) {
    render_manager_ = rm;

    // Trigger a scene-graph update every frame so the texture stays current.
    if (window()) {
        connect(window(), &QQuickWindow::beforeSynchronizing, this, [this]() { update(); },
                Qt::DirectConnection);
    }
}

QSGNode* GameViewport::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    if (!render_manager_ || !window())
        return oldNode;

    auto texture_id = static_cast<quint64>(render_manager_->get_renderer().get_render_texture_id());
    if (texture_id == 0)
        return oldNode;

    auto* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node)
        node = new QSGSimpleTextureNode();

    // Recreate the QSGTexture wrapper when the GL texture ID changes.
    if (texture_id != last_texture_id_) {
        last_texture_id_ = texture_id;
        auto native_id = static_cast<GLuint>(texture_id);

        // Use the actual render texture pixel dimensions.
        QSize tex_size(render_w_ > 0 ? render_w_ : static_cast<int>(width()),
                       render_h_ > 0 ? render_h_ : static_cast<int>(height()));

        // Mark as opaque — the FBO uses non-premultiplied alpha (glBlendFunc
        // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA). Without this flag Qt's scene
        // graph would composite with premultiplied-alpha blending, darkening
        // the image.
        auto* tex = QNativeInterface::QSGOpenGLTexture::fromNative(
            native_id, window(), tex_size, QQuickWindow::TextureIsOpaque);
        tex->setFiltering(QSGTexture::Nearest); // Pixel art — no bilinear blur
        node->setTexture(tex);
        node->setOwnsTexture(true); // QSGTexture wrapper is owned, not the GL texture
    }

    // Integer-scaled, aspect-correct letterbox within the item bounds.
    // The render target is 4:3 (render_w_ x render_h_). We find the largest
    // integer multiplier of the base resolution (256x192) that fits, then
    // center the rect.
    constexpr int BASE_W = 256;
    constexpr int BASE_H = 192;

    int avail_w = static_cast<int>(width());
    int avail_h = static_cast<int>(height());

    int scale = std::max(1, std::min(avail_w / BASE_W, avail_h / BASE_H));
    int display_w = BASE_W * scale;
    int display_h = BASE_H * scale;

    qreal ox = std::floor((avail_w - display_w) / 2.0);
    qreal oy = std::floor((avail_h - display_h) / 2.0);

    node->setRect(QRectF(ox, oy, display_w, display_h));

    // GL textures are bottom-up; flip UV if the renderer says so.
    if (render_manager_->get_renderer().uv_flipped())
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    else
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}
