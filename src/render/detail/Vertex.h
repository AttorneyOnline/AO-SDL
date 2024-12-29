#ifndef RENDER_VERTEX_H
#define RENDER_VERTEX_H

#include <glm/glm.hpp>

#include <GL/glew.h>

#include <vector>

struct VertexData {
    glm::vec2 position;
    glm::vec2 texcoord;
};

class Mesh {
  public:
    Mesh(std::vector<VertexData> verts, std::vector<unsigned int> indicies);
    Mesh() = default;

    GLuint get_vao();

    std::vector<VertexData> verts;
    std::vector<unsigned int> indicies;

  private:
    GLuint vao, vbo, ebo;
};

#endif
