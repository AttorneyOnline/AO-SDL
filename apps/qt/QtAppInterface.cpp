#include "QtAppInterface.h"

#include "EngineEventBridge.h"
#include "EngineInterface.h"
#include "asset/MediaManager.h"
#include "ui/CharIconProvider.h"
#include "ui/EmoteIconProvider.h"
#include "ui/QtImageWatcher.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

#include "ao/ui/screens/ServerListScreen.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>

QtAppInterface::QtAppInterface(QObject* parent) : QObject(parent) {
}

QtAppInterface::~QtAppInterface() = default;

AudioController* QtAppInterface::audio_controller() const { return audio_.get(); }
DebugController* QtAppInterface::debug_controller() const { return dbg_.get(); }
ServerListController* QtAppInterface::server_list_controller() const { return sl_.get(); }
CharSelectController* QtAppInterface::char_select_controller() const { return cs_.get(); }
ChatController* QtAppInterface::chat_controller() const { return chat_.get(); }
ICController* QtAppInterface::ic_controller() const { return ic_.get(); }
PlayerController* QtAppInterface::player_controller() const { return players_.get(); }
EvidenceController* QtAppInterface::evidence_controller() const { return evidence_.get(); }
MusicAreaController* QtAppInterface::music_area_controller() const { return music_area_.get(); }
HUDController* QtAppInterface::hud_controller() const { return hud_.get(); }
QString QtAppInterface::current_screen_id() const { return current_screen_id_; }

void QtAppInterface::init(EngineInterface& engine) {
    engine_ = &engine;

    ui_mgr_ = std::make_unique<UIManager>();

    audio_ = std::make_unique<AudioController>();
    dbg_ = std::make_unique<DebugController>();

    // ICController needs UIManager for sheet polling; CharSelectController depends on ICController.
    ic_ = std::make_unique<ICController>(*ui_mgr_);
    chat_ = std::make_unique<ChatController>();
    players_ = std::make_unique<PlayerController>();
    evidence_ = std::make_unique<EvidenceController>();
    music_area_ = std::make_unique<MusicAreaController>();
    hud_ = std::make_unique<HUDController>();

    sl_ = std::make_unique<ServerListController>(*ui_mgr_);
    cs_ = std::make_unique<CharSelectController>(*ui_mgr_, *ic_);

    ui_mgr_->push_screen(std::make_unique<ServerListScreen>());
    Log::debug("[QtAppInterface] controllers created");

    watcher_ = std::make_unique<QtImageWatcher>(MediaManager::instance().assets());

    bridge_ = std::make_unique<EngineEventBridge>();

    // 1. Engine drain: polls HttpPool and delivers HTTP callbacks to MountHttp caches.
    // 2. Image watcher: checks http_cache_generation(); decodes newly available assets
    //    and fires pending provider callbacks — runs after HTTP responses land.
    bridge_->add_channel([this] {
        engine_->drain();
        watcher_->drain();
    });
    bridge_->add_channel([this] {
        audio_->drain();
        dbg_->drain();
    });
    bridge_->add_channel([this] {
        sl_->drain();
        cs_->drain();
        ic_->drain();
        chat_->drain();
        players_->drain();
        evidence_->drain();
        music_area_->drain();
        hud_->drain();
    });
    // Navigation sync always runs last so QML sees the final screen state.
    bridge_->add_channel([this] { sync_current_screen_id(); });
}

bool QtAppInterface::setup_qml() {
    qml_engine_ = std::make_unique<QQmlApplicationEngine>();
    qml_engine_->rootContext()->setContextProperty("app", this);
    qml_engine_->addImageProvider(QStringLiteral("charicon"),
                                  new CharIconProvider(MediaManager::instance().assets(), *watcher_));
    qml_engine_->addImageProvider(QStringLiteral("emoteicon"),
                                  new EmoteIconProvider(MediaManager::instance().assets(), *watcher_));
    Log::debug("[QtAppInterface] QML engine created");

    Log::info("[QtAppInterface] loading QML module");
    qml_engine_->loadFromModule("AO", "Main");
    if (qml_engine_->rootObjects().isEmpty()) {
        Log::fatal("[QtAppInterface] QML engine failed to load — no root objects");
        return false;
    }
    Log::info("[QtAppInterface] QML loaded");
    return true;
}

void QtAppInterface::start() {
    bridge_->start();
    Log::debug("[QtAppInterface] event bridge started");
}

void QtAppInterface::stop() {
    bridge_->stop();
}

void QtAppInterface::disconnect() {
    Log::info("[QtAppInterface] disconnect — resetting courtroom controllers");
    chat_->reset();
    ic_->reset();
    players_->reset();
    evidence_->reset();
    music_area_->reset();
    hud_->reset();
    audio_->reset();
    ui_mgr_->pop_to_root();
}

void QtAppInterface::sync_current_screen_id() {
    if (!ui_mgr_)
        return;

    Screen* screen = ui_mgr_->active_screen();
    QString new_id = screen ? QString::fromStdString(screen->screen_id()) : QString{};

    if (new_id == current_screen_id_)
        return;

    Log::info("[QtAppInterface] screen changed: '{}' -> '{}'", current_screen_id_.toStdString(), new_id.toStdString());
    current_screen_id_ = new_id;
    emit currentScreenIdChanged();
}
