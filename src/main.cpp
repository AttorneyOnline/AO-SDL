#include <cmath>
#include <ctime>

#include <SDL2/SDL.h>

#include <GL/glew.h>

#include "render/Shader.h"
#include "render/Sprite.h"
#include "render/Texture.h"
#include "render/Transform.h"
#include "utils/Log.h"

int main(int argc, char* argv[]) {
    srand(time(0));
    Log::log_print(LogLevel::DEBUG, "test %d %f", 1, 0.5f);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Log::log_print(LogLevel::FATAL, "Failed to initialize SDL2: %s", SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    Transform::set_aspect_ratio(640.0f / 480.0f);

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

    // Enable VSync
    if (SDL_GL_SetSwapInterval(0) < 0) {
        Log::log_print(LogLevel::ERROR, "Failed to enable VSync: %s", SDL_GetError());
    }

    GLProgram program;
    Texture2D bgtex("C:\\Users\\Marisa\\Documents\\saiban\\assets\\base\\background\\default\\defenseempty.png", GL_RGB,
                    GL_RGB);
    Texture2D charatex("C:\\Users\\Marisa\\Documents\\saiban\\assets\\base\\characters\\Phoenix\\nope.apng", GL_RGBA,
                       GL_RGBA);
    Texture2D fgtex("C:\\Users\\Marisa\\Documents\\saiban\\assets\\base\\background\\default\\defensedesk.png", GL_RGBA,
                    GL_RGBA);

    GLShader vert(ShaderType::Vertex, "C:\\Users\\Marisa\\Documents\\saiban\\assets\\shaders\\vertex.glsl");
    GLShader frag(ShaderType::Fragment, "C:\\Users\\Marisa\\Documents\\saiban\\assets\\shaders\\fragment.glsl");
    program.link_shaders({vert, frag});

    Sprite bg(bgtex);
    Sprite ch(charatex);
    Sprite fg(fgtex);

    bg.zindex(0);
    ch.zindex(1);
    fg.zindex(2);

    /*
    std::vector<Sprite> sprites;
    for (int i = 0; i < 1000; i++) {
        Sprite sprite(charatex);
        sprite.scale({0.05f, 0.05f});
        float randf1 = ((((float)rand()) / RAND_MAX) * 2) - 1.0f;
        float randf2 = ((((float)rand()) / RAND_MAX) * 2) - 1.0f;
        sprite.translate({randf1, randf2});
        sprite.zindex(i);
        sprites.push_back(sprite);
    }
    */

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

    // SDL main loop
    Log::log_print(LogLevel::DEBUG, "Entering main render loop");
    int frame_counter = 0;
    uint64_t t = 0;
    uint64_t cumulative_times = 0;
    bool run = true;
    while (run) {
        uint64_t frame_start = SDL_GetTicks64();
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                run = false;
            }
            else if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    int w = ev.window.data1;
                    int h = ev.window.data2;

                    glViewport(0, 0, w, h);
                    Transform::set_aspect_ratio((float)w / (float)h);
                }
            }
        }

        // sprite.scale({glm::sin(glm::radians((float)t)), glm::sin(glm::radians((float)t))});
        // sprite.rotate(t);
        // sprite.translate({glm::sin(glm::radians((float)t)), glm::cos(glm::radians((float)t))});

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int n = 0; n < 100; n++) {
            Texture2D badtex("C:\\Users\\Marisa\\Documents\\saiban\\assets\\base\\evidence\\cardkey.png", GL_RGBA,
                             GL_RGBA);
            Sprite badsprite(badtex);
            badsprite.scale({0.25f, 0.25f});
            float randf1 = ((((float)rand()) / RAND_MAX) * 2) - 1.0f;
            float randf2 = ((((float)rand()) / RAND_MAX) * 2) - 1.0f;
            badsprite.translate({randf1, randf2});
            badsprite.rotate(45);
            badsprite.draw(program);
        }
        // fg.draw(program);
        // ch.draw(program);
        bg.draw(program);

        /*
        for (auto sprite : sprites) {
            // sprite.rotate(t);
            sprite.draw(program);
        }
        */

        // bg.scale({glm::sin(glm::radians((float)t)), glm::sin(glm::radians((float)t))});
        // ch.scale({glm::sin(glm::radians((float)t)), glm::sin(glm::radians((float)t))});
        // fg.scale({glm::sin(glm::radians((float)t)), glm::sin(glm::radians((float)t))});

        // bg.translate({glm::sin(glm::radians((float)t)), 0.0f});
        // ch.translate({glm::sin(glm::radians((float)t)), 0.0f});
        // fg.translate({glm::sin(glm::radians((float)t)), 0.0f});

        // bg.rotate(t);
        // ch.rotate(t);
        // fg.rotate(t);

        SDL_GL_SwapWindow(window);

        uint64_t frame_time = SDL_GetTicks64() - frame_start;
        cumulative_times += frame_time;
        frame_counter++;
        t++;

        if (cumulative_times >= 1000) {
            float avg = (float)cumulative_times / (float)frame_counter;
            Log::log_print(LogLevel::INFO, "Average frame time: %fms (%f fps)", avg, (1.0f / avg) * 1000.0f);
            cumulative_times = 0;
            frame_counter = 0;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
