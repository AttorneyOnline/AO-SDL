#pragma once

#include "Shader.h"
#include "GLTexture.h"
#include "render/IRenderer.h"
#include "render/RenderState.h"

#include <map>
#include <memory>

class GLRenderer : public IRenderer {
  public:
    GLRenderer();

    static void init_gl();

    // IRenderer interface
    uint32_t draw(const RenderState* state) override;
    void bind_default_framebuffer() override;
    void clear() override;

  private:
    std::tuple<GLuint, GLuint> setup_render_texture();
    GLProgram program;

    uint64_t t = 0;

    std::map<uint64_t, GLTexture2D> textures;

    GLuint render_texture;
    GLuint framebuffer_id;
};

// Factory: creates a GLRenderer after initializing GLEW.
std::unique_ptr<IRenderer> create_gl_renderer();
