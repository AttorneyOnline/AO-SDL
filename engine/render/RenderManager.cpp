#include "render/RenderManager.h"

#include "render/RenderState.h"

RenderManager::RenderManager(StateBuffer& buf, std::unique_ptr<IRenderer> renderer)
    : state_buf(buf), renderer(std::move(renderer)) {}

uint32_t RenderManager::render_frame() {
    const RenderState* state = state_buf.get_consumer_buf();
    uint32_t render_texture = renderer->draw(state);
    state_buf.update();

    return render_texture;
}

void RenderManager::begin_frame() {
    renderer->bind_default_framebuffer();
    renderer->clear();
}
