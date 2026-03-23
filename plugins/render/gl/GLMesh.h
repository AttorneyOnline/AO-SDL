#pragma once

#include "render/Math.h"

#include <GL/glew.h>

#include <vector>

struct VertexData {
    Vec2 position;
    Vec2 texcoord;
    Vec3 color = {1.0f, 1.0f, 1.0f};
};

class GLMesh {
  public:
    GLMesh(std::vector<VertexData> verts, std::vector<unsigned int> indices);
    GLMesh() = default;

    GLuint get_vao();

    std::vector<VertexData> verts;
    std::vector<unsigned int> indices;

  private:
    GLuint vao, vbo, ebo;
};
