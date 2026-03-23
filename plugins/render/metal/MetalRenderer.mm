#import "MetalRenderer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>

#include "asset/ImageAsset.h"
#include "render/RenderState.h"
#include "render/Transform.h"
#include "utils/Log.h"

#include "asset/MediaManager.h"
#include "asset/ShaderAsset.h"

#include <algorithm>
#include <unordered_map>

static std::string load_metal_source(const std::string &path) {
    auto data = MediaManager::instance().assets().raw(path);
    if (!data) {
        Log::log_print(FATAL, "MetalRenderer: missing shader: %s", path.c_str());
        return "";
    }
    return {data->begin(), data->end()};
}

// ---- GPU-side uniform structs (must match embedded MSL) ---------------------

struct VertexUniforms {
    simd_float4x4 local;
    float aspect;
};

struct FragmentUniforms {
    int32_t frame_index;
    float opacity;
    int32_t frame_count;
};

// ---- helpers ----------------------------------------------------------------

static simd_float4x4 mat4_to_simd(const Mat4 &m) {
    simd_float4x4 s;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            s.columns[c][r] = m[c][r];
    return s;
}

// ---- vertex layout (matches GLMesh VertexData) ------------------------------

struct MetalVertex {
    simd_float2 position;
    simd_float2 texcoord;
};

// ---- impl -------------------------------------------------------------------

struct MetalRendererImpl {
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLRenderPipelineState> pipeline;
    id<MTLRenderPipelineState> wireframe_pipeline;
    id<MTLDepthStencilState> depth_state;
    id<MTLSamplerState> sampler;
    id<MTLSamplerState> sampler_linear;
    id<MTLBuffer> quad_vb;
    id<MTLBuffer> quad_ib;
    id<MTLTexture> render_texture;
    id<MTLTexture> depth_texture;
    id<MTLRenderPipelineState> blit_pipeline;
    id<MTLTexture> display_texture;
    int fb_width;
    int fb_height;
    int display_width = 0;
    int display_height = 0;
    uint64_t frame_counter = 0;
    bool wireframe = false;

    struct TextureCacheEntry {
        std::weak_ptr<ImageAsset> asset;
        id<MTLBuffer> buffer; // nil for fallback (non-zero-copy) path
        id<MTLTexture> texture;
        uint64_t generation = 0;
        bool zero_copy = false;
    };
    std::unordered_map<const ImageAsset *, TextureCacheEntry> texture_cache;
    // Textures pending release — kept alive for one frame to avoid dangling
    // pointers in ImGui draw lists that reference them.
    std::vector<TextureCacheEntry> deferred_release;
    NSUInteger min_tex_align = 0; // cached from device

    // 2D views of array textures for ImGui preview (ImGui can't sample texture2d_array)
    struct PreviewViewEntry {
        std::weak_ptr<ImageAsset> asset;
        id<MTLTexture> view;
        uint64_t generation = 0;
    };
    std::unordered_map<const ImageAsset *, PreviewViewEntry> preview_views;

    std::unordered_map<const ShaderAsset *, id<MTLRenderPipelineState>> shader_pipeline_cache;

    struct MeshCacheEntry {
        std::weak_ptr<MeshAsset> asset;
        id<MTLBuffer> vb;
        id<MTLBuffer> ib;
        uint64_t generation = 0;
        size_t index_count = 0;
    };
    std::unordered_map<const MeshAsset *, MeshCacheEntry> mesh_cache;

    std::pair<id<MTLBuffer>, id<MTLBuffer>> get_mesh_buffers(const std::shared_ptr<MeshAsset> &mesh) {
        auto it = mesh_cache.find(mesh.get());
        if (it != mesh_cache.end() && it->second.generation == mesh->generation())
            return {it->second.vb, it->second.ib};

        id<MTLBuffer> vb = [device newBufferWithBytes:mesh->vertices().data()
                                               length:mesh->vertices().size() * sizeof(MeshVertex)
                                              options:MTLResourceStorageModeShared];
        id<MTLBuffer> ib = [device newBufferWithBytes:mesh->indices().data()
                                               length:mesh->indices().size() * sizeof(uint32_t)
                                              options:MTLResourceStorageModeShared];
        mesh_cache[mesh.get()] = {mesh, vb, ib, mesh->generation(), mesh->index_count()};
        return {vb, ib};
    }

