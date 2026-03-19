#pragma once

#include "Shader.h"
#include "GLTexture.h"
#include "render/RenderState.h"

#include <map>

class GLRenderer {
  public:
    GLRenderer();

    static void init_gl();

    GLuint draw(const RenderState* state);
    void bind_default_framebuffer();
    void clear();

  private:
    std::tuple<GLuint, GLuint> setup_render_texture();
    GLProgram program;

    uint64_t t = 0;

    std::map<uint64_t, GLTexture2D> textures;

    GLuint render_texture;
    GLuint framebuffer_id;
};
