#include "Texture.h"

Texture::Texture(const uint8_t* pixels, GLint internal_format, GLenum source_format)
    : pixels(pixels), internal_format(internal_format), source_format(source_format), num_channels(0), texture(0) {
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
    // Log::log_print(LogLevel::DEBUG, "%d texture units available", max_texture_units);
}

void Texture::set_parameter(GLenum target, GLenum param_name, GLint value) {
    glTexParameteri(target, param_name, value);
}

void Texture::activate(GLenum target, int texture_unit) {
    if (texture_unit >= max_texture_units) {
        // Log::log_print(LogLevel::ERROR, "Could not activate texture unit %d (not enough texture units!)",
        // texture_unit);
        return;
    }

    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(target, texture);
}

Texture2D::Texture2D(Image img)
    : Texture2D(img.get_width(), img.get_height(), img.get_pixels(), GL_RGBA,
                img.get_num_channels() == 3 ? GL_RGB : GL_RGBA) {
}

Texture2D::Texture2D(int width, int height, const uint8_t* pixels, GLint internal_format, GLenum source_format)
    : Texture(pixels, internal_format, source_format), width(width), height(height) {

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    set_parameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // stbi_set_flip_vertically_on_load(true);
    // uint8_t* tex_data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, source_format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    // stbi_image_free(tex_data);
}

void Texture2D::set_parameter(GLenum param_name, GLint value) {
    Texture::set_parameter(GL_TEXTURE_2D, param_name, value);
}

void Texture2D::activate(int texture_unit) {
    Texture::activate(GL_TEXTURE_2D, texture_unit);
}

int Texture2D::get_width() {
    return width;
}

int Texture2D::get_height() {
    return height;
}
