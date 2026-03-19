#pragma once

#include "Shader.h"
#include "GLTexture.h"
#include "GLMesh.h"
#include "render/Transform.h"

class GLSprite : public Transform {
  public:
    GLSprite(GLTexture2D texture);
    void draw(GLProgram& shader);

  private:
    GLTexture2D texture;

    static GLMesh get_quad_mesh();
    static GLMesh* quad_mesh;
};
