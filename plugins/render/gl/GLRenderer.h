#pragma once

#include "Shader.h"
#include "asset/ImageAsset.h"
#include "render/IRenderer.h"
#include "render/RenderState.h"

#include <GL/glew.h>

#include <memory>
#include <unordered_map>

class GLRenderer : public IRenderer {
  public:
    /// @param vertex_source   GLSL vertex shader source.
    /// @param fragment_source GLSL fragment shader source.
    /// @param width           Render framebuffer width in pixels.
    /// @param height          Render framebuffer height in pixels.
    GLRenderer(const std::string& vertex_source, const std::string& fragment_source, int width, int height);
    ~GLRenderer();

    static void init_gl();

    void draw(const RenderState* state) override;
    void bind_default_framebuffer() override;
    void clear() override;
    uintptr_t get_render_texture_id() const override;
    bool uv_flipped() const override {
        return true;
    }

  private:
    std::tuple<GLuint, GLuint> setup_render_texture();

    GLuint get_texture_array(const std::shared_ptr<ImageAsset>& asset);
    void evict_expired_textures();

    GLProgram program;
    GLuint render_texture;
    GLuint framebuffer_id;
    int fb_width;
    int fb_height;
    uint64_t frame_counter = 0;

    struct TextureCacheEntry {
        std::weak_ptr<ImageAsset> asset;
        GLuint texture;
    };

    std::unordered_map<const ImageAsset*, TextureCacheEntry> texture_cache;
};

/// Factory: creates a GLRenderer after initializing GLEW.
std::unique_ptr<IRenderer> create_gl_renderer(const std::string& vertex_source, const std::string& fragment_source,
                                              int width, int height);

/// Factory with embedded shaders — matches the common create_renderer signature.
std::unique_ptr<IRenderer> create_renderer(int width, int height);
