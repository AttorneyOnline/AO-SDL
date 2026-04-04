#pragma once

#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/CourtroomController.h"
#include "ui/controllers/DebugController.h"
#include "ui/controllers/ServerListController.h"

#include <QObject>
#include <QString>

#include <memory>

class EngineEventBridge;
class EngineInterface;
class QQmlApplicationEngine;
class UIManager;

/**
 * @brief QObject singleton that wires engine events into the Qt event loop
 *        and exposes screen controllers to QML.
 *
 * Registered as the "app" QML context property.  Owns the UI-facing layer
 * (UIManager, controllers, EngineEventBridge) and holds a non-owning
 * reference to EngineInterface whose lifetime is managed by main().
 *
 * The SDL frontend keeps equivalent wiring in main() because SDL's event
 * loop is an explicit poll the application owns.  Qt's loop is framework-
 * managed (app.exec()), so this class bridges engine events into it via
 * EngineEventBridge and structures the QML-facing surface.
 *
 * Deliberately thin — no business logic.  The engine does the real work.
 *
 * Lifecycle:
 *   1. main() constructs EngineInterface on the stack.
 *   2. main() calls init(engine).
 *   3. main() calls setup_qml() to create the QML engine and load the module.
 *   4. main() calls start() to connect the bridge to the Qt event dispatcher.
 *   5. app.exec() runs.
 *   6. main() calls stop().
 *   7. main() calls engine.stop() (EngineInterface outlives this object).
 */
class QtAppInterface : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QtAppInterface)

    Q_PROPERTY(ServerListController* serverListController READ server_list_controller CONSTANT)
    Q_PROPERTY(CharSelectController* charSelectController READ char_select_controller CONSTANT)
    Q_PROPERTY(CourtroomController* courtroomController READ courtroom_controller CONSTANT)
    Q_PROPERTY(DebugController* debugController READ debug_controller CONSTANT)
    Q_PROPERTY(QString currentScreenId READ current_screen_id NOTIFY currentScreenIdChanged)

  public:
    static QtAppInterface& instance();
    ~QtAppInterface() override;

    /**
     * @brief Construct owned objects and wire drain channels.
     *
     * Must be called exactly once from main(), before setup_qml().
     * Construction order is enforced internally (UIManager first,
     * CourtroomController before CharSelectController, etc.).
     */
    void init(EngineInterface& engine);

    /**
     * @brief Create the QQmlApplicationEngine, register context properties
     *        and image providers, and load the QML module.
     *
     * @return true if QML loaded successfully (root objects non-empty).
     */
    bool setup_qml();

    /** Connect the EngineEventBridge to the Qt event dispatcher. */
    void start();

    /** Disconnect the bridge. */
    void stop();

    void sync_current_screen_id();

    ServerListController* server_list_controller() const;
    CharSelectController* char_select_controller() const;
    CourtroomController* courtroom_controller() const;
    DebugController* debug_controller() const;
    QString current_screen_id() const;

  signals:
    void currentScreenIdChanged();

  private:
    explicit QtAppInterface(QObject* parent = nullptr);

    EngineInterface* engine_ = nullptr;

    std::unique_ptr<UIManager> ui_mgr_;
    std::unique_ptr<CourtroomController> cr_;
    std::unique_ptr<CharSelectController> cs_;
    std::unique_ptr<ServerListController> sl_;
    std::unique_ptr<DebugController> dbg_;
    std::unique_ptr<EngineEventBridge> bridge_;
    std::unique_ptr<QQmlApplicationEngine> qml_engine_;

    QString current_screen_id_;
};