    void evict_expired_meshes() {
        for (auto it = mesh_cache.begin(); it != mesh_cache.end();) {
            if (it->second.asset.expired())
                it = mesh_cache.erase(it);
            else
                ++it;
        }
    }

    // --- setup ---------------------------------------------------------------

    void init(int w, int h) {
        fb_width = w;
        fb_height = h;

        device = MTLCreateSystemDefaultDevice();
        command_queue = [device newCommandQueue];
        min_tex_align = [device minimumLinearTextureAlignmentForPixelFormat:MTLPixelFormatRGBA8Unorm];

        @autoreleasepool {
            build_pipeline();
            build_blit_pipeline();
            build_wireframe_pipeline();
            build_depth_state();
            build_sampler();
            build_quad();
            build_render_targets();
        }
    }

    void build_pipeline() {
        auto vert_src = load_metal_source("shaders/main/metal/vertex.metal");
        auto frag_src = load_metal_source("shaders/main/metal/fragment.metal");
        NSString *src = [NSString stringWithUTF8String:(vert_src + "\n" + frag_src).c_str()];

        NSError *err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            Log::log_print(FATAL, "Metal shader compile: %s", [[err localizedDescription] UTF8String]);
            return;
        }

        id<MTLFunction> vert = [lib newFunctionWithName:@"vertex_main"];
        id<MTLFunction> frag = [lib newFunctionWithName:@"fragment_main"];

        MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;
        vd.attributes[0].offset = offsetof(MetalVertex, position);
        vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat2;
        vd.attributes[1].offset = offsetof(MetalVertex, texcoord);
        vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(MetalVertex);

        MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction = vert;
        pd.fragmentFunction = frag;
        pd.vertexDescriptor = vd;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
        pd.colorAttachments[0].blendingEnabled = YES;
        pd.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!pipeline) {
            Log::log_print(FATAL, "Metal pipeline: %s", [[err localizedDescription] UTF8String]);
        }
    }

    void build_blit_pipeline() {
        auto vert_src = load_metal_source("shaders/blit/metal/vertex.metal");
        auto frag_src = load_metal_source("shaders/blit/metal/fragment.metal");
        NSString *src = [NSString stringWithUTF8String:(vert_src + "\n" + frag_src).c_str()];

        NSError *err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            Log::log_print(FATAL, "Metal blit shader compile: %s", [[err localizedDescription] UTF8String]);
            return;
        }

        MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction = [lib newFunctionWithName:@"blit_vertex"];
        pd.fragmentFunction = [lib newFunctionWithName:@"blit_fragment"];
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;

        blit_pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!blit_pipeline) {
            Log::log_print(FATAL, "Metal blit pipeline: %s", [[err localizedDescription] UTF8String]);
        }
    }

    void build_wireframe_pipeline() {
        auto vert_src = load_metal_source("shaders/wireframe/metal/vertex.metal");
        auto frag_src = load_metal_source("shaders/wireframe/metal/fragment.metal");
        NSString *src = [NSString stringWithUTF8String:(vert_src + "\n" + frag_src).c_str()];

        NSError *err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib)
            return;

        MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;
        vd.attributes[0].offset = offsetof(MetalVertex, position);
        vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat2;
        vd.attributes[1].offset = offsetof(MetalVertex, texcoord);
        vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(MetalVertex);

        MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction = [lib newFunctionWithName:@"wf_vertex"];
        pd.fragmentFunction = [lib newFunctionWithName:@"wf_fragment"];
        pd.vertexDescriptor = vd;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
        pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        wireframe_pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
    }

    void ensure_display_texture(int w, int h) {
        if (display_texture && display_width == w && display_height == h)
            return;

        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                      width:w
                                                                                     height:h
                                                                                  mipmapped:NO];
        td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModePrivate;
        display_texture = [device newTextureWithDescriptor:td];
        display_width = w;
        display_height = h;
    }

    uintptr_t blit_for_display(int w, int h) {
        if (w <= 0 || h <= 0)
            return (uintptr_t)(__bridge void *)render_texture;

        ensure_display_texture(w, h);

        @autoreleasepool {
            id<MTLCommandBuffer> cmd = [command_queue commandBuffer];

            MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];
            rpd.colorAttachments[0].texture = display_texture;
            rpd.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

            id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
            [enc setRenderPipelineState:blit_pipeline];
            [enc setFragmentTexture:render_texture atIndex:0];
            [enc setFragmentSamplerState:(wireframe ? sampler_linear : sampler) atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            [enc endEncoding];

            [cmd commit];
            [cmd waitUntilCompleted];
        }

        return (uintptr_t)(__bridge void *)display_texture;
    }

    void build_depth_state() {
        MTLDepthStencilDescriptor *dd = [[MTLDepthStencilDescriptor alloc] init];
        dd.depthCompareFunction = MTLCompareFunctionLess;
        dd.depthWriteEnabled = YES;
        depth_state = [device newDepthStencilStateWithDescriptor:dd];
    }

    void build_sampler() {
        MTLSamplerDescriptor *sd = [[MTLSamplerDescriptor alloc] init];
        sd.minFilter = MTLSamplerMinMagFilterNearest;
        sd.magFilter = MTLSamplerMinMagFilterNearest;
        sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler = [device newSamplerStateWithDescriptor:sd];

        sd.minFilter = MTLSamplerMinMagFilterLinear;
        sd.magFilter = MTLSamplerMinMagFilterLinear;
        sampler_linear = [device newSamplerStateWithDescriptor:sd];
    }

    void build_quad() {
        const MetalVertex verts[4] = {
            {{1.0f, 1.0f}, {1.0f, 1.0f}},
            {{1.0f, -1.0f}, {1.0f, 0.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{-1.0f, 1.0f}, {0.0f, 1.0f}},
        };
        const uint32_t indices[6] = {0, 1, 3, 1, 2, 3};

        quad_vb = [device newBufferWithBytes:verts length:sizeof(verts) options:MTLResourceStorageModeShared];
        quad_ib = [device newBufferWithBytes:indices length:sizeof(indices) options:MTLResourceStorageModeShared];
    }

    void build_render_targets() {
        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                      width:fb_width
                                                                                     height:fb_height
                                                                                  mipmapped:NO];
        td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModePrivate;
        render_texture = [device newTextureWithDescriptor:td];

        MTLTextureDescriptor *dd = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                      width:fb_width
                                                                                     height:fb_height
                                                                                  mipmapped:NO];
        dd.usage = MTLTextureUsageRenderTarget;
        dd.storageMode = MTLStorageModePrivate;
        depth_texture = [device newTextureWithDescriptor:dd];
    }

    // --- texture cache -------------------------------------------------------

    /// Check if an asset's pixel layout allows zero-copy buffer-backed textures.
    /// Requires bytesPerRow (width * 4) to be a multiple of the device's
    /// minimum linear texture alignment.
    /// Check if an asset's pixel layout allows zero-copy buffer-backed textures.
    /// Requirements:
    ///   - Single frame (buffer-backed textures don't support texture2d_array)
    ///   - bytesPerRow aligned to device minimum
    /// Check if an asset supports zero-copy buffer-backed textures.
    /// All frames are packed into a single texture2d atlas (vertically tiled),
    /// so this works for both single-frame and multi-frame assets.
    /// Requires bytesPerRow aligned to device minimum.
    bool can_zero_copy(const std::shared_ptr<ImageAsset> &asset) const {
        if (min_tex_align == 0)
            return false;
        NSUInteger bytes_per_row = (NSUInteger)asset->width() * 4;
        return (bytes_per_row % min_tex_align) == 0;
    }

    /// Create a buffer-backed texture that shares the asset's pixel memory.
    /// No GPU-side copy — the texture reads directly from the AlignedBuffer.
    /// Create a buffer-backed atlas texture (zero-copy).
    /// All frames are tiled vertically into a single texture2d.
    /// The shader computes atlas UVs from frame_index and frame_count.
    id<MTLTexture> create_zero_copy_texture(const std::shared_ptr<ImageAsset> &asset, id<MTLBuffer> &out_buffer) {
        int w = asset->width();
        int h = asset->height();
        int count = asset->frame_count();
        NSUInteger bytes_per_row = (NSUInteger)w * 4;
        NSUInteger atlas_height = (NSUInteger)h * count;

        const auto &buf = asset->pixel_data();

        // Wrap the page-aligned pixel buffer — no copy, GPU reads same memory.
        out_buffer = [device newBufferWithBytesNoCopy:(void *)buf.data()
                                               length:buf.allocated_size()
                                              options:MTLResourceStorageModeShared
                                          deallocator:nil];
        if (!out_buffer)
            return nil;

        // Create a tall texture2d: width x (height * frame_count).
        // Frames are packed vertically, matching the contiguous AlignedBuffer layout.
        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                      width:w
                                                                                     height:atlas_height
                                                                                  mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModeShared;

        id<MTLTexture> tex = [out_buffer newTextureWithDescriptor:td offset:0 bytesPerRow:bytes_per_row];
        return tex;
    }

    /// Fallback: allocate a texture2d atlas and copy pixel data into it.
    id<MTLTexture> create_fallback_texture(const std::shared_ptr<ImageAsset> &asset) {
        int w = asset->width();
        int h = asset->height();
        int count = asset->frame_count();

        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                      width:w
                                                                                     height:(NSUInteger)h * count
                                                                                  mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModeShared;

        id<MTLTexture> tex = [device newTextureWithDescriptor:td];

        // Upload all frames as a single contiguous block
        MTLRegion region = MTLRegionMake2D(0, 0, w, (NSUInteger)h * count);
        [tex replaceRegion:region mipmapLevel:0 withBytes:asset->pixel_data().data() bytesPerRow:w * 4];

        return tex;
    }

    id<MTLTexture> get_texture_array(const std::shared_ptr<ImageAsset> &asset) {
        auto it = texture_cache.find(asset.get());
        if (it != texture_cache.end()) {
            if (it->second.generation == asset->generation())
                return it->second.texture;

            // Generation mismatch — defer-release the old entry so ImGui draw
            // lists from this frame can still reference the old texture safely.
            deferred_release.push_back(std::move(it->second));
            preview_views.erase(it->first); // also invalidate preview
            texture_cache.erase(it);
        }

        int w = asset->width();
        int h = asset->height();
        int count = asset->frame_count();
        if (w == 0 || h == 0 || count == 0)
            return nil;

        TextureCacheEntry entry;
        entry.asset = asset;
        entry.generation = asset->generation();

        if (can_zero_copy(asset)) {
            entry.texture = create_zero_copy_texture(asset, entry.buffer);
            entry.zero_copy = true;
            Log::log_print(VERBOSE, "MetalRenderer: zero-copy %dx%d x %d frames for %s", w, h, count,
                           asset->path().c_str());
        }

        if (!entry.texture) {
            entry.texture = create_fallback_texture(asset);
            entry.zero_copy = false;
            Log::log_print(VERBOSE, "MetalRenderer: uploaded %dx%d x %d frames for %s", w, h, count,
                           asset->path().c_str());
        }

        texture_cache[asset.get()] = entry;
        return entry.texture;
    }

    /// Flush deferred releases from previous frame, then sweep for expired entries.
    /// Must be called BEFORE ImGui widget building to avoid dangling texture pointers.
    void evict_expired_textures() {
        // Flush deferred releases from previous frame — safe now, ImGui has rendered.
        deferred_release.clear();

        for (auto it = texture_cache.begin(); it != texture_cache.end();) {
            if (it->second.asset.expired()) {
                Log::log_print(VERBOSE, "MetalRenderer: evicting expired texture");
                preview_views.erase(it->first);
                it = texture_cache.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = preview_views.begin(); it != preview_views.end();) {
            if (it->second.asset.expired())
                it = preview_views.erase(it);
            else
                ++it;
        }
    }

    // --- shader pipeline cache -----------------------------------------------

    id<MTLRenderPipelineState> resolve_pipeline(const ShaderAsset *shader) {
        if (!shader || shader->is_default())
            return pipeline;

        auto it = shader_pipeline_cache.find(shader);
        if (it != shader_pipeline_cache.end())
            return it->second;

        // Metal: concatenate vertex + fragment source (both use known function names)
        std::string combined = shader->vertex_source() + "\n" + shader->fragment_source();
        NSString *src = [NSString stringWithUTF8String:combined.c_str()];

        NSError *err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            Log::log_print(FATAL, "Metal custom shader compile: %s", [[err localizedDescription] UTF8String]);
            return pipeline;
        }

        id<MTLFunction> vert = [lib newFunctionWithName:@"vertex_main"];
        id<MTLFunction> frag = [lib newFunctionWithName:@"fragment_main"];
        if (!vert || !frag) {
            Log::log_print(ERR, "Metal custom shader: missing vertex_main(%d) or fragment_main(%d)", vert != nil,
                           frag != nil);
            return pipeline;
        }

        MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;
        vd.attributes[0].offset = offsetof(MetalVertex, position);
        vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat2;
        vd.attributes[1].offset = offsetof(MetalVertex, texcoord);
        vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(MetalVertex);

        MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction = vert;
        pd.fragmentFunction = frag;
        pd.vertexDescriptor = vd;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
        pd.colorAttachments[0].blendingEnabled = YES;
        pd.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        auto pso = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!pso) {
            Log::log_print(ERR, "Metal custom pipeline create failed: %s",
                           err ? [[err localizedDescription] UTF8String] : "unknown");
            return pipeline;
        }

        shader_pipeline_cache[shader] = pso;
        return pso;
    }

    // --- draw ----------------------------------------------------------------

    int draw_call_count = 0;

    void draw(const RenderState *state) {
        draw_call_count = 0;
        @autoreleasepool {
            id<MTLCommandBuffer> cmd = [command_queue commandBuffer];

            MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];
            rpd.colorAttachments[0].texture = render_texture;
            rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
            rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
            rpd.colorAttachments[0].clearColor =
                wireframe ? MTLClearColorMake(0.0, 0.0, 0.0, 1.0) : MTLClearColorMake(0.1, 0.1, 0.2, 1.0);
            rpd.depthAttachment.texture = depth_texture;
            rpd.depthAttachment.loadAction = MTLLoadActionClear;
            rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
            rpd.depthAttachment.clearDepth = 1.0;

            id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
            [enc setDepthStencilState:depth_state];
            [enc setTriangleFillMode:wireframe ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
            [enc setVertexBuffer:quad_vb offset:0 atIndex:0];
            [enc setFragmentSamplerState:(wireframe ? sampler_linear : sampler) atIndex:0];

            // Evict expired textures/meshes periodically. Deferred releases from
            // the previous frame are flushed here (ImGui has finished rendering).
            if (++frame_counter % 60 == 0) {
                evict_expired_textures();
                evict_expired_meshes();
            }

            if (state) {
                for (const auto &[_, group] : state->get_layer_groups()) {
                    const ShaderAsset *group_shader = group.get_shader().get();
                    Mat4 group_mat = group.transform().get_local_transform();

                    for (const auto &[__, layer] : group.get_layers()) {
                        const auto &asset = layer.get_asset();
                        if (!asset || asset->frame_count() == 0)
                            continue;

                        id<MTLTexture> tex = get_texture_array(asset);
                        if (!tex)
                            continue;

                        int frame = std::clamp(layer.get_frame_index(), 0, asset->frame_count() - 1);

                        Mat4 local = group_mat * layer.transform().get_local_transform();
                        VertexUniforms vu;
                        vu.local = mat4_to_simd(local);
                        vu.aspect = Transform::get_aspect_ratio();
                        [enc setVertexBytes:&vu length:sizeof(vu) atIndex:1];

                        if (wireframe) {
                            // Wireframe: solid green, no textures
                            [enc setRenderPipelineState:wireframe_pipeline];
                        } else {
                            // Resolve shader: layer overrides group overrides default
                            const ShaderAsset *effective = layer.get_shader().get();
                            if (!effective)
                                effective = group_shader;
                            [enc setRenderPipelineState:resolve_pipeline(effective)];

                            [enc setFragmentTexture:tex atIndex:0];

                            // Build fragment uniforms
                            if (effective && effective->uniform_provider()) {
                                auto custom = effective->uniform_provider()->get_uniforms();
                                // Sort by key name for deterministic Metal struct layout
                                std::vector<std::pair<std::string, UniformValue>> sorted(custom.begin(), custom.end());
                                std::sort(sorted.begin(), sorted.end(),
                                          [](const auto &a, const auto &b) { return a.first < b.first; });
                                struct {
                                    int32_t frame_index;
                                    float opacity;
                                    int32_t frame_count;
                                    float extras[16];
                                } fu;
                                fu.frame_index = frame;
                                fu.opacity = layer.get_opacity();
                                fu.frame_count = asset->frame_count();
                                int ei = 0;
                                for (const auto &[name, val] : sorted) {
                                    if (auto *f = std::get_if<float>(&val))
                                        if (ei < 16)
                                            fu.extras[ei++] = *f;
                                }
                                size_t fu_size = offsetof(decltype(fu), extras) + ei * sizeof(float);
                                [enc setFragmentBytes:&fu length:fu_size atIndex:0];
                            } else {
                                FragmentUniforms fu;
                                fu.frame_index = frame;
                                fu.opacity = layer.get_opacity();
                                fu.frame_count = asset->frame_count();
                                [enc setFragmentBytes:&fu length:sizeof(fu) atIndex:0];
                            }
                        } // end !wireframe

                        const auto &mesh = layer.get_mesh();
                        if (mesh && mesh->index_count() > 0) {
                            auto [mvb, mib] = get_mesh_buffers(mesh);
                            [enc setVertexBuffer:mvb offset:0 atIndex:0];
                            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                            indexCount:mesh->index_count()
                                             indexType:MTLIndexTypeUInt32
                                           indexBuffer:mib
                                     indexBufferOffset:0];
                            // Restore default quad VB for subsequent layers
                            [enc setVertexBuffer:quad_vb offset:0 atIndex:0];
                        } else {
                            [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                            indexCount:6
                                             indexType:MTLIndexTypeUInt32
                                           indexBuffer:quad_ib
                                     indexBufferOffset:0];
                        }
                        draw_call_count++;
                    }
                }
            }

            [enc endEncoding];
            [cmd commit];
            [cmd waitUntilCompleted];
        }
    }
};

