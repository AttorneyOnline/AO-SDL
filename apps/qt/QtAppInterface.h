#pragma once

#include "ui/controllers/AudioController.h"
#include "ui/controllers/CharSelectController.h"
#include "ui/controllers/ChatController.h"
#include "ui/controllers/DebugController.h"
#include "ui/controllers/EvidenceController.h"
#include "ui/controllers/HUDController.h"
#include "ui/controllers/ICController.h"
#include "ui/controllers/MusicAreaController.h"
#include "ui/controllers/PlayerController.h"
#include "ui/controllers/ServerListController.h"

#include <QObject>
#include <QString>

#include <memory>

class EngineEventBridge;
class EngineInterface;
class QQmlApplicationEngine;
class QtImageWatcher;
class UIManager;

/**
 * @brief QObject singleton that wires engine events into the Qt event loop
 *        and exposes screen controllers to QML.
 *
 * Registered as the "app" QML context property.  Owns the UI-facing layer
 * (UIManager, controllers, EngineEventBridge) and holds a non-owning
 * reference to EngineInterface whose lifetime is managed by main().
 *
 * Controllers are split by concern:
 *   Global (permanent drain):  audioController, debugController
 *   Server list:               serverListController
 *   Char select:               charSelectController
 *   Courtroom (screen-scoped): chatController, icController, playerController,
 *                              evidenceController, musicAreaController, hudController
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

    // Navigation
    Q_PROPERTY(QString currentScreenId READ current_screen_id NOTIFY currentScreenIdChanged)

    // Global controllers
    Q_PROPERTY(AudioController* audioController READ audio_controller CONSTANT)
    Q_PROPERTY(DebugController* debugController READ debug_controller CONSTANT)

    // Server list
    Q_PROPERTY(ServerListController* serverListController READ server_list_controller CONSTANT)

    // Char select
    Q_PROPERTY(CharSelectController* charSelectController READ char_select_controller CONSTANT)

    // Courtroom sub-controllers
    Q_PROPERTY(ChatController* chatController READ chat_controller CONSTANT)
    Q_PROPERTY(ICController* icController READ ic_controller CONSTANT)
    Q_PROPERTY(PlayerController* playerController READ player_controller CONSTANT)
    Q_PROPERTY(EvidenceController* evidenceController READ evidence_controller CONSTANT)
    Q_PROPERTY(MusicAreaController* musicAreaController READ music_area_controller CONSTANT)
    Q_PROPERTY(HUDController* hudController READ hud_controller CONSTANT)

  public:
    explicit QtAppInterface(QObject* parent = nullptr);
    ~QtAppInterface() override;

    /**
     * @brief Construct owned objects and wire drain channels.
     *
     * Must be called exactly once from main(), before setup_qml().
     * Construction order is enforced internally (UIManager first,
     * ICController before CharSelectController, etc.).
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

    // --- Accessors ---
    AudioController* audio_controller() const;
    DebugController* debug_controller() const;
    ServerListController* server_list_controller() const;
    CharSelectController* char_select_controller() const;
    ChatController* chat_controller() const;
    ICController* ic_controller() const;
    PlayerController* player_controller() const;
    EvidenceController* evidence_controller() const;
    MusicAreaController* music_area_controller() const;
    HUDController* hud_controller() const;
    QString current_screen_id() const;

    /**
     * @brief Reset all courtroom controllers and navigate back to the server list.
     *
     * Called from QML (DisconnectModal, disconnect button) to cleanly tear down
     * the courtroom session.
     */
    Q_INVOKABLE void disconnect();

  signals:
    void currentScreenIdChanged();

  private:
    EngineInterface* engine_ = nullptr;

    std::unique_ptr<UIManager> ui_mgr_;

    // Global (permanent drain)
    std::unique_ptr<AudioController> audio_;
    std::unique_ptr<DebugController> dbg_;

    // Navigation controllers
    std::unique_ptr<ServerListController> sl_;
    std::unique_ptr<CharSelectController> cs_;

    // Courtroom sub-controllers
    std::unique_ptr<ChatController> chat_;
    std::unique_ptr<ICController> ic_;
    std::unique_ptr<PlayerController> players_;
    std::unique_ptr<EvidenceController> evidence_;
    std::unique_ptr<MusicAreaController> music_area_;
    std::unique_ptr<HUDController> hud_;

    std::unique_ptr<EngineEventBridge> bridge_;
    std::unique_ptr<QQmlApplicationEngine> qml_engine_;
    std::unique_ptr<QtImageWatcher> watcher_;

    QString current_screen_id_;
};
