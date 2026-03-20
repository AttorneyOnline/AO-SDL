#include "render/RenderManager.h"

#include "render/RenderState.h"

RenderManager::RenderManager(StateBuffer& buf, std::unique_ptr<IRenderer> renderer)
    : state_buf(buf), renderer(std::move(renderer)) {
}

void RenderManager::render_frame() {
    const RenderState* state = state_buf.get_consumer_buf();
    renderer->draw(state);
    state_buf.update();
}

void RenderManager::begin_frame() {
    renderer->bind_default_framebuffer();
    renderer->clear();
}
