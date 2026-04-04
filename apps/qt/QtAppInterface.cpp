#include "QtAppInterface.h"

#include "EngineEventBridge.h"
#include "EngineInterface.h"
#include "asset/MediaManager.h"
#include "ui/CharIconProvider.h"
#include "ui/EmoteIconProvider.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

#include "ao/ui/screens/ServerListScreen.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>

QtAppInterface::QtAppInterface(QObject* parent) : QObject(parent) {
}

QtAppInterface::~QtAppInterface() = default;

QtAppInterface& QtAppInterface::instance() {
    static QtAppInterface ctx;
    return ctx;
}

// --- Accessors (raw pointers from unique_ptr — QML does not own them) --------

ServerListController* QtAppInterface::server_list_controller() const {
    return sl_.get();
}
CharSelectController* QtAppInterface::char_select_controller() const {
    return cs_.get();
}
CourtroomController* QtAppInterface::courtroom_controller() const {
    return cr_.get();
}
DebugController* QtAppInterface::debug_controller() const {
    return dbg_.get();
}
QString QtAppInterface::current_screen_id() const {
    return current_screen_id_;
}

// --- Lifecycle ---------------------------------------------------------------

void QtAppInterface::init(EngineInterface& engine) {
    engine_ = &engine;

    // 1. UIManager — no dependencies.
    ui_mgr_ = std::make_unique<UIManager>();

    // 2. Controllers (construction order matters: cr before cs).
    cr_ = std::make_unique<CourtroomController>(*ui_mgr_);
    cs_ = std::make_unique<CharSelectController>(*ui_mgr_, *cr_);
    sl_ = std::make_unique<ServerListController>(*ui_mgr_);
    dbg_ = std::make_unique<DebugController>();

    // 3. Push the initial screen.
    ui_mgr_->push_screen(std::make_unique<ServerListScreen>());
    Log::debug("[QtAppInterface] controllers created");

    // 4. EngineEventBridge — drain channels registered in execution order.
    bridge_ = std::make_unique<EngineEventBridge>();
    bridge_->add_channel([this] { engine_->drain(); });
    bridge_->add_channel([this] {
        sl_->drain();
        cs_->drain();
        cr_->drain();
        dbg_->drain();
    });
    bridge_->add_channel([this] { sync_current_screen_id(); });
}

bool QtAppInterface::setup_qml() {
    qml_engine_ = std::make_unique<QQmlApplicationEngine>();
    qml_engine_->rootContext()->setContextProperty("app", this);
    qml_engine_->addImageProvider(QStringLiteral("charicon"), new CharIconProvider(MediaManager::instance().assets()));
    qml_engine_->addImageProvider(QStringLiteral("emoteicon"),
                                  new EmoteIconProvider(MediaManager::instance().assets()));
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
