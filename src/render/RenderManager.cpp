#include "RenderManager.h"

#include "RenderState.h"
#include "detail/Renderer.h"

#include <gl/glew.h>

#include <cstdio>

RenderManager::RenderManager(StateBuffer& buf) : state_buf(buf), renderer_ptr(nullptr) {

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        // Log::log_print(LogLevel::FATAL, "Failed to initialize GLEW: %s", glewGetErrorString(glewError));
        printf("Failed to initialize GLEW: %s\n", glewGetErrorString(glewError));
    }

    renderer_ptr.reset(new Renderer);
}

uint32_t RenderManager::render_frame() {

    const RenderState* state = state_buf.get_consumer_buf();
    uint32_t render_texture = renderer_ptr->draw(state);
    state_buf.update();

    return render_texture;

    /*
    // uint64_t frame_time = SDL_GetTicks64() - frame_start;
    uint64_t frame_time = 10;
    cumulative_times += frame_time;
    frame_counter++;
    t++;

    if (cumulative_times >= 1000) {
        float avg = (float)cumulative_times / (float)frame_counter;
        // Log::log_print(LogLevel::INFO, "Average frame time: %fms (%f fps)", avg, (1.0f / avg) * 1000.0f);
        cumulative_times = 0;
        frame_counter = 0;
    }
    */
}

void RenderManager::clear_framebuffer() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
