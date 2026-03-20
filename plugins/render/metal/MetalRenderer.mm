#import "MetalRenderer.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <simd/simd.h>

#include "render/RenderState.h"
#include "render/Transform.h"
#include "asset/ImageAsset.h"
#include "utils/Log.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <unordered_map>

// ---- GPU-side uniform structs (must match embedded MSL) ---------------------

struct VertexUniforms {
    simd_float4x4 local;
    float aspect;
};

struct FragmentUniforms {
    int32_t frame_index;
};

// ---- helpers ----------------------------------------------------------------

static simd_float4x4 glm_to_simd(const glm::mat4& m) {
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
    id<MTLDevice>              device;
    id<MTLCommandQueue>        command_queue;
    id<MTLRenderPipelineState> pipeline;
    id<MTLDepthStencilState>   depth_state;
    id<MTLSamplerState>        sampler;
    id<MTLBuffer>              quad_vb;
    id<MTLBuffer>              quad_ib;
    id<MTLTexture>             render_texture;
    id<MTLTexture>             depth_texture;
    id<MTLRenderPipelineState> blit_pipeline;
    id<MTLTexture>             display_texture;
    int fb_width;
    int fb_height;
    int display_width  = 0;
    int display_height = 0;
    uint64_t frame_counter = 0;

    struct TextureCacheEntry {
        std::weak_ptr<ImageAsset> asset;
        id<MTLTexture> texture;
    };
    std::unordered_map<const ImageAsset*, TextureCacheEntry> texture_cache;

    // --- setup ---------------------------------------------------------------

    void init(int w, int h) {
        fb_width  = w;
        fb_height = h;

        device = MTLCreateSystemDefaultDevice();
        command_queue = [device newCommandQueue];

        build_pipeline();
        build_blit_pipeline();
        build_depth_state();
        build_sampler();
        build_quad();
        build_render_targets();
    }

    void build_pipeline() {
        NSString* src = @
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "struct VertexIn { float2 position [[attribute(0)]]; float2 texcoord [[attribute(1)]]; };\n"
            "struct VertexOut { float4 position [[position]]; float2 texcoord; };\n"
            "struct VertexUniforms { float4x4 local; float aspect; };\n"
            "struct FragmentUniforms { int frame_index; };\n"
            "vertex VertexOut vertex_main(VertexIn in [[stage_in]],\n"
            "                             constant VertexUniforms& u [[buffer(1)]]) {\n"
            "    VertexOut out;\n"
            "    out.position = u.local * float4(in.position, 0.0, 1.0);\n"
            "    out.texcoord = in.texcoord;\n"
            "    return out;\n"
            "}\n"
            "fragment float4 fragment_main(VertexOut in [[stage_in]],\n"
            "                              texture2d_array<float> tex [[texture(0)]],\n"
            "                              sampler samp [[sampler(0)]],\n"
            "                              constant FragmentUniforms& u [[buffer(0)]]) {\n"
            "    float4 color = tex.sample(samp, in.texcoord, u.frame_index);\n"
            "    if (color.a < 0.001) discard_fragment();\n"
            "    return color;\n"
            "}\n";

        NSError* err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            Log::log_print(FATAL, "Metal shader compile: %s",
                           [[err localizedDescription] UTF8String]);
            return;
        }

        id<MTLFunction> vert = [lib newFunctionWithName:@"vertex_main"];
        id<MTLFunction> frag = [lib newFunctionWithName:@"fragment_main"];

        MTLVertexDescriptor* vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;
        vd.attributes[0].offset = offsetof(MetalVertex, position);
        vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat2;
        vd.attributes[1].offset = offsetof(MetalVertex, texcoord);
        vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = sizeof(MetalVertex);

        MTLRenderPipelineDescriptor* pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction   = vert;
        pd.fragmentFunction = frag;
        pd.vertexDescriptor = vd;
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
        pd.colorAttachments[0].blendingEnabled = YES;
        pd.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
        pd.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

        pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!pipeline) {
            Log::log_print(FATAL, "Metal pipeline: %s",
                           [[err localizedDescription] UTF8String]);
        }
    }

    void build_blit_pipeline() {
        NSString* src = @
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            "struct VertexOut { float4 position [[position]]; float2 texcoord; };\n"
            "vertex VertexOut blit_vertex(uint vid [[vertex_id]]) {\n"
            "    float2 pos[4] = { {-1,-1}, {1,-1}, {-1,1}, {1,1} };\n"
            "    float2 uv[4]  = { {0,1}, {1,1}, {0,0}, {1,0} };\n"
            "    VertexOut out;\n"
            "    out.position = float4(pos[vid], 0, 1);\n"
            "    out.texcoord = uv[vid];\n"
            "    return out;\n"
            "}\n"
            "fragment float4 blit_fragment(VertexOut in [[stage_in]],\n"
            "                              texture2d<float> tex [[texture(0)]],\n"
            "                              sampler samp [[sampler(0)]]) {\n"
            "    return tex.sample(samp, in.texcoord);\n"
            "}\n";

        NSError* err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            Log::log_print(FATAL, "Metal blit shader compile: %s",
                           [[err localizedDescription] UTF8String]);
            return;
        }

        MTLRenderPipelineDescriptor* pd = [[MTLRenderPipelineDescriptor alloc] init];
        pd.vertexFunction   = [lib newFunctionWithName:@"blit_vertex"];
        pd.fragmentFunction = [lib newFunctionWithName:@"blit_fragment"];
        pd.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;

        blit_pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!blit_pipeline) {
            Log::log_print(FATAL, "Metal blit pipeline: %s",
                           [[err localizedDescription] UTF8String]);
        }
    }

    void ensure_display_texture(int w, int h) {
        if (display_texture && display_width == w && display_height == h)
            return;

        MTLTextureDescriptor* td =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                              width:w
                                                             height:h
                                                          mipmapped:NO];
        td.usage       = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModePrivate;
        display_texture = [device newTextureWithDescriptor:td];
        display_width   = w;
        display_height  = h;
    }

    uintptr_t blit_for_display(int w, int h) {
        if (w <= 0 || h <= 0)
            return (uintptr_t)(__bridge void*)render_texture;

        ensure_display_texture(w, h);

        @autoreleasepool {
            id<MTLCommandBuffer> cmd = [command_queue commandBuffer];

            MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
            rpd.colorAttachments[0].texture     = display_texture;
            rpd.colorAttachments[0].loadAction  = MTLLoadActionDontCare;
            rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

            id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
            [enc setRenderPipelineState:blit_pipeline];
            [enc setFragmentTexture:render_texture atIndex:0];
            [enc setFragmentSamplerState:sampler atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
            [enc endEncoding];

            [cmd commit];
            [cmd waitUntilCompleted];
        }

        return (uintptr_t)(__bridge void*)display_texture;
    }

    void build_depth_state() {
        MTLDepthStencilDescriptor* dd = [[MTLDepthStencilDescriptor alloc] init];
        dd.depthCompareFunction = MTLCompareFunctionLess;
        dd.depthWriteEnabled    = YES;
        depth_state = [device newDepthStencilStateWithDescriptor:dd];
    }

    void build_sampler() {
        MTLSamplerDescriptor* sd = [[MTLSamplerDescriptor alloc] init];
        sd.minFilter    = MTLSamplerMinMagFilterNearest;
        sd.magFilter    = MTLSamplerMinMagFilterNearest;
        sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
        sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
        sampler = [device newSamplerStateWithDescriptor:sd];
    }

    void build_quad() {
        const MetalVertex verts[4] = {
            {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
            {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        };
        const uint32_t indices[6] = {0, 1, 3, 1, 2, 3};

        quad_vb = [device newBufferWithBytes:verts
                                     length:sizeof(verts)
                                    options:MTLResourceStorageModeShared];
        quad_ib = [device newBufferWithBytes:indices
                                     length:sizeof(indices)
                                    options:MTLResourceStorageModeShared];
    }

    void build_render_targets() {
        MTLTextureDescriptor* td =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                              width:fb_width
                                                             height:fb_height
                                                          mipmapped:NO];
        td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModePrivate;
        render_texture = [device newTextureWithDescriptor:td];

        MTLTextureDescriptor* dd =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                              width:fb_width
                                                             height:fb_height
                                                          mipmapped:NO];
        dd.usage = MTLTextureUsageRenderTarget;
        dd.storageMode = MTLStorageModePrivate;
        depth_texture = [device newTextureWithDescriptor:dd];
    }

    // --- texture cache -------------------------------------------------------

    id<MTLTexture> get_texture_array(const std::shared_ptr<ImageAsset>& asset) {
        auto it = texture_cache.find(asset.get());
        if (it != texture_cache.end()) return it->second.texture;

        int w     = asset->width();
        int h     = asset->height();
        int count = asset->frame_count();
        if (w == 0 || h == 0 || count == 0) return nil;

        MTLTextureDescriptor* td = [[MTLTextureDescriptor alloc] init];
        td.textureType = MTLTextureType2DArray;
        td.pixelFormat = MTLPixelFormatRGBA8Unorm;
        td.width       = w;
        td.height      = h;
        td.arrayLength = count;
        td.usage       = MTLTextureUsageShaderRead;
        td.storageMode = MTLStorageModeShared;

        id<MTLTexture> tex = [device newTextureWithDescriptor:td];

        for (int i = 0; i < count; i++) {
            const auto& frame = asset->frame(i);
            int fw = std::min(frame.width, w);
            int fh = std::min(frame.height, h);
            MTLRegion region = MTLRegionMake2D(0, 0, fw, fh);
            [tex replaceRegion:region
                   mipmapLevel:0
                         slice:i
                     withBytes:frame.pixels.data()
                   bytesPerRow:fw * 4
                 bytesPerImage:fw * fh * 4];
        }

        Log::log_print(DEBUG, "MetalRenderer: uploaded %dx%d x %d frames for %s",
                       w, h, count, asset->path().c_str());

        texture_cache[asset.get()] = {asset, tex};
        return tex;
    }

    void evict_expired_textures() {
        for (auto it = texture_cache.begin(); it != texture_cache.end();) {
            if (it->second.asset.expired()) {
                Log::log_print(DEBUG, "MetalRenderer: evicting expired texture");
                it = texture_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // --- draw ----------------------------------------------------------------

    void draw(const RenderState* state) {
        @autoreleasepool {
            id<MTLCommandBuffer> cmd = [command_queue commandBuffer];

            MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
            rpd.colorAttachments[0].texture    = render_texture;
            rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
            rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
            rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.1, 0.2, 1.0);
            rpd.depthAttachment.texture     = depth_texture;
            rpd.depthAttachment.loadAction  = MTLLoadActionClear;
            rpd.depthAttachment.storeAction = MTLStoreActionDontCare;
            rpd.depthAttachment.clearDepth  = 1.0;

            id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
            [enc setRenderPipelineState:pipeline];
            [enc setDepthStencilState:depth_state];
            [enc setVertexBuffer:quad_vb offset:0 atIndex:0];
            [enc setFragmentSamplerState:sampler atIndex:0];

            if (++frame_counter % 60 == 0) {
                evict_expired_textures();
            }

            if (state) {
                for (const auto& [_, group] : state->get_layer_groups()) {
                    for (const auto& [__, layer] : group.get_layers()) {
                        const auto& asset = layer.get_asset();
                        if (!asset || asset->frame_count() == 0) continue;

                        id<MTLTexture> tex = get_texture_array(asset);
                        if (!tex) continue;

                        int frame = std::clamp(layer.get_frame_index(), 0,
                                               asset->frame_count() - 1);

                        Transform t;
                        t.zindex(layer.get_z_index() + 1);

                        VertexUniforms vu;
                        vu.local  = glm_to_simd(t.get_local_transform());
                        vu.aspect = Transform::get_aspect_ratio();

                        FragmentUniforms fu;
                        fu.frame_index = frame;

                        [enc setVertexBytes:&vu   length:sizeof(vu)   atIndex:1];
                        [enc setFragmentBytes:&fu length:sizeof(fu)   atIndex:0];
                        [enc setFragmentTexture:tex atIndex:0];

                        [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                        indexCount:6
                                         indexType:MTLIndexTypeUInt32
                                       indexBuffer:quad_ib
                                 indexBufferOffset:0];
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

MetalRenderer::MetalRenderer(int width, int height)
    : impl(std::make_unique<MetalRendererImpl>()) {
    impl->init(width, height);
}

MetalRenderer::~MetalRenderer() = default;

void MetalRenderer::draw(const RenderState* state) {
    impl->draw(state);
}

void MetalRenderer::bind_default_framebuffer() {
    // No-op for Metal — the app layer manages the drawable.
}

void MetalRenderer::clear() {
    // Clearing is done at the start of each draw() via loadAction.
}

uintptr_t MetalRenderer::get_render_texture_id() const {
    return (uintptr_t)(__bridge void*)impl->render_texture;
}

uintptr_t MetalRenderer::get_display_texture_id(int display_w, int display_h) {
    return impl->blit_for_display(display_w, display_h);
}

void* MetalRenderer::get_device_ptr() const {
    return (__bridge void*)impl->device;
}

void* MetalRenderer::get_command_queue_ptr() const {
    return (__bridge void*)impl->command_queue;
}

std::unique_ptr<IRenderer> create_renderer(int width, int height) {
    return std::make_unique<MetalRenderer>(width, height);
}
