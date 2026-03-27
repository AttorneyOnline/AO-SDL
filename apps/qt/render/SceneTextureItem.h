#pragma once

#include <QQuickItem>
#include <QQuickWindow>

#include <memory>

class QRhiTexture;
class RenderManager;

/**
 * @brief QML item that renders the game scene outside the Qt scene graph.
 *
 * Hooks into QQuickWindow::beforeRendering() (Qt's render thread) to drive
 * RenderManager::render_frame() each frame.  The renderer writes into its own
 * offscreen FBO; the resulting texture is wrapped as a QRhiTexture and
 * displayed via a QSGSimpleTextureNode in updatePaintNode().
 *
 * Initialisation order each session:
 *   1. handleWindowChanged() — connects render-thread signals
 *   2. renderGL() → initGL() on first call — creates RenderManager, registers
 *      it on RenderBridge
 *   3. updatePaintNode() — wraps the renderer texture for the scene graph
 *
 * RenderBridge must have a valid StateBuffer set (via setStateBuffer()) before
 * the first renderGL() fires.
 */
class SceneTextureItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

  public:
    explicit SceneTextureItem(QQuickItem* parent = nullptr);
    ~SceneTextureItem() override;

  protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override;

  private slots:
    void handleWindowChanged(QQuickWindow* win);
    void renderGL();
    void handleSceneGraphInvalidated();

  private:
    void initGL();
    void cleanupGL();

    // Owned on the render thread; created in initGL(), destroyed in cleanupGL().
    std::unique_ptr<RenderManager> m_renderManager;

    bool m_glInitialized = false;

    // Scene-graph wrapper for the renderer's offscreen texture.
    QRhiTexture* m_rhiTexture  = nullptr;
    uintptr_t    m_cachedTexId = 0;
};
