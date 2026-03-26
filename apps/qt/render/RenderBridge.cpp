#include "RenderBridge.h"

RenderBridge::RenderBridge(QObject* parent)
    : QObject(parent) {}

RenderBridge& RenderBridge::instance() {
    static RenderBridge bridge;
    return bridge;
}

void RenderBridge::setRenderManager(RenderManager* rm) {
    m_renderManager = rm;
}
