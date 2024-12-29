#ifndef RENDER_SHADER_H
#define RENDER_SHADER_H

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <GL/glew.h>

enum ShaderType { Vertex = GL_VERTEX_SHADER, Fragment = GL_FRAGMENT_SHADER, Compute = GL_COMPUTE_SHADER };

class GLShader {
  public:
    GLShader(ShaderType type, const std::string& path);
    ~GLShader();

    GLuint get_id();
    ShaderType get_shader_type();

  private:
    void print_log(GLuint shader);

    GLuint id;
    ShaderType shader_type;
};

class GLProgram {
  public:
    GLProgram();
    ~GLProgram();

    bool link_shaders(std::initializer_list<std::reference_wrapper<GLShader>> shaders);
    void use();

    void uniform(const std::string& name, GLint value);
    void uniform(const std::string& name, GLfloat value);
    void uniform(const std::string& name, glm::mat4 value);

    GLuint get_id();

  private:
    GLuint id;
};

#endif
