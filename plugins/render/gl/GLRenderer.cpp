#include "GLRenderer.h"

#include "GLSprite.h"
#include "asset/ShaderAsset.h"
#include "render/Transform.h"
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

    // Wireframe shader: solid green, no texturing
    {
        static const char* wf_vert = R"glsl(
#version 450
layout (location = 0) in vec2 vertex_pos;
layout (location = 1) in vec2 vertex_tex_coord;
uniform mat4 local;
void main() { gl_Position = local * vec4(vertex_pos, 0.0, 1.0); }
)glsl";
        static const char* wf_frag = R"glsl(
#version 450
layout (location = 0) out vec4 frag_color;
void main() { frag_color = vec4(0.0, 1.0, 0.0, 1.0); }
)glsl";
        GLShader wv(ShaderType::Vertex, wf_vert, true);
        GLShader wf(ShaderType::Fragment, wf_frag, true);
        wireframe_program_.link_shaders({wv, wf});
    }

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

GLProgram& GLRenderer::resolve_program(const ShaderAsset* shader) {
    if (!shader || shader->is_default())
        return program;

    auto it = shader_cache_.find(shader);
    if (it != shader_cache_.end())
        return *it->second;

    auto prog = std::make_unique<GLProgram>();
    GLShader vert(ShaderType::Vertex, shader->vertex_source(), true);
    GLShader frag(ShaderType::Fragment, shader->fragment_source(), true);
    prog->link_shaders({vert, frag});

    auto* ptr = prog.get();
    shader_cache_[shader] = std::move(prog);
    return *ptr;
}

static void apply_uniforms(GLProgram& prog, const ShaderAsset* shader) {
    if (!shader || !shader->uniform_provider())
        return;
    prog.use();
    for (const auto& [name, val] : shader->uniform_provider()->get_uniforms()) {
        if (auto* i = std::get_if<int>(&val))
            prog.uniform(name, (GLint)*i);
        else if (auto* f = std::get_if<float>(&val))
            prog.uniform(name, (GLfloat)*f);
        else if (auto* v2 = std::get_if<Vec2>(&val))
            prog.uniform(name, *v2);
        else if (auto* v3 = std::get_if<Vec3>(&val))
            prog.uniform(name, *v3);
        else if (auto* m = std::get_if<Mat4>(&val))
            prog.uniform(name, *m);
    }
}

