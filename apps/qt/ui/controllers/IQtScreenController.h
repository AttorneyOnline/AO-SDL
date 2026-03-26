#pragma once

#include "ui/IUIRenderer.h"

#include <QObject>

class Screen;

/**
 * @brief Abstract base for Qt/QML screen controllers.
 *
 * Mirrors the SDL IScreenController pattern but as a QObject so that
 * controllers can expose Q_PROPERTYs and signals directly to QML.
 *
 * Each concrete controller (ServerListController, CharSelectController,
 * CourtroomController) owns its Qt models and Q_PROPERTYs.
 *
 * Lifecycle:
 *  - Controllers are created once at startup and registered with QtUIRenderer.
 *  - sync() is called every event-loop tick when this screen is the active one.
 *  - Controllers emit navActionRequested() when the user triggers navigation.
 */
class IQtScreenController : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(IQtScreenController)

  public:
    explicit IQtScreenController(QObject* parent = nullptr)
        : QObject(parent) {}

    ~IQtScreenController() override = default;

    /**
     * @brief Synchronise Qt models and properties from the active Screen state.
     *
     * Called by QtUIRenderer::update() each event-loop tick while this screen
     * is on top of the UIManager stack.  Implementations must not block.
     *
     * @param screen The active Screen whose state should be mirrored.
     */
    virtual void sync(Screen& screen) = 0;

  signals:
    /**
     * @brief Emitted when the user triggers a navigation action.
     *
     * Connected by QtUIRenderer to its own navActionRequested() signal,
     * which in turn is connected to QtGameWindow::onNavAction().
     */
    void navActionRequested(IUIRenderer::NavAction action);
};
