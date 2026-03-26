#pragma once

#include <QObject>

class RenderManager;

/**
 * @brief Singleton bridge exposing RenderManager to the QML scene graph.
 *
 * QtGameWindow registers the RenderManager pointer here after graphics
 * initialisation. SceneTextureItem reads it each frame to call render_frame()
 * and retrieve the offscreen texture handle.
 *
 * Registered as a QML context property "renderBridge" by QtGameWindow.
 */
class RenderBridge : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(RenderBridge)

  public:
    static RenderBridge& instance();

    void setRenderManager(RenderManager* rm);

    RenderManager* renderManager() const {
        return m_renderManager;
    }

  private:
    explicit RenderBridge(QObject* parent = nullptr);

    RenderManager* m_renderManager = nullptr;
};
