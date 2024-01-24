#include "render/Shader.h"
#include "utils/log.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

GLShader::GLShader(ShaderType type, const std::string& path) : shader_type(type) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();

    std::string source_string = buf.str();
    const char* source_cstr = source_string.c_str();

    GLuint shader = glCreateShader(shader_type);
    GLint shader_compiled = GL_FALSE;

    glShaderSource(shader, 1, &source_cstr, NULL);

    // Compile vertex shader
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);
    if (shader_compiled != GL_TRUE) {
        Log::log_print(LogLevel::ERROR, "Unable to compile shader 0x%08X", shader);
        print_log();
        id = 0;
    }
    else {
        id = shader;
    }
}

GLShader::~GLShader() {
    Log::log_print(LogLevel::DEBUG, "GLShader %d destructor called", id);
    glDeleteShader(id);
}

void GLShader::print_log() {
    if (glIsShader(id)) {
        int info_log_len, max_len;

        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &max_len);
        char* info_log = (char*)malloc(max_len);

        glGetShaderInfoLog(id, max_len, &info_log_len, info_log);
        if (info_log_len > 0) {
            Log::log_print(LogLevel::DEBUG, "%s", info_log);
        }

        free(info_log);
    }
    else {
        Log::log_print(LogLevel::WARNING, "Could not print log for 0x%08X: not a shader!", id);
    }
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

bool GLProgram::link_shaders(std::vector<GLShader> shaders) {
    for (auto shader : shaders) {
        glAttachShader(id, shader.get_id());
    }

    GLint link_status = GL_FALSE;

    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        Log::log_print(LogLevel::ERROR, "Unable to link GL program %d", id);
        // TODO: print linker log
        return false;
    }

    return true;
}

void GLProgram::use() {
    glUseProgram(id);
}

void GLProgram::uniform_int(const std::string& name, GLint value) {
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void GLProgram::uniform_float(const std::string& name, GLfloat value) {
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

GLuint GLProgram::get_id() {
    return id;
}
