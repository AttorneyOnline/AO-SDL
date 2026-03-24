#include "game/Session.h"

#include "asset/AssetLibrary.h"
#include "asset/MountHttp.h"
#include "event/EventManager.h"
#include "event/StopAudioEvent.h"
#include "utils/Log.h"

std::atomic<uint32_t> Session::next_session_id_{1};

Session::Session(MountManager& mounts, AssetLibrary& assets)
    : mounts_(mounts), assets_(assets), session_id_(next_session_id_++) {
    assets_.set_active_session(session_id_);
    Log::log_print(INFO, "Session %u started", session_id_);
}

Session::~Session() {
    // Stop all audio channels
    auto& ch = EventManager::instance().get_channel<StopAudioEvent>();
    ch.publish(StopAudioEvent(0, StopAudioEvent::Type::MUSIC));
    ch.publish(StopAudioEvent(1, StopAudioEvent::Type::MUSIC));
    ch.publish(StopAudioEvent(0, StopAudioEvent::Type::SFX));
    ch.publish(StopAudioEvent(0, StopAudioEvent::Type::BLIP));

    // Remove session HTTP mounts
    for (auto handle : mount_handles_)
        mounts_.remove_mount(handle);

    // Clear session-tagged cache entries
    assets_.clear_session(session_id_);
    assets_.clear_active_session();

    Log::log_print(INFO, "Session %u ended (removed %zu mount(s))", session_id_, mount_handles_.size());
}

MountManager::MountHandle Session::add_http_mount(const std::string& url, HttpPool& pool) {
    auto mount = std::make_unique<MountHttp>(url, pool);
    auto handle = mounts_.add_mount(std::move(mount));
    mount_handles_.push_back(handle);
    Log::log_print(INFO, "Session %u: added HTTP mount %s (handle %u)", session_id_, url.c_str(), handle);
    return handle;
}
