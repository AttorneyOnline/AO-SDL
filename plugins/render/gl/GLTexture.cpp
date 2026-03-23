#include "GLTexture.h"

static int cached_max_texture_units() {
    static int val = [] {
        int v = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &v);
        return v;
    }();
    return val;
}

GLTexture::GLTexture(const uint8_t* pixels, GLint internal_format, GLenum source_format)
    : pixels(pixels), num_channels(0), internal_format(internal_format), source_format(source_format),
      max_texture_units(cached_max_texture_units()), texture(0) {
}

GLTexture::~GLTexture() {
    if (texture)
        glDeleteTextures(1, &texture);
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : pixels(other.pixels), num_channels(other.num_channels), internal_format(other.internal_format),
      source_format(other.source_format), max_texture_units(other.max_texture_units), texture(other.texture) {
    other.texture = 0;
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept {
    if (this != &other) {
        if (texture)
            glDeleteTextures(1, &texture);
        pixels = other.pixels;
        num_channels = other.num_channels;
        internal_format = other.internal_format;
        source_format = other.source_format;
        max_texture_units = other.max_texture_units;
        texture = other.texture;
        other.texture = 0;
    }
    return *this;
}

void GLTexture::set_parameter(GLenum target, GLenum param_name, GLint value) {
    glTexParameteri(target, param_name, value);
}

void GLTexture::activate(GLenum target, int texture_unit) {
    if (texture_unit >= max_texture_units) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(target, texture);
}

GLTexture2D::GLTexture2D(int width, int height, const uint8_t* pixels, GLint internal_format, GLenum source_format)
    : GLTexture(pixels, internal_format, source_format), width(width), height(height) {

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, source_format, GL_UNSIGNED_BYTE, pixels);
}

void GLTexture2D::set_parameter(GLenum param_name, GLint value) {
    GLTexture::set_parameter(GL_TEXTURE_2D, param_name, value);
}

void GLTexture2D::activate(int texture_unit) {
    GLTexture::activate(GL_TEXTURE_2D, texture_unit);
}

int GLTexture2D::get_width() {
    return width;
}

int GLTexture2D::get_height() {
    return height;
}
