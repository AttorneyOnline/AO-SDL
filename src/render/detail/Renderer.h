#ifndef RENDERER_H
#define RENDERER_H

#include "Shader.h"
#include "Texture.h"
#include "render/RenderState.h"

class Renderer {
  public:
    Renderer();

    GLuint draw(const RenderState* state);

    std::tuple<GLuint, GLuint> setup_render_texture();

  private:
    GLProgram program;

    uint64_t t = 0;

    std::map<uint64_t, Texture2D> textures;

    GLuint render_texture;
    GLuint framebuffer_id;
};

#endif
