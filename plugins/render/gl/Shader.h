#pragma once

#include "render/Math.h"

#include <GL/glew.h>

#include <string>
#include <vector>

enum ShaderType { Vertex = GL_VERTEX_SHADER, Fragment = GL_FRAGMENT_SHADER, Compute = GL_COMPUTE_SHADER };

class GLShader {
  public:
    /// Load shader source from a file path.
    GLShader(ShaderType type, const std::string& path);

    /// Compile shader from source code directly.
    GLShader(ShaderType type, const std::string& source, bool from_source);

    ~GLShader();

    GLuint get_id();
    ShaderType get_shader_type();

  private:
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
    void uniform(const std::string& name, Vec2 value);
    void uniform(const std::string& name, Vec3 value);
    void uniform(const std::string& name, Mat4 value);

    GLuint get_id();

  private:
    GLuint id;
};
