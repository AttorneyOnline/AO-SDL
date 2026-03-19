#include "RenderManager.h"

#include "RenderState.h"
#include "gl/GLRenderer.h"

#include <cstdio>

RenderManager::RenderManager(StateBuffer& buf) : state_buf(buf), renderer_ptr(nullptr) {
    GLRenderer::init_gl();
    renderer_ptr.reset(new GLRenderer);
}

uint32_t RenderManager::render_frame() {
    const RenderState* state = state_buf.get_consumer_buf();
    uint32_t render_texture = renderer_ptr->draw(state);
    state_buf.update();

    return render_texture;
}

void RenderManager::begin_frame() {
    renderer_ptr->bind_default_framebuffer();
    renderer_ptr->clear();
}
