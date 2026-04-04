#include "EngineInterface.h"

#include "QtDebugContext.h"
#include "asset/MediaManager.h"
#include "event/AssetUrlEvent.h"
#include "event/EventManager.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "game/GameThread.h"
#include "game/Session.h"
#include "net/HttpPool.h"
#include "net/WSClientThread.h"
#include "render/StateBuffer.h"
#include "utils/Log.h"

#include "ao/ao_plugin.h"

EngineInterface::EngineInterface(StateBuffer& render_buffer)
    : http_(std::make_unique<HttpPool>(4)), protocol_(ao::create_protocol()),
      net_(std::make_unique<WSClientThread>(*protocol_)), presenter_(ao::create_presenter()),
      game_(std::make_unique<GameThread>(render_buffer, *presenter_)) {
    auto& debug_ctx = QtDebugContext::instance();
    debug_ctx.game_thread = game_.get();
    debug_ctx.presenter = presenter_.get();
    Log::debug("[EngineInterface] initialised");
}

EngineInterface::~EngineInterface() {
    stop();
}

void EngineInterface::drain() {
    http_->poll();

    if (EventManager::instance().get_channel<SessionStartEvent>().get_event()) {
        Log::info("[EngineInterface] session started");
        active_session_ =
            std::make_unique<Session>(MediaManager::instance().mounts_ref(), MediaManager::instance().assets());
        active_session_->add_http_mount("https://attorneyoffline.de/base/", *http_, 300);
    }

    auto& asset_ch = EventManager::instance().get_channel<AssetUrlEvent>();
    while (auto ev = asset_ch.get_event()) {
        if (active_session_) {
            Log::debug("[EngineInterface] adding asset mount: {}", ev->url());
            active_session_->add_http_mount(ev->url(), *http_);
        }
    }

    if (EventManager::instance().get_channel<SessionEndEvent>().get_event()) {
        Log::info("[EngineInterface] session ended");
        active_session_.reset();
    }
}

void EngineInterface::stop() {
    if (net_)
        net_->stop();
    if (game_)
        game_->stop();
    if (http_)
        http_->stop();
    Log::info("[EngineInterface] stopped");
}

HttpPool& EngineInterface::http() {
    return *http_;
}
