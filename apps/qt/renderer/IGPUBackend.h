#pragma once

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

    /// SDL window flags this backend requires (e.g. SDL_WINDOW_OPENGL, SDL_WINDOW_METAL).
    virtual uint32_t window_flags() const = 0;

    /// Set SDL attributes that must be configured before window creation.
    static void pre_init();

    /// Create the GPU context for @p window. Called right after window creation,
    /// before the renderer is constructed (so GLEW can initialize).
    virtual void create_context(QQuickWindow *window) = 0;

    /// Initialise the QML rendering backend. Called after the renderer exists.
    virtual void init_qml(QQuickWindow *window, IRenderer &renderer) = 0;

    /// Tear down the QML backend and release the GPU context.
    virtual void shutdown() = 0;

    /// Start a new frame (QML new-frame calls, acquire drawable, etc.).
    virtual void begin_frame() = 0;

    /// Submit the QML draw data and present / swap.
    virtual void present() = 0;
};

/// Factory — defined in exactly one backend source file per build.
std::unique_ptr<IGPUBackend> create_gpu_backend();
