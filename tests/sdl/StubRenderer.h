#pragma once

#include "render/IRenderer.h"

/// Minimal IRenderer for tests — all methods are no-ops.
class StubRenderer : public IRenderer {
  public:
    void draw(const RenderState* /*state*/) override {
    }
    void bind_default_framebuffer() override {
    }
    void clear() override {
    }
    uintptr_t get_render_texture_id() const override {
        return 0;
    }
    bool uv_flipped() const override {
        return false;
    }
    void resize(int /*width*/, int /*height*/) override {
    }
    const char* backend_name() const override {
        return "stub";
    }
};
