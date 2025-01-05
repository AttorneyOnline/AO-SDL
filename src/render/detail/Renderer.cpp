#include "Renderer.h"

#include "Sprite.h"

Renderer::Renderer() {
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

GLuint Renderer::draw(const RenderState* state) {
    // Render to our framebuffer
    // todo: manage the framebuffer and bind it elsewhere. that functionality doesn't really belong in this class.
    // the code to generate the render textures should also be done elsewhere
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0, 0, 1920,
               1080); // Render on the whole framebuffer, complete from the lower left corner to the upper right

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (const auto& layer_group_pair : state->get_layer_groups()) {
        for (const auto& layer_pair : layer_group_pair.second.get_layers()) {
            Layer layer = layer_pair.second;
            Image layer_image = layer.get_image();

            if (!textures.contains(layer_image.get_id())) {
                Texture2D tex(layer_image);
                textures.emplace(layer_image.get_id(), tex);
            }

            Texture2D tex = textures.at(layer_image.get_id());
            Sprite layer_sprite(tex);
            layer_sprite.zindex(layer.get_z_index() + 1);
            layer_sprite.draw(program);
        }
    }

    return render_texture;
}

std::tuple<GLuint, GLuint> Renderer::setup_render_texture() {
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    GLuint viewport_framebuffer = 0;
    glGenFramebuffers(1, &viewport_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, viewport_framebuffer);

    GLuint render_texture;
    glGenTextures(1, &render_texture);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, render_texture);

    // Give an empty image to OpenGL ( the last "0" )
    // todo: make render texture size scalable via config
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint render_texture_depth;
    glGenRenderbuffers(1, &render_texture_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, render_texture_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1920, 1080);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_texture_depth);

    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, render_texture, 0);

    // Set the list of draw buffers.
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers); // "1" is the size of DrawBuffers

    return {render_texture, viewport_framebuffer};
}
