#include "GLRenderer.h"

#include "GLSprite.h"
#include "utils/Log.h"

#include <algorithm>

void GLRenderer::init_gl() {
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        printf("Failed to initialize GLEW: %s\n", glewGetErrorString(glewError));
    }
}

GLRenderer::GLRenderer(const std::string& vertex_source, const std::string& fragment_source, int width, int height)
    : fb_width(width), fb_height(height) {
    GLShader vert(ShaderType::Vertex, vertex_source, true);
    GLShader frag(ShaderType::Fragment, fragment_source, true);
    program.link_shaders({vert, frag});

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    auto [tex, fbo] = setup_render_texture();
    render_texture = tex;
    framebuffer_id = fbo;
}

GLRenderer::~GLRenderer() {
    for (auto& [_, entry] : texture_cache) {
        glDeleteTextures(1, &entry.texture);
    }
}

GLuint GLRenderer::get_texture_array(const std::shared_ptr<ImageAsset>& asset) {
    auto it = texture_cache.find(asset.get());
    if (it != texture_cache.end()) {
        if (it->second.generation == asset->generation())
            return it->second.texture;

        // Same texture, new pixel data — update in place
        glBindTexture(GL_TEXTURE_2D_ARRAY, it->second.texture);
        int count = asset->frame_count();
        for (int i = 0; i < count; i++) {
            const auto& frame = asset->frame(i);
            int fw = std::min(frame.width, asset->width());
            int fh = std::min(frame.height, asset->height());
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, fw, fh, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            frame.pixels.data());
        }
        it->second.generation = asset->generation();
        return it->second.texture;
    }

    int w = asset->width();
    int h = asset->height();
    int count = asset->frame_count();
    if (w == 0 || h == 0 || count == 0)
        return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, w, h, count, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (int i = 0; i < count; i++) {
        const auto& frame = asset->frame(i);
        int fw = std::min(frame.width, w);
        int fh = std::min(frame.height, h);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, fw, fh, 1, GL_RGBA, GL_UNSIGNED_BYTE, frame.pixels.data());
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Log::log_print(DEBUG, "GLRenderer: uploaded %dx%d x %d frames for %s", w, h, count, asset->path().c_str());

    texture_cache[asset.get()] = {asset, tex, asset->generation()};
    return tex;
}

void GLRenderer::evict_expired_textures() {
    for (auto it = texture_cache.begin(); it != texture_cache.end();) {
        if (it->second.asset.expired()) {
            Log::log_print(DEBUG, "GLRenderer: evicting expired texture %u", it->second.texture);
            glDeleteTextures(1, &it->second.texture);
            it = texture_cache.erase(it);
        }
        else {
            ++it;
        }
    }
}

void GLRenderer::draw(const RenderState* state) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0, 0, fb_width, fb_height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sweep expired textures periodically (every ~60 frames, not every frame)
    if (++frame_counter % 60 == 0) {
        evict_expired_textures();
    }

    for (const auto& [_, group] : state->get_layer_groups()) {
        for (const auto& [__, layer] : group.get_layers()) {
            const auto& asset = layer.get_asset();
            if (!asset || asset->frame_count() == 0)
                continue;

            GLuint tex_array = get_texture_array(asset);
            if (tex_array == 0)
                continue;

            int frame = std::clamp(layer.get_frame_index(), 0, asset->frame_count() - 1);

            GLSprite sprite(tex_array, frame);
            sprite.zindex(layer.get_z_index() + 1);
            sprite.draw(program);
        }
    }
}

uintptr_t GLRenderer::get_render_texture_id() const {
    return static_cast<uintptr_t>(render_texture);
}

void GLRenderer::bind_default_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

std::unique_ptr<IRenderer> create_gl_renderer(const std::string& vertex_source, const std::string& fragment_source,
                                              int width, int height) {
    GLRenderer::init_gl();
    return std::make_unique<GLRenderer>(vertex_source, fragment_source, width, height);
}

// Embedded shaders for the common factory signature
static const char* embedded_vertex_glsl = R"glsl(
#version 450
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;

out vec2 vert_texcoord;

uniform mat4 local;
uniform float aspect;

void main() {
    gl_Position = local * vec4(vertex_pos.x, vertex_pos.y, 0.0f, 1.0f);
    vert_texcoord = vertex_tex_coord;
}
)glsl";

static const char* embedded_fragment_glsl = R"glsl(
#version 450
layout (location = 0) out vec4 frag_color;

in vec2 vert_texcoord;

uniform sampler2DArray texture_sample;
uniform int frame_index;

void main() {
    vec4 tex_color = texture(texture_sample, vec3(vert_texcoord, float(frame_index)));
    if (tex_color.a < 0.001f) {
        discard;
    }
    frag_color = tex_color;
}
)glsl";

std::unique_ptr<IRenderer> create_renderer(int width, int height) {
    return create_gl_renderer(embedded_vertex_glsl, embedded_fragment_glsl, width, height);
}

std::tuple<GLuint, GLuint> GLRenderer::setup_render_texture() {
    GLuint viewport_framebuffer = 0;
    glGenFramebuffers(1, &viewport_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, viewport_framebuffer);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fb_width, fb_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GLuint depth;
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fb_width, fb_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);

    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffers);

    return {tex, viewport_framebuffer};
}
