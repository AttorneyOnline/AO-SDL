#pragma once

#include "renderer/IGPUBackend.h"
#include "ui/UIManager.h"

#include <functional>
#include <memory>

class QQuickView;
class QmlUIBridge;
class RenderManager;

/// Factory that creates a RenderManager once the GL context is available.
using RendererFactory = std::function<std::unique_ptr<RenderManager>()>;

class QtGameWindow {
  public:
    QtGameWindow(UIManager& ui_manager, std::unique_ptr<IGPUBackend> backend);
    ~QtGameWindow();

    /// Set a callback invoked once per frame before UI rendering.
    void set_frame_callback(std::function<void()> cb) {
        frame_callback_ = std::move(cb);
    }

    /// Wire up the render pipeline and start the per-frame cycle.
    /// The @p factory is called once the GL context is ready (sceneGraphInitialized).
    /// Unlike SDL's blocking start_loop(), this just connects Qt signals;
    /// the actual loop is driven by QGuiApplication::exec().
    void start_rendering(RendererFactory factory, QmlUIBridge& bridge, int render_w = 0, int render_h = 0);

    /// Access the underlying QQuickView (e.g. to set context properties).
    QQuickView* view() const { return window_; }

  private:
    QQuickView* window_;
    UIManager& ui_manager_;
    std::unique_ptr<IGPUBackend> gpu_;
    std::unique_ptr<RenderManager> renderer_;
    std::function<void()> frame_callback_;
};
