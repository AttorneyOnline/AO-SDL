#include "Shader.h"


#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

static GLuint compile_shader(GLenum type, const std::string& source) {
    const char* source_cstr = source.c_str();
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source_cstr, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        // Print log and return 0
        if (glIsShader(shader)) {
            int max_len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);
            std::string log(max_len, '\0');
            glGetShaderInfoLog(shader, max_len, nullptr, log.data());
            std::cerr << "Shader compile error: " << log << std::endl;
        }
        return 0;
    }
    return shader;
}

GLShader::GLShader(ShaderType type, const std::string& path) : shader_type(type) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();
    id = compile_shader(shader_type, buf.str());
}

GLShader::GLShader(ShaderType type, const std::string& source, bool /*from_source*/) : shader_type(type) {
    id = compile_shader(shader_type, source);
}

GLShader::~GLShader() {
    glDeleteShader(id);
}

GLuint GLShader::get_id() {
    return id;
}

ShaderType GLShader::get_shader_type() {
    return shader_type;
}

GLProgram::GLProgram() : id(glCreateProgram()) {
}

GLProgram::~GLProgram() {
    glDeleteProgram(id);
}

bool GLProgram::link_shaders(std::initializer_list<std::reference_wrapper<GLShader>> shaders) {
    for (GLShader& shader : shaders) {
        glAttachShader(id, shader.get_id());
    }

    GLint link_status = GL_FALSE;

    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        return false;
    }

    return true;
}

void GLProgram::use() {
    glUseProgram(id);
}

void GLProgram::uniform(const std::string& name, GLint value) {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void GLProgram::uniform(const std::string& name, GLfloat value) {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

void GLProgram::uniform(const std::string& name, Mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, value.data());
}

GLuint GLProgram::get_id() {
    return id;
}
