#pragma once

#include <QObject>

/**
 * @brief Abstract base for Qt/QML screen controllers.
 *
 * Each concrete controller owns its Qt models and Q_PROPERTYs.
 * Navigation is performed by calling UIManager::push_screen() /
 * pop_screen() / pop_to_root() directly — no nav signal needed.
 *
 * Lifecycle:
 *  - Controllers are created once at startup and live for the process.
 *  - drain() is called on every event-loop wakeup by EngineEventBridge,
 *    regardless of which screen is currently active.  Implementations
 *    must be fast and non-blocking; they pull from EventChannels only.
 */
class IQtScreenController : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(IQtScreenController)

  public:
    explicit IQtScreenController(QObject* parent = nullptr) : QObject(parent) {
    }

    ~IQtScreenController() override = default;

    /**
     * @brief Drain pending engine events and update Qt models.
     *
     * Called by EngineEventBridge on every event-loop wakeup.
     * Must run on the main thread.  Must not block.
     */
    virtual void drain() = 0;
};
