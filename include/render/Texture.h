#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include <string>

#include <GL/glew.h>

class Texture {
  public:
    Texture(std::string path, GLint internal_format, GLenum source_format);

  protected:
    void set_parameter(GLenum target, GLenum param_name, GLint value);
    void activate(GLenum target, int texture_unit);

    std::string path;

    int num_channels;
    GLint internal_format;
    GLenum source_format;

    int max_texture_units;

    GLuint texture;
};

class Texture2D : Texture {
  public:
    Texture2D(std::string path, GLint internal_format, GLenum source_format);
    void activate(int texture_unit);

    void set_parameter(GLenum param_name, GLint value);

  private:
    int width;
    int height;
};

#endif
