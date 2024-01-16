#include "render/ShaderLibrary.h"
#include "utils/log.h"

#include <cstdlib>
#include <stdexcept>

void ShaderLibrary::print_shader_log(GLuint shader) {
    if (glIsShader(shader)) {
        int info_log_len, max_len;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);
        char* info_log = (char*)malloc(max_len);

        glGetShaderInfoLog(shader, max_len, &info_log_len, info_log);
        if (info_log_len > 0) {
            Log::log_print(LogLevel::DEBUG, "%s", info_log);
        }

        free(info_log);
    } else {
        Log::log_print(LogLevel::WARNING, "Could not print log for 0x%08X: not a shader!", shader);
    }
}

bool ShaderLibrary::add_shader(std::string name, GLenum shader_type, const GLchar* const* source) {
    GLuint shader = glCreateShader(shader_type);
    GLint shader_compiled = GL_FALSE;

    glShaderSource(shader, 1, source, NULL);

    // Compile vertex shader
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);
    if (shader_compiled != GL_TRUE) {
        Log::log_print(LogLevel::ERROR, "Unable to compile shader 0x%08X", shader);
        print_shader_log(shader);
        return false;
    }

    ShaderInfo shader_info;
    shader_info.shader = shader;
    shader_info.shader_type = shader_type;
    shader_map.insert({name, shader_info});

    return true;
}

GLuint ShaderLibrary::add_program(std::string name) {
    GLuint program_id = glCreateProgram();
    program_map.insert({name, program_id});

    return program_id;
}

bool ShaderLibrary::attach_shader(std::string shader_name, std::string program_name) {
    GLuint program, shader;

    try {
        shader = get_shader(shader_name);
    } catch (std::out_of_range ex) {
        Log::log_print(LogLevel::ERROR, "Could not attach shader %s to program %s: %s not found", shader_name.c_str(),
                       program_name.c_str(), shader_name.c_str());
        return false;
    }

    try {
        program = get_program(program_name);
    } catch (std::out_of_range ex) {
        Log::log_print(LogLevel::ERROR, "Could not attach shader %s to program %s: %s not found", shader_name.c_str(),
                       program_name.c_str(), program_name.c_str());
        return false;
    }

    glAttachShader(program, shader);

    return true;
}

bool ShaderLibrary::link_program(std::string name) {
    GLuint program;

    try {
        program = get_program(name);
    } catch (std::out_of_range ex) {
        Log::log_print(LogLevel::ERROR, "Unable to link GL program %s: %s not found", name.c_str(), name.c_str());
    }

    glLinkProgram(program);
    GLint program_success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &program_success);
    if (program_success != GL_TRUE) {
        Log::log_print(LogLevel::ERROR, "Unable to link GL program %s", name.c_str());
        // TODO: print linker log
        return false;
    }

    return true;
}

GLuint ShaderLibrary::get_shader(std::string name) {
    auto pos = shader_map.find(name);
    if (pos == shader_map.end()) {
        throw std::out_of_range("name");
    }

    return pos->second.shader;
}

GLuint ShaderLibrary::get_program(std::string name) {
    auto pos = program_map.find(name);
    if (pos == program_map.end()) {
        throw std::out_of_range("name");
    }

    return pos->second;
}
