#pragma once

#include "render/IRenderer.h"

#include <memory>

/// Opaque wrapper so the header stays pure C++.
struct MetalRendererImpl;

class MetalRenderer : public IRenderer {
  public:
    MetalRenderer(int width, int height);
    ~MetalRenderer();

    void draw(const RenderState* state) override;
    void bind_default_framebuffer() override;
    void clear() override;
    void resize(int width, int height) override;
    void set_wireframe(bool enabled) override;
    uintptr_t get_texture_id(const std::shared_ptr<ImageAsset>& asset) override;
    uintptr_t get_render_texture_id() const override;
    uintptr_t get_display_texture_id(int display_w, int display_h) override;
    bool uv_flipped() const override {
        return false;
    }
    void* get_device_ptr() const override;
    void* get_command_queue_ptr() const override;
    const char* backend_name() const override { return "Metal"; }

  private:
    std::unique_ptr<MetalRendererImpl> impl;
};

/// Factory with the common create_renderer signature.
std::unique_ptr<IRenderer> create_renderer(int width, int height);
