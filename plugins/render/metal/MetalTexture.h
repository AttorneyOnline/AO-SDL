#pragma once

#include <cstdint>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

/// Wraps an MTLTexture for use by the Texture2D pimpl on the Metal backend.
class MetalTexture2D {
  public:
    MetalTexture2D(int width, int height, const uint8_t* pixels, int channels);
    ~MetalTexture2D();

    uint64_t get_id() const;
    int get_width() const;
    int get_height() const;

  private:
    int width_;
    int height_;
#ifdef __OBJC__
    id<MTLTexture> texture_;
#else
    void* texture_;
#endif
};
