#pragma once

#include <QQuickItem>
#include <QQuickWindow>

class QOpenGLExtraFunctions;
class QRhiTexture;

/**
 * @brief QML item that renders a GL triangle outside the scene graph.
 *
 * Hooks into QQuickWindow::beforeRendering() to issue raw OpenGL commands
 * on Qt's render thread, drawing into a private FBO.  The FBO colour
 * attachment is then wrapped as a QRhiTexture and displayed through a
 * QSGSimpleTextureNode in updatePaintNode().
 *
 * No manual context management is needed — Qt's threaded render loop
 * ensures the GL context is current when our slots fire.
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

    QOpenGLExtraFunctions* m_gl = nullptr;

    // FBO colour attachment that receives the triangle.
    uint m_fbo        = 0;
    uint m_fboTexture = 0;
    int  m_texW       = 0;
    int  m_texH       = 0;

    // Shader / geometry.
    uint m_vao     = 0;
    uint m_vbo     = 0;
    uint m_program = 0;
    float m_angle  = 0.0f;

    bool m_glInitialized = false;

    // Scene-graph wrapper for the FBO texture.
    QRhiTexture* m_rhiTexture  = nullptr;
    uintptr_t    m_cachedTexId = 0;
};
