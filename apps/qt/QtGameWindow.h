#pragma once

#include "ui/IUIRenderer.h"

#include <QObject>
#include <memory>
#include <string>

class QQmlComponent;
class QQmlEngine;
class QQuickItem;
class QQuickWindow;
class RenderManager;
class StateBuffer;
class UIManager;

#if !defined(Q_OS_APPLE)
class QOffscreenSurface;
class QOpenGLContext;
#endif

/**
 * @brief Qt/QML equivalent of SDLGameWindow.
 *
 * Creates a QQuickWindow with the platform-appropriate graphics API (OpenGL
 * on non-Apple, Metal on Apple), initialises the IRenderer against that
 * context/device, and loads Main.qml. Navigation actions from the UI are
 * forwarded back to UIManager via onNavAction().
 *
 * The render loop is driven by Qt's scene graph; there is no blocking
 * start_loop() call. Call show() then hand control to QGuiApplication::exec().
 */
class QtGameWindow : public QObject {
    Q_OBJECT

  public:
    /**
     * @param uiMgr     Engine screen stack — the window forwards nav actions to it.
     * @param buffer    Triple-buffered game state read by SceneTextureItem each frame.
     * @param renderW   Internal render width  (BASE_W * scale).
     * @param renderH   Internal render height (BASE_H * scale).
     */
    QtGameWindow(UIManager& uiMgr, StateBuffer& buffer,
                 int renderW, int renderH,
                 QObject* parent = nullptr);
    ~QtGameWindow() override;

    /// Show the window. Call after all engine objects are initialised.
    void show();

  public slots:
    /// Called by Qt controllers when a navigation action is triggered.
    void onNavAction(IUIRenderer::NavAction action);

  private slots:
    void onSceneGraphInitialized();
    void onScreenChanged();

  private:
    void initGraphics();
    void loadFonts();
    void setupQml();

    UIManager&    m_uiMgr;
    StateBuffer&  m_buffer;
    int           m_renderW;
    int           m_renderH;

    QQuickWindow*              m_window    = nullptr;
    QQmlEngine*                m_engine    = nullptr;
    QQmlComponent*             m_component = nullptr;
    QQuickItem*                m_rootItem  = nullptr;
    std::unique_ptr<RenderManager> m_renderManager;

    // Active screen ID cached to detect transitions
    std::string m_activeScreenId;

#if !defined(Q_OS_APPLE)
    QOffscreenSurface* m_glSurface  = nullptr;
    QOpenGLContext*    m_glContext   = nullptr;
#endif
};
