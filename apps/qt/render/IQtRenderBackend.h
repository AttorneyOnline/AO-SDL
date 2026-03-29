#pragma once

#include <memory>

#include <rhi/qrhi.h>

/// Abstract GPU-backend adapter for the Qt application layer.
///
/// Encapsulates the three things that differ between OpenGL and Metal when
/// integrating an external renderer with Qt Quick's render thread:
///
///   1. Whether the GPU context is ready for renderer construction.
///   2. Post-draw state restoration (GL needs its default FBO rebound;
///      Metal is a no-op since the renderer uses its own command queue).
///   3. The QRhiTexture format matching the renderer's offscreen target.
///
/// Concrete implementations are selected at link time — one per build
/// configuration — following the same pattern as apps/sdl IGPUBackend.
class IQtRenderBackend {
  public:
    virtual ~IQtRenderBackend() = default;

    /// True when the GPU context is ready for renderer initialisation.
    /// GL returns true only when a current QOpenGLContext exists.
    /// Metal always returns true (the renderer creates its own device).
    virtual bool isContextReady() const = 0;

    /// Called after the game renderer draws each frame, before
    /// QQuickWindow::endExternalCommands().
    /// GL restores the default framebuffer; Metal is a no-op.
    virtual void restoreState() const = 0;

    /// QRhiTexture pixel format matching the renderer's offscreen target.
    /// GL produces RGBA8; Metal produces BGRA8.
    virtual QRhiTexture::Format textureFormat() const = 0;

    /// Backend name matching IRenderer::backend_name() ("OpenGL" or "Metal").
    /// Used to set the shader backend before the renderer is constructed.
    virtual const char* backendName() const = 0;
};

/// Factory — defined in MetalRenderBackend.mm or GLRenderBackend.cpp,
/// selected at link time via CMake.
std::unique_ptr<IQtRenderBackend> create_qt_render_backend();
