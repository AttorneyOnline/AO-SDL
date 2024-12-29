#include "Sprite.h"

Mesh* Sprite::quad_mesh = nullptr;

Sprite::Sprite(Texture2D texture) : texture(texture) {
}

void Sprite::draw(GLProgram& shader) {
    shader.use();
    shader.uniform("local", get_local_transform());
    shader.uniform("aspect", get_aspect_ratio());

    texture.activate(0);
    shader.uniform("texture_sample", 0);

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(get_quad_mesh().get_vao());
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);

    glUseProgram(NULL);
}

Mesh Sprite::get_quad_mesh() {
    if (quad_mesh == nullptr) {
        const VertexData top_right = {{1.0f, 1.0f}, {1.0f, 1.0f}};
        const VertexData bot_right = {{1.0f, -1.0f}, {1.0f, 0.0f}};
        const VertexData top_left = {{-1.0f, 1.0f}, {0.0f, 1.0f}};
        const VertexData bot_left = {{-1.0f, -1.0f}, {0.0f, 0.0f}};

        const std::vector<VertexData> vertex_data = {top_right, bot_right, bot_left, top_left};
        const std::vector<unsigned int> index_data = {0, 1, 3, 1, 2, 3};

        quad_mesh = new Mesh(vertex_data, index_data);
    }

    return *quad_mesh;
}
