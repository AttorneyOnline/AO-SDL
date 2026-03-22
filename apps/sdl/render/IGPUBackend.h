#pragma once

#include <memory>

struct SDL_Window;
class IRenderer;

/// Abstract GPU backend for the SDL application layer.
///
/// Owns the GPU context (GL context / Metal view+layer), the ImGui rendering
/// backend, and the per-frame present/swap logic.  Concrete implementations
/// are selected at link time — one per build configuration.
class IGPUBackend {
  public:
    virtual ~IGPUBackend() = default;

    /// SDL window flags this backend requires (e.g. SDL_WINDOW_OPENGL, SDL_WINDOW_METAL).
    virtual uint32_t window_flags() const = 0;

    /// Set SDL attributes that must be configured before window creation.
    virtual void pre_init() {
    }

    /// Create the GPU context and initialise the ImGui backend for @p window.
    /// @p renderer is available for backends that need device/queue handles.
    virtual void init(SDL_Window* window, IRenderer& renderer) = 0;

    /// Tear down the ImGui backend and release the GPU context.
    virtual void shutdown() = 0;

    /// Start a new frame (ImGui new-frame calls, acquire drawable, etc.).
    virtual void begin_frame() = 0;

    /// Submit the ImGui draw data and present / swap.
    virtual void present() = 0;
};

/// Factory — defined in exactly one backend source file per build.
std::unique_ptr<IGPUBackend> create_gpu_backend();
