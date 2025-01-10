#pragma once

#include "StateBuffer.h"
#include "render/detail/Renderer.h"

#include <cstdint>
#include <memory>

class Renderer;

class RenderManager {
  public:
    RenderManager(StateBuffer& buf);

    uint32_t render_frame();
    void clear_framebuffer();
    void bind_framebuffer(unsigned int fb);

  private:
    StateBuffer& state_buf;

    class RendererDeleter {
      public:
        void operator()(Renderer* ptr) const {
            delete ptr;
        }
    };

    std::unique_ptr<class Renderer, RendererDeleter> renderer_ptr;
};
