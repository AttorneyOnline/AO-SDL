#include "RenderBridge.h"

#include "render/IRenderer.h"
#include "render/RenderManager.h"
#include "render/StateBuffer.h"
#include "utils/Log.h"

RenderBridge::RenderBridge(QObject* parent)
    : QObject(parent) {}

RenderBridge& RenderBridge::instance() {
    static RenderBridge bridge;
    return bridge;
}

void RenderBridge::setStateBuffer(StateBuffer* buf,
                                   int renderWidth, int renderHeight) {
    Log::debug("[RenderBridge] setStateBuffer ({}x{}, buf={})",
               renderWidth, renderHeight, static_cast<const void*>(buf));
    m_stateBuffer  = buf;
    m_renderWidth  = renderWidth;
    m_renderHeight = renderHeight;
}

void RenderBridge::setRenderManager(RenderManager* rm,
                                    int renderWidth, int renderHeight) {
    Log::debug("[RenderBridge] setRenderManager ({}x{}, rm={})",
               renderWidth, renderHeight, static_cast<const void*>(rm));
    m_renderManager = rm;
    m_renderWidth   = renderWidth;
    m_renderHeight  = renderHeight;
}

void RenderBridge::renderFrame() {
    if (m_renderManager)
        m_renderManager->render_frame();
}

uintptr_t RenderBridge::nativeTextureId() const {
    if (!m_renderManager)
        return 0;
    return m_renderManager->get_renderer().get_render_texture_id();
}

bool RenderBridge::uvFlipped() const {
    if (!m_renderManager)
        return false;
    return m_renderManager->get_renderer().uv_flipped();
}
