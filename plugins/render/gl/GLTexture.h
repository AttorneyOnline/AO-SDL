#pragma once

#include <GL/glew.h>

#include <cstdint>

class GLTexture {
  public:
    GLTexture(const uint8_t* pixels, GLint internal_format, GLenum source_format);
    virtual ~GLTexture();

    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;
    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

  protected:
    void set_parameter(GLenum target, GLenum param_name, GLint value);
    void activate(GLenum target, int texture_unit);

    const uint8_t* pixels;

    int num_channels;
    GLint internal_format;
    GLenum source_format;

    int max_texture_units;

    GLuint texture;
};

class GLTexture2D : public GLTexture {
  public:
    GLTexture2D(int width, int height, const uint8_t* pixels, GLint internal_format, GLenum source_format);
    void activate(int texture_unit);

    void set_parameter(GLenum param_name, GLint value);

    int get_width();
    int get_height();
    GLuint get_id() const {
        return texture;
    }

  private:
    int width;
    int height;
};
