#include "GLRenderer.h"

#include "GLSprite.h"

void GLRenderer::init_gl() {
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        printf("Failed to initialize GLEW: %s\n", glewGetErrorString(glewError));
    }
}

GLRenderer::GLRenderer() {
    GLShader vert(ShaderType::Vertex,
                  "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\shaders\\vertex.glsl");
    GLShader frag(ShaderType::Fragment,
                  "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\shaders\\fragment.glsl");
    program.link_shaders({vert, frag});

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    std::tuple<GLuint, GLuint> buffer_info = setup_render_texture();

    render_texture = std::get<0>(buffer_info);
    framebuffer_id = std::get<1>(buffer_info);
}

GLuint GLRenderer::draw(const RenderState* state) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0, 0, 1280, 720);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const auto& layer_group_pair : state->get_layer_groups()) {
        for (const auto& layer_pair : layer_group_pair.second.get_layers()) {
            Layer layer = layer_pair.second;
            Image layer_image = layer.get_image();

            if (!textures.contains(layer_image.get_id())) {
                GLTexture2D tex(layer_image);
                textures.emplace(layer_image.get_id(), tex);
            }

            GLTexture2D tex = textures.at(layer_image.get_id());
            GLSprite layer_sprite(tex);
            layer_sprite.zindex(layer.get_z_index() + 1);
            layer_sprite.draw(program);
        }
    }

    return render_texture;
}

void GLRenderer::bind_default_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

std::tuple<GLuint, GLuint> GLRenderer::setup_render_texture() {
    GLuint viewport_framebuffer = 0;
    glGenFramebuffers(1, &viewport_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, viewport_framebuffer);

    GLuint render_texture;
    glGenTextures(1, &render_texture);

    glBindTexture(GL_TEXTURE_2D, render_texture);

    // todo: make render texture size scalable via config
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint render_texture_depth;
    glGenRenderbuffers(1, &render_texture_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, render_texture_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1280, 720);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_texture_depth);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);

    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    return {render_texture, viewport_framebuffer};
}
