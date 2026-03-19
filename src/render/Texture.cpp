#include "Texture.h"

#include "render/gl/GLTexture.h"

struct Texture2D::Impl {
    GLTexture2D gl_texture;

    Impl(int width, int height, const uint8_t* pixels, int channels)
        : gl_texture(width, height, pixels, GL_RGBA, channels == 3 ? GL_RGB : GL_RGBA) {
    }
};

Texture2D::Texture2D(int width, int height, const uint8_t* pixels, int channels)
    : impl(std::make_unique<Impl>(width, height, pixels, channels)) {
}

Texture2D::~Texture2D() = default;

Texture2D::Texture2D(Texture2D&&) noexcept = default;
Texture2D& Texture2D::operator=(Texture2D&&) noexcept = default;

uint64_t Texture2D::get_id() const {
    return impl->gl_texture.get_id();
}

int Texture2D::get_width() const {
    return impl->gl_texture.get_width();
}

int Texture2D::get_height() const {
    return impl->gl_texture.get_height();
}