// ---- MetalRenderer public API -----------------------------------------------

MetalRenderer::MetalRenderer(int width, int height) : impl(std::make_unique<MetalRendererImpl>()) {
    impl->init(width, height);
}

MetalRenderer::~MetalRenderer() = default;

void MetalRenderer::draw(const RenderState *state) {
    impl->draw(state);
    draw_calls_ = impl->draw_call_count;
}

void MetalRenderer::bind_default_framebuffer() {
    // No-op for Metal — the app layer manages the drawable.
}

void MetalRenderer::clear() {
    // Clearing is done at the start of each draw() via loadAction.
}

void MetalRenderer::set_wireframe(bool enabled) { impl->wireframe = enabled; }

uintptr_t MetalRenderer::get_texture_id(const std::shared_ptr<ImageAsset> &asset) {
    if (!asset || asset->frame_count() == 0)
        return 0;
    // Upload on demand if not cached
    id<MTLTexture> tex = impl->get_texture_array(asset);
    if (!tex)
        return 0;

    // Always create a separate preview texture owned by preview_views.
    // The main texture cache can evict/recreate entries (e.g. glyph atlas grow),
    // but ImGui draw lists hold raw texture pointers across the frame. Preview
    // textures must stay alive independently of the main cache.
    auto vit = impl->preview_views.find(asset.get());
    if (vit != impl->preview_views.end() && vit->second.generation == asset->generation())
        return (uintptr_t)(__bridge void *)vit->second.view;

    int w = asset->width();
    int h = asset->height();
    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                  width:w
                                                                                 height:h
                                                                              mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead;
    td.storageMode = MTLStorageModeShared;
    id<MTLTexture> view = [impl->device newTextureWithDescriptor:td];
    MTLRegion region = MTLRegionMake2D(0, 0, w, h);
    [view replaceRegion:region mipmapLevel:0 withBytes:asset->frame_pixels(0) bytesPerRow:w * 4];

    impl->preview_views[asset.get()] = {asset, view, asset->generation()};
    return (uintptr_t)(__bridge void *)view;
}

void MetalRenderer::resize(int width, int height) {
    if (width == impl->fb_width && height == impl->fb_height)
        return;
    impl->fb_width = width;
    impl->fb_height = height;
    @autoreleasepool {
        impl->build_render_targets();
    }
    // Invalidate display texture so it gets recreated at next blit
    impl->display_texture = nil;
    impl->display_width = 0;
    impl->display_height = 0;
    Log::log_print(DEBUG, "MetalRenderer: resized to %dx%d", width, height);
}

uintptr_t MetalRenderer::get_render_texture_id() const { return (uintptr_t)(__bridge void *)impl->render_texture; }

uintptr_t MetalRenderer::get_display_texture_id(int display_w, int display_h) {
    return impl->blit_for_display(display_w, display_h);
}

void *MetalRenderer::get_device_ptr() const { return (__bridge void *)impl->device; }

void *MetalRenderer::get_command_queue_ptr() const { return (__bridge void *)impl->command_queue; }

std::unique_ptr<IRenderer> create_renderer(int width, int height) {
    return std::make_unique<MetalRenderer>(width, height);
}
