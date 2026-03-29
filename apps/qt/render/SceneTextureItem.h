#pragma once

#include <QQuickItem>
#include <QQuickWindow>

#include <memory>

class IQtRenderBackend;
class QRhiTexture;
class RenderManager;

/**
 * @brief QML item that renders the game scene outside the Qt scene graph.
 *
 * Hooks into QQuickWindow::beforeRendering() (Qt's render thread) to drive
 * RenderManager::render_frame() each frame.  The renderer writes into its own
 * offscreen target; the resulting texture is wrapped as a QRhiTexture and
 * displayed via a QSGSimpleTextureNode in updatePaintNode().
 *
 * GPU-backend differences (GL state restoration, texture format, context
 * readiness) are handled by IQtRenderBackend, selected at link time.
 *
 * RenderBridge must have a valid StateBuffer set (via setStateBuffer()) before
 * the first render fires.
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
    void render();
    void handleSceneGraphInvalidated();

  private:
    void initRenderer();
    void cleanup();

    std::unique_ptr<IQtRenderBackend> m_backend;

    // Owned on the render thread; created in initRenderer(), destroyed in cleanup().
    std::unique_ptr<RenderManager> m_renderManager;

    bool m_initialized = false;

    // Scene-graph wrapper for the renderer's offscreen texture.
    QRhiTexture* m_rhiTexture  = nullptr;
    uintptr_t    m_cachedTexId = 0;
};
