#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include <cstdint>

#include "render/Image.h"

#include <GL/glew.h>

class Texture {
  public:
    Texture(const uint8_t* pixels, GLint internal_format, GLenum source_format);

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

class Texture2D : Texture {
  public:
    Texture2D(Image img);
    Texture2D(int width, int height, const uint8_t* pixels, GLint internal_format, GLenum source_format);
    void activate(int texture_unit);

    void set_parameter(GLenum param_name, GLint value);

    int get_width();
    int get_height();

  private:
    int width;
    int height;
};

#endif
