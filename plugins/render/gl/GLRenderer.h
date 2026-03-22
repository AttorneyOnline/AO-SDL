#pragma once

#include "Shader.h"
#include "asset/ImageAsset.h"
#include "asset/MeshAsset.h"
#include "render/IRenderer.h"
#include "render/RenderState.h"

#include <GL/glew.h>

#include <memory>
#include <unordered_map>

class GLRenderer : public IRenderer {
  public:
    /// @param width  Render framebuffer width in pixels.
    /// @param height Render framebuffer height in pixels.
    GLRenderer(int width, int height);
    ~GLRenderer();

    static void init_gl();

    void draw(const RenderState* state) override;
    void bind_default_framebuffer() override;
    void clear() override;
    void resize(int width, int height) override;
    void set_wireframe(bool enabled) override;
    uintptr_t get_texture_id(const std::shared_ptr<ImageAsset>& asset) override;
    uintptr_t get_render_texture_id() const override;
    bool uv_flipped() const override {
        return true;
    }
    const char* backend_name() const override {
        return "OpenGL";
    }

  private:
    std::tuple<GLuint, GLuint> setup_render_texture();

    GLuint get_texture_array(const std::shared_ptr<ImageAsset>& asset);
    void evict_expired_textures();

    GLProgram& resolve_program(const class ShaderAsset* shader);

    GLProgram program;
    GLProgram wireframe_program_;
    bool wireframe_ = false;
    GLuint render_texture;
    GLuint framebuffer_id;
    int fb_width;
    int fb_height;
    uint64_t frame_counter = 0;

    struct TextureCacheEntry {
        std::weak_ptr<ImageAsset> asset;
        GLuint texture;
        uint64_t generation = 0;
    };

    std::unordered_map<const ImageAsset*, TextureCacheEntry> texture_cache;
    std::unordered_map<const class ShaderAsset*, std::unique_ptr<GLProgram>> shader_cache_;

    struct MeshCacheEntry {
        std::weak_ptr<MeshAsset> asset;
        GLuint vao = 0, vbo = 0, ebo = 0;
        uint64_t generation = 0;
        size_t index_count = 0;
    };
    std::unordered_map<const MeshAsset*, MeshCacheEntry> mesh_cache_;

    MeshCacheEntry& get_mesh_entry(const std::shared_ptr<MeshAsset>& mesh);
    void evict_expired_meshes();
};

/// Factory: creates a GLRenderer after initializing GLEW.
std::unique_ptr<IRenderer> create_renderer(int width, int height);
