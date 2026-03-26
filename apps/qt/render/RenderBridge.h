#pragma once

#include <QObject>
#include <cstdint>

class RenderManager;

/**
 * @brief Singleton bridge between RenderManager and the QML scene graph.
 *
 * QtGameWindow initialises this with the RenderManager pointer and render
 * dimensions after the graphics context is ready.  SceneTextureItem reads
 * the bridge each frame to drive rendering and retrieve the native texture
 * handle for display.
 *
 * Registered as the "renderBridge" QML context property by QtGameWindow.
 */
class RenderBridge : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(RenderBridge)

    Q_PROPERTY(int renderWidth  READ renderWidth  CONSTANT)
    Q_PROPERTY(int renderHeight READ renderHeight CONSTANT)

  public:
    static RenderBridge& instance();

    /**
     * @brief Bind the RenderManager and its render dimensions.
     *
     * Must be called from the main thread once after graphics initialisation
     * and before the first frame is rendered.
     */
    void setRenderManager(RenderManager* rm, int renderWidth, int renderHeight);

    RenderManager* renderManager() const { return m_renderManager; }

    /// Render-resolution width passed to create_renderer().
    int renderWidth()  const { return m_renderWidth;  }

    /// Render-resolution height passed to create_renderer().
    int renderHeight() const { return m_renderHeight; }

    /**
     * @brief Advance the render pipeline by one frame.
     *
     * Called by SceneTextureItem::updatePaintNode() on the Qt render thread.
     * Pulls the latest committed game state from the StateBuffer and issues
     * draw calls via IRenderer.
     */
    void renderFrame();

    /**
     * @brief Opaque handle to the renderer's offscreen texture.
     *
     * On OpenGL this is a GLuint cast to uintptr_t.
     * On Metal this is an id<MTLTexture> bridged to uintptr_t.
     */
    uintptr_t nativeTextureId() const;

    /**
     * @brief True when the texture has V=0 at the bottom (OpenGL convention).
     *
     * Callers should apply QSGSimpleTextureNode::MirrorVertically when true.
     */
    bool uvFlipped() const;

  private:
    explicit RenderBridge(QObject* parent = nullptr);

    RenderManager* m_renderManager = nullptr;
    int            m_renderWidth   = 0;
    int            m_renderHeight  = 0;
};
