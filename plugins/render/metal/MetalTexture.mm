#import "MetalTexture.h"
#import <Metal/Metal.h>

#include <vector>

// Shared device reference — set once from MetalRenderer init.
// We use the system default device since there is only one on macOS/iOS.
static id<MTLDevice> shared_device() {
    static id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
    return dev;
}

MetalTexture2D::MetalTexture2D(int width, int height, const uint8_t *pixels, int channels)
    : width_(width), height_(height) {

    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                  width:width
                                                                                 height:height
                                                                              mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead;
    td.storageMode = MTLStorageModeShared;

    texture_ = [shared_device() newTextureWithDescriptor:td];

    if (pixels) {
        // If source is RGB (3 channels), we need to expand to RGBA.
        if (channels == 3) {
            std::vector<uint8_t> rgba(width * height * 4);
            for (int i = 0; i < width * height; i++) {
                rgba[i * 4 + 0] = pixels[i * 3 + 0];
                rgba[i * 4 + 1] = pixels[i * 3 + 1];
                rgba[i * 4 + 2] = pixels[i * 3 + 2];
                rgba[i * 4 + 3] = 255;
            }
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [texture_ replaceRegion:region mipmapLevel:0 withBytes:rgba.data() bytesPerRow:width * 4];
        } else {
            MTLRegion region = MTLRegionMake2D(0, 0, width, height);
            [texture_ replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:width * 4];
        }
    }
}

MetalTexture2D::~MetalTexture2D() {
    // ARC handles release
}

uint64_t MetalTexture2D::get_id() const { return (uint64_t)(uintptr_t)(__bridge void *)texture_; }

int MetalTexture2D::get_width() const { return width_; }

int MetalTexture2D::get_height() const { return height_; }
