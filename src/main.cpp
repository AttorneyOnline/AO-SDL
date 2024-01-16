#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <SDL2/SDL.h>

#include <GL/glew.h>

#include "render/ShaderLibrary.h"
#include "utils/Log.h"

std::string read_file(std::string path) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();

    return buf.str();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        Log::log_print(LogLevel::FATAL, "Failed to create window: %s", SDL_GetError());
    }

    // OpenGL 3.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create context
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        Log::log_print(LogLevel::FATAL, "Failed to create OpenGL context: %s", SDL_GetError());
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize GLEW: %s", glewGetErrorString(glewError));
    }

    // Disable VSync
    if (SDL_GL_SetSwapInterval(0) < 0) {
        Log::log_print(LogLevel::ERROR, "Failed to disable VSync: %s", SDL_GetError());
    }

    GLuint program_id;
    GLuint texture;
    ShaderLibrary lib;
    GLuint vao, vbo, ebo;

    int num_attr;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_attr);
    Log::log_print(LogLevel::DEBUG, "Hardware supports %d vertex attributes", num_attr);

    try {
        int tex_width, tex_height, tex_num_channels;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        uint8_t* tex_data = stbi_load("C:\\Users\\Marisa\\source\\repos\\bangle\\assets\\extures\\container.jpg",
                                      &tex_width, &tex_height, &tex_num_channels, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(tex_data);

        program_id = lib.add_program("prog");

        std::string vert_shader = read_file("C:\\Users\\Marisa\\source\\repos\\bangle\\assets\\shaders\\vertex.glsl");
        const char* vert_shader_source = vert_shader.c_str();
        lib.add_shader("vert", GL_VERTEX_SHADER, &vert_shader_source);
        lib.attach_shader("vert", "prog");

        std::string frag_shader = read_file("C:\\Users\\Marisa\\source\\repos\\bangle\\assets\\shaders\\fragment.glsl");
        const char* frag_shader_source = frag_shader.c_str();
        lib.add_shader("frag", GL_FRAGMENT_SHADER, &frag_shader_source);
        lib.attach_shader("frag", "prog");

        // Link program
        lib.link_program("prog");

        GLint glsl_vertex_pos = glGetAttribLocation(program_id, "vertex_pos");
        GLint glsl_vertex_color = glGetAttribLocation(program_id, "vertex_color");
        GLint glsl_vertex_tex_coord = glGetAttribLocation(program_id, "vertex_tex_coord");

        // VBO data
        GLfloat vertices[] = {
            // positions        // colors         // texture coords
            0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
            0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
            -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
        };

        // EBO data
        GLuint indexData[] = {0, 1, 3, 1, 2, 3};

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        // Create VBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Create EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indexData, GL_STATIC_DRAW);

        // Setup vertex attributes

        // Position
        glVertexAttribPointer(glsl_vertex_pos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), NULL);
        glEnableVertexAttribArray(glsl_vertex_pos);

        // Color
        glVertexAttribPointer(glsl_vertex_color, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
                              (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(glsl_vertex_color);

        // Texture coordinate
        glVertexAttribPointer(glsl_vertex_tex_coord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
                              (void*)(6 * sizeof(GLfloat)));
        glEnableVertexAttribArray(glsl_vertex_tex_coord);

        glBindVertexArray(0);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    } catch (std::exception ex) {
        Log::log_print(LogLevel::ERROR, "Caught exception");
        return false;
    }

    // SDL main loop
    Log::log_print(LogLevel::DEBUG, "Entering main render loop");
    int frame_counter = 0;
    uint64_t cumulative_times = 0;
    bool run = true;
    while (run) {
        uint64_t frame_start = SDL_GetTicks64();
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                run = false;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(program_id, "texture_sample"), 0);

        glBindVertexArray(vao);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
        glBindVertexArray(0);

        glUseProgram(NULL);

        SDL_GL_SwapWindow(window);

        uint64_t frame_time = SDL_GetTicks64() - frame_start;
        cumulative_times += frame_time;
        frame_counter++;

        if (cumulative_times >= 1000) {
            float avg = (float)cumulative_times / (float)frame_counter;
            Log::log_print(LogLevel::INFO, "Average frame time: %fms (%f fps)", avg, (1.0f / avg) * 1000.0f);
            cumulative_times = 0;
            frame_counter = 0;
        }
    }

    glDeleteProgram(program_id);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
