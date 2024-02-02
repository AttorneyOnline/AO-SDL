#include "render/Texture.h"

#include "utils/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture(std::string path, GLint internal_format, GLenum source_format)
    : path(path), internal_format(internal_format), source_format(source_format), num_channels(0), texture(0) {
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
    Log::log_print(LogLevel::DEBUG, "%d texture units available", max_texture_units);
}

void Texture::set_parameter(GLenum target, GLenum param_name, GLint value) {
    glTexParameteri(target, param_name, value);
}

void Texture::activate(GLenum target, int texture_unit) {
    if (texture_unit >= max_texture_units) {
        Log::log_print(LogLevel::ERROR, "Could not activate texture unit %d (not enough texture units!)", texture_unit);
        return;
    }

    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(target, texture);
}

Texture2D::Texture2D(std::string path, GLint internal_format, GLenum source_format)
    : Texture(path, internal_format, source_format) {

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    uint8_t* tex_data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, source_format, GL_UNSIGNED_BYTE, tex_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(tex_data);
}

void Texture2D::set_parameter(GLenum param_name, GLint value) {
    Texture::set_parameter(GL_TEXTURE_2D, param_name, value);
}

void Texture2D::activate(int texture_unit) {
    Texture::activate(GL_TEXTURE_2D, texture_unit);
}
