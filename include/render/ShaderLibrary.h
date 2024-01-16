#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <map>
#include <string>

#include <GL/glew.h>

class ShaderLibrary {
  public:
    bool load_shader(std::string filename);
    bool add_shader(std::string name, GLenum shader_type, const GLchar* const* source);
    GLuint add_program(std::string name);
    bool attach_shader(std::string shader_name, std::string program_name);
    bool link_program(std::string name);

    GLuint get_shader(std::string name);
    GLuint get_program(std::string name);

  private:
    void print_shader_log(GLuint shader);

    struct ShaderInfo {
        GLuint shader;
        GLenum shader_type;
    };

    std::map<std::string, ShaderInfo> shader_map;
    std::map<std::string, GLuint> program_map;
};

// bool add_program(char* name);
// bool attach_shader(char* shader_name, char* program_name);
// bool link_program(char* name);

#endif
