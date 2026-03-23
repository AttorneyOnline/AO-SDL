#include "GLMesh.h"

GLMesh::GLMesh(std::vector<VertexData> verts, std::vector<unsigned int> indices) : verts(verts), indices(indices) {
    GLint glsl_vertex_pos = 0;
    GLint glsl_vertex_tex_coord = 1;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(VertexData), &verts[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(glsl_vertex_pos, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), NULL);
    glEnableVertexAttribArray(glsl_vertex_pos);

    glVertexAttribPointer(glsl_vertex_tex_coord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData),
                          (void*)(offsetof(VertexData, texcoord)));
    glEnableVertexAttribArray(glsl_vertex_tex_coord);

    GLint glsl_vertex_color = 2;
    glVertexAttribPointer(glsl_vertex_color, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData),
                          (void*)(offsetof(VertexData, color)));
    glEnableVertexAttribArray(glsl_vertex_color);

    glBindVertexArray(0);
}

GLuint GLMesh::get_vao() {
    return vao;
}
