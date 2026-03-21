#pragma once

#include "GLMesh.h"
#include "Shader.h"
#include "render/Math.h"

#include <GL/glew.h>

class GLSprite {
  public:
    GLSprite(GLuint texture_array, int frame_index, const Mat4& transform, float aspect, float opacity);
    void draw(GLProgram& shader);

  private:
    GLuint texture_array;
    int frame_index;
    Mat4 transform;
    float aspect;
    float opacity;

    static GLMesh get_quad_mesh();
    static GLMesh* quad_mesh;
};
