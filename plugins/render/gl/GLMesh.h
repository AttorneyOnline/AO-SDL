#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>

struct VertexData {
    glm::vec2 position;
    glm::vec2 texcoord;
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
