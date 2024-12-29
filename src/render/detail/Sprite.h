#ifndef RENDER_SPRITE_H
#define RENDER_SPRITE_H

#include "Shader.h"
#include "Texture.h"
#include "Transform.h"
#include "Vertex.h"

#include <vector>

class Sprite : public Transform {
  public:
    Sprite(Texture2D texture);
    void draw(GLProgram& shader);

  private:
    Texture2D texture;

    static Mesh get_quad_mesh();
    static Mesh* quad_mesh;
};

#endif
