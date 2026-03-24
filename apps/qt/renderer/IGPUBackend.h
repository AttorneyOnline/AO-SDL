#pragma once

#include <functional>
#include <memory>

class IRenderer;
class QQuickWindow;

/// Abstract GPU backend for the Qt application layer.
///
/// Owns the GPU context (GL context / Metal view+layer), the QML rendering
/// backend, and the per-frame present/swap logic.  Concrete implementations
/// are selected at link time — one per build configuration.
class IGPUBackend {
  public:
    virtual ~IGPUBackend() = default;

    /// Window flags this backend requires.
    virtual uint32_t window_flags() const = 0;

    /// Set GL/Metal attributes that must be configured before window creation.
    static void pre_init();

    /// Create the GPU context for @p window. Called right after window creation,
    /// before the renderer is constructed (so GLEW can initialize).
    virtual void create_context(QQuickWindow* window) = 0;

    /// Initialise the QML rendering backend and wire up per-frame callbacks.
    /// @p render_cb is invoked each frame inside the external-command bracket
    /// (between beginExternalCommands/endExternalCommands) so that the game
    /// can render into its offscreen FBO before the scene graph composites.
    virtual void init_qml(QQuickWindow* window, IRenderer& renderer,
                          std::function<void()> render_cb) = 0;

    /// Tear down the QML backend and release the GPU context.
    virtual void shutdown() = 0;

    /// Start a new frame (acquire drawable, etc.).
    virtual void begin_frame() = 0;

    /// Submit draw data and present / swap.
    virtual void present() = 0;
};

/// Factory — defined in exactly one backend source file per build.
std::unique_ptr<IGPUBackend> create_gpu_backend();
