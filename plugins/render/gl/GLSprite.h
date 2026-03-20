#pragma once

#include "GLMesh.h"
#include "Shader.h"
#include "render/Transform.h"

#include <GL/glew.h>

class GLSprite : public Transform {
  public:
    GLSprite(GLuint texture_array, int frame_index);
    void draw(GLProgram& shader);

  private:
    GLuint texture_array;
    int frame_index;

    static GLMesh get_quad_mesh();
    static GLMesh* quad_mesh;
};
