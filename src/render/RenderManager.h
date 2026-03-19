#pragma once

#include "StateBuffer.h"
#include "render/gl/GLRenderer.h"

#include <cstdint>
#include <memory>

class RenderManager {
  public:
    RenderManager(StateBuffer& buf);

    uint32_t render_frame();
    void begin_frame();

  private:
    StateBuffer& state_buf;

    class GLRendererDeleter {
      public:
        void operator()(GLRenderer* ptr) const {
            delete ptr;
        }
    };

    std::unique_ptr<GLRenderer, GLRendererDeleter> renderer_ptr;
};
