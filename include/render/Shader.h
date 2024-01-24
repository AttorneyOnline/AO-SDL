#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <string>
#include <vector>

#include <GL/glew.h>

enum ShaderType { Vertex = GL_VERTEX_SHADER, Fragment = GL_FRAGMENT_SHADER, Compute = GL_COMPUTE_SHADER };

class GLShader {
  public:
    GLShader(ShaderType type, const std::string& path);
    ~GLShader();

    GLuint get_id();
    ShaderType get_shader_type();

  private:
    void print_log();

    GLuint id;
    ShaderType shader_type;
};

class GLProgram {
  public:
    GLProgram();
    ~GLProgram();

    bool link_shaders(std::vector<GLShader> shaders);
    void use();

    void uniform_int(const std::string& name, GLint value);
    void uniform_float(const std::string& name, GLfloat value);

    GLuint get_id();

  private:
    GLuint id;
};

#endif
