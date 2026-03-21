#include "IGPUBackend.h"
#include "render/IRenderer.h"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_metal.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

class MetalBackend : public IGPUBackend {
  public:
    uint32_t window_flags() const override {
        return SDL_WINDOW_METAL;
    }

    void init(SDL_Window* window, IRenderer& renderer) override {
        window_ = window;
        metal_view_ = SDL_Metal_CreateView(window);

        device_ = (__bridge id<MTLDevice>)renderer.get_device_ptr();
        queue_  = (__bridge id<MTLCommandQueue>)renderer.get_command_queue_ptr();

        layer_ = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(metal_view_);
        layer_.device = device_;
        layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;

        ImGui_ImplSDL2_InitForMetal(window);
        ImGui_ImplMetal_Init(device_);
    }

    void shutdown() override {
        ImGui_ImplMetal_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        if (metal_view_) SDL_Metal_DestroyView(metal_view_);
    }

    void begin_frame() override {
        pool_ = [[NSAutoreleasePool alloc] init];

        int w, h;
        SDL_Metal_GetDrawableSize(window_, &w, &h);
        layer_.drawableSize = CGSizeMake(w, h);

        drawable_ = [layer_ nextDrawable];

        rpd_ = [MTLRenderPassDescriptor renderPassDescriptor];
        rpd_.colorAttachments[0].texture     = drawable_.texture;
        rpd_.colorAttachments[0].loadAction  = MTLLoadActionClear;
        rpd_.colorAttachments[0].storeAction = MTLStoreActionStore;
        rpd_.colorAttachments[0].clearColor  = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

        ImGui_ImplMetal_NewFrame(rpd_);
        ImGui_ImplSDL2_NewFrame();
    }

    void present() override {
        if (!drawable_) return;

        id<MTLCommandBuffer> cmd = [queue_ commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd_];
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, enc);
        [enc endEncoding];
        [cmd presentDrawable:drawable_];
        [cmd commit];

        drawable_ = nil;
        rpd_ = nil;

        [pool_ drain];
        pool_ = nil;
    }

  private:
    SDL_Window*          window_     = nullptr;
    SDL_MetalView        metal_view_ = nullptr;
    id<MTLDevice>        device_     = nil;
    id<MTLCommandQueue>  queue_      = nil;
    CAMetalLayer*        layer_      = nil;
    id<CAMetalDrawable>  drawable_   = nil;
    MTLRenderPassDescriptor* rpd_    = nil;
    NSAutoreleasePool*   pool_      = nil;
};

std::unique_ptr<IGPUBackend> create_gpu_backend() {
    return std::make_unique<MetalBackend>();
}
