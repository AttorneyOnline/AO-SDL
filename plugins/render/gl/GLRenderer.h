#pragma once

#include "Shader.h"
#include "render/IRenderer.h"
#include "render/RenderState.h"
#include "asset/ImageAsset.h"

#include <GL/glew.h>

#include <memory>
#include <unordered_map>

class GLRenderer : public IRenderer {
  public:
    GLRenderer(const std::string& vertex_source, const std::string& fragment_source);
    ~GLRenderer();

    static void init_gl();

    uint32_t draw(const RenderState* state) override;
    void bind_default_framebuffer() override;
    void clear() override;

  private:
    std::tuple<GLuint, GLuint> setup_render_texture();

    /// Get or create a GL_TEXTURE_2D_ARRAY for the given asset.
    /// Uploads all frames on first encounter, caches handle.
    GLuint get_texture_array(const std::shared_ptr<ImageAsset>& asset);

    /// Sweep expired entries from the texture cache and free their GL textures.
    void evict_expired_textures();

    GLProgram program;
    GLuint render_texture;
    GLuint framebuffer_id;

    /// Cache: weak_ptr to ImageAsset + GL texture handle.
    /// The weak_ptr follows the AssetCache's lifetime management.
    /// When the AssetCache evicts an asset (shared_ptr dies), the weak_ptr
    /// expires and we delete the GL texture on the next sweep.
    struct TextureCacheEntry {
        std::weak_ptr<ImageAsset> asset;
        GLuint texture;
    };

    /// Keyed by raw ImageAsset pointer (for fast lookup from Layer).
    /// The weak_ptr inside the entry is the actual lifetime tracker.
    std::unordered_map<const ImageAsset*, TextureCacheEntry> texture_cache;
};

std::unique_ptr<IRenderer> create_gl_renderer(const std::string& vertex_source,
                                               const std::string& fragment_source);