GLRenderer::MeshCacheEntry& GLRenderer::get_mesh_entry(const std::shared_ptr<MeshAsset>& mesh) {
    auto it = mesh_cache_.find(mesh.get());
    if (it != mesh_cache_.end() && it->second.generation == mesh->generation())
        return it->second;

    MeshCacheEntry entry;
    if (it != mesh_cache_.end()) {
        entry = it->second;
    } else {
        glGenVertexArrays(1, &entry.vao);
        glGenBuffers(1, &entry.vbo);
        glGenBuffers(1, &entry.ebo);
    }

    glBindVertexArray(entry.vao);
    glBindBuffer(GL_ARRAY_BUFFER, entry.vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices().size() * sizeof(MeshVertex),
                 mesh->vertices().data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entry.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices().size() * sizeof(uint32_t),
                 mesh->indices().data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    entry.asset = mesh;
    entry.generation = mesh->generation();
    entry.index_count = mesh->index_count();
    mesh_cache_[mesh.get()] = entry;
    return mesh_cache_[mesh.get()];
}

void GLRenderer::evict_expired_meshes() {
    for (auto it = mesh_cache_.begin(); it != mesh_cache_.end();) {
        if (it->second.asset.expired()) {
            glDeleteVertexArrays(1, &it->second.vao);
            glDeleteBuffers(1, &it->second.vbo);
            glDeleteBuffers(1, &it->second.ebo);
            it = mesh_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void GLRenderer::draw(const RenderState* state) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0, 0, fb_width, fb_height);

    if (wireframe_) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glDisable(GL_BLEND);
    } else {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glEnable(GL_BLEND);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (++frame_counter % 60 == 0) {
        evict_expired_textures();
        evict_expired_meshes();
    }

    draw_calls_ = 0;
    for (const auto& [_, group] : state->get_layer_groups()) {
        const ShaderAsset* group_shader = group.get_shader().get();
        Mat4 group_mat = group.transform().get_local_transform();

        for (const auto& [__, layer] : group.get_layers()) {
            const auto& asset = layer.get_asset();
            if (!asset || asset->frame_count() == 0)
                continue;

            GLuint tex_array = get_texture_array(asset);
            if (tex_array == 0)
                continue;

            int frame = std::clamp(layer.get_frame_index(), 0, asset->frame_count() - 1);

            Mat4 local = group_mat * layer.transform().get_local_transform();

            if (wireframe_) {
                // Wireframe: solid green, no textures
                wireframe_program_.use();
                wireframe_program_.uniform("local", local);

                const auto& mesh = layer.get_mesh();
                if (mesh && mesh->index_count() > 0) {
                    auto& entry = get_mesh_entry(mesh);
                    glBindVertexArray(entry.vao);
                    glDrawElements(GL_TRIANGLES, (GLsizei)entry.index_count, GL_UNSIGNED_INT, NULL);
                    glBindVertexArray(0);
                } else {
                    glBindVertexArray(GLSprite::get_quad_mesh().get_vao());
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
                    glBindVertexArray(0);
                }
                glUseProgram(0);
            } else {
                const ShaderAsset* effective = layer.get_shader().get();
                if (!effective) effective = group_shader;
                GLProgram& prog = resolve_program(effective);
                apply_uniforms(prog, effective);

                const auto& mesh = layer.get_mesh();
                if (mesh && mesh->index_count() > 0) {
                    prog.use();
                    prog.uniform("local", local);
                    prog.uniform("aspect", Transform::get_aspect_ratio());
                    prog.uniform("frame_index", (GLint)frame);
                    prog.uniform("opacity", layer.get_opacity());
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_array);
                    prog.uniform("texture_sample", 0);

                    auto& entry = get_mesh_entry(mesh);
                    glBindVertexArray(entry.vao);
                    glDrawElements(GL_TRIANGLES, (GLsizei)entry.index_count, GL_UNSIGNED_INT, NULL);
                    glBindVertexArray(0);
                    glUseProgram(0);
                } else {
                    GLSprite sprite(tex_array, frame, local, Transform::get_aspect_ratio(), layer.get_opacity());
                    sprite.draw(prog);
                }
            }
            draw_calls_++;
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

void GLRenderer::set_wireframe(bool enabled) {
    wireframe_ = enabled;
    glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);

    // Use linear filtering in wireframe mode so thin lines aren't lost during downsampling
    GLenum filter = enabled ? GL_LINEAR : GL_NEAREST;
    glBindTexture(GL_TEXTURE_2D, render_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

uintptr_t GLRenderer::get_texture_id(const std::shared_ptr<ImageAsset>& asset) {
    if (!asset || asset->frame_count() == 0) return 0;
    // Upload on demand if not cached
    GLuint tex = get_texture_array(asset);
    return (uintptr_t)tex;
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
uniform float opacity;

void main() {
    vec4 tex_color = texture(texture_sample, vec3(vert_texcoord, float(frame_index)));
    tex_color.a *= opacity;
    if (tex_color.a < 0.001f) {
        discard;
    }
    frag_color = tex_color;
}
)glsl";

std::unique_ptr<IRenderer> create_renderer(int width, int height) {
    return create_gl_renderer(embedded_vertex_glsl, embedded_fragment_glsl, width, height);
}

void GLRenderer::resize(int width, int height) {
    if (width == fb_width && height == fb_height)
        return;

    // Tear down old render targets
    glDeleteFramebuffers(1, &framebuffer_id);
    glDeleteTextures(1, &render_texture);

    fb_width = width;
    fb_height = height;

    auto [tex, fbo] = setup_render_texture();
    render_texture = tex;
    framebuffer_id = fbo;

    Log::log_print(DEBUG, "GLRenderer: resized to %dx%d", fb_width, fb_height);
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
