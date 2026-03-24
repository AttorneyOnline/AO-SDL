#pragma once

#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

class RenderManager;

/// QQuickItem that displays the engine's offscreen GL texture in the QML scene.
///
/// The engine renders into an FBO each frame (via RenderManager). This item
/// wraps that texture ID in a QSGSimpleTextureNode so Qt's scene graph can
/// composite it alongside QML UI elements.
class GameViewport : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

  public:
    explicit GameViewport(QQuickItem* parent = nullptr);

    /// Set the render manager whose texture this viewport displays.
    /// Must be called from C++ after the QML scene loads.
    void set_render_manager(RenderManager* rm);

    /// Set the actual pixel dimensions of the offscreen render texture.
    void set_render_size(int w, int h) { render_w_ = w; render_h_ = h; }

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

  private:
    RenderManager* render_manager_ = nullptr;
    uint64_t last_texture_id_ = 0;
    int render_w_ = 0;
    int render_h_ = 0;
};
