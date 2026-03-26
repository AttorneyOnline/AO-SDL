#include "QtUIRenderer.h"

#include "ui/Screen.h"
#include "ui/UIManager.h"

QtUIRenderer::QtUIRenderer(UIManager& uiMgr, QObject* parent)
    : QObject(parent)
    , m_uiMgr(uiMgr)
{}

void QtUIRenderer::registerController(const std::string& screenId,
                                      IQtScreenController* controller) {
    m_controllers[screenId] = controller;

    // Forward the controller's nav signals through this renderer so that
    // QtGameWindow only needs a single connection point.
    connect(controller, &IQtScreenController::navActionRequested,
            this,       &QtUIRenderer::navActionRequested);
}

void QtUIRenderer::update() {
    Screen* screen = m_uiMgr.active_screen();
    if (!screen)
        return;

    const std::string& id = screen->screen_id();

    // On screen transition, clear the cached ID so the next sync is not skipped.
    // (Controllers themselves handle per-screen state reset in sync().)
    if (id != m_activeScreenId)
        m_activeScreenId = id;

    auto it = m_controllers.find(id);
    if (it != m_controllers.end())
        it->second->sync(*screen);
}
