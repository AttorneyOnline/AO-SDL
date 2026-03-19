#include "GLSprite.h"

GLMesh* GLSprite::quad_mesh = nullptr;

GLSprite::GLSprite(GLuint texture_array, int frame_index)
    : texture_array(texture_array), frame_index(frame_index) {}

void GLSprite::draw(GLProgram& shader) {
    shader.use();
    shader.uniform("local", get_local_transform());
    shader.uniform("aspect", get_aspect_ratio());
    shader.uniform("frame_index", (GLint)frame_index);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    shader.uniform("texture_sample", 0);

    glBindVertexArray(get_quad_mesh().get_vao());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);

    glUseProgram(NULL);
}

GLMesh GLSprite::get_quad_mesh() {
    if (quad_mesh == nullptr) {
        const VertexData top_right = {{1.0f, 1.0f}, {1.0f, 1.0f}};
        const VertexData bot_right = {{1.0f, -1.0f}, {1.0f, 0.0f}};
        const VertexData top_left = {{-1.0f, 1.0f}, {0.0f, 1.0f}};
        const VertexData bot_left = {{-1.0f, -1.0f}, {0.0f, 0.0f}};

        const std::vector<VertexData> vertex_data = {top_right, bot_right, bot_left, top_left};
        const std::vector<unsigned int> index_data = {0, 1, 3, 1, 2, 3};

        quad_mesh = new GLMesh(vertex_data, index_data);
    }

    return *quad_mesh;
}
