#pragma once

#include "ui/IUIRenderer.h"
#include "ui/controllers/IQtScreenController.h"

#include <QObject>

#include <string>
#include <unordered_map>

class UIManager;

/**
 * @brief Qt/QML implementation of the UI renderer lifecycle.
 *
 * QtUIRenderer owns a registry of IQtScreenController instances keyed by
 * screen ID.  On each event-loop tick the caller invokes update(), which:
 *
 *  1. Identifies the active screen from UIManager.
 *  2. Looks up the matching controller and calls sync() on it.
 *  3. Forwards any navActionRequested() signal from the controller.
 *
 * Unlike ImGuiUIRenderer, QtUIRenderer does not issue GPU draw calls —
 * visual rendering is handled declaratively by QML.  It satisfies the
 * IUIRenderer interface solely for interface compatibility; begin_frame(),
 * render_screen(), and end_frame() are no-ops.
 *
 * Controllers and models are registered in main.cpp (or QtGameWindow) via
 * registerController().  Each concrete controller exposes Q_PROPERTYs and
 * is set on the QML context as a named context property.
 */
class QtUIRenderer : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QtUIRenderer)

  public:
    explicit QtUIRenderer(UIManager& uiMgr, QObject* parent = nullptr);

    /**
     * @brief Register a controller for a given screen ID.
     *
     * @param screenId  The Screen::screen_id() value this controller handles.
     * @param controller Heap-allocated controller; ownership stays with caller.
     */
    void registerController(const std::string& screenId,
                            IQtScreenController* controller);

    /**
     * @brief Synchronise the active screen's controller.
     *
     * Must be called on every event-loop wake-up, after UIManager::handle_events().
     * Typically registered as an EngineEventBridge channel in main().
     */
    void update();

  signals:
    /**
     * @brief Forwarded from the active controller.
     *
     * Connect this to QtGameWindow::onNavAction() in main().
     */
    void navActionRequested(IUIRenderer::NavAction action);

  private:
    UIManager&    m_uiMgr;
    std::unordered_map<std::string, IQtScreenController*> m_controllers;
    std::string   m_activeScreenId;
};
