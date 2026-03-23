// Stub Texture2D implementation for tests that don't have a GL context.
// Provides the symbols that CharSelectScreen etc. need without linking aorender_gl.

#include "render/Texture.h"

struct Texture2D::Impl {
    int width, height;
    Impl(int w, int h) : width(w), height(h) {
    }
};

Texture2D::Texture2D(int width, int height, const uint8_t*, int) : impl(std::make_unique<Impl>(width, height)) {
}

Texture2D::~Texture2D() = default;
Texture2D::Texture2D(Texture2D&&) noexcept = default;
Texture2D& Texture2D::operator=(Texture2D&&) noexcept = default;

uint64_t Texture2D::get_id() const {
    return 0;
}
int Texture2D::get_width() const {
    return impl->width;
}
int Texture2D::get_height() const {
    return impl->height;
}
