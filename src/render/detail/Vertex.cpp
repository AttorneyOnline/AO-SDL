#include "Vertex.h"

Mesh::Mesh(std::vector<VertexData> verts, std::vector<unsigned int> indicies) : verts(verts), indicies(indicies) {
    GLint glsl_vertex_pos = 0;
    GLint glsl_vertex_tex_coord = 1;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // Create VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VertexData), &verts[0], GL_STATIC_DRAW);

    // Create EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(unsigned int), &indicies[0], GL_STATIC_DRAW);

    // Setup vertex attributes

    // Position
    glVertexAttribPointer(glsl_vertex_pos, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), NULL);
    glEnableVertexAttribArray(glsl_vertex_pos);

    // Texture coordinate
    glVertexAttribPointer(glsl_vertex_tex_coord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData),
                          (void*)(offsetof(VertexData, texcoord)));
    glEnableVertexAttribArray(glsl_vertex_tex_coord);

    glBindVertexArray(0);

    // Log::log_print(LogLevel::DEBUG, "We just made a new VAO");
}

GLuint Mesh::get_vao() {
    return vao;
}
