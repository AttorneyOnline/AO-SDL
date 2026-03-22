#include "ao/game/AOMusicPlayer.h"

#include "event/EventManager.h"
#include "event/MusicChangeEvent.h"
#include "event/NowPlayingEvent.h"
#include "event/PlayMusicRequestEvent.h"
#include "event/StopAudioEvent.h"

void AOMusicPlayer::tick(bool active) {
    auto& music_ch = EventManager::instance().get_channel<MusicChangeEvent>();
    while (auto ev = music_ch.get_event()) {
        const auto& track = ev->track();
        EventManager::instance().get_channel<NowPlayingEvent>().publish(NowPlayingEvent(track));

        // Always store the latest music state
        pending_track_ = track;
        pending_channel_ = ev->channel();
        pending_loop_ = (ev->looping() != 0);

        if (!active)
            continue; // Buffer the request but don't play until courtroom is visible

        if (track.empty() || track == "~stop.mp3") {
            EventManager::instance().get_channel<StopAudioEvent>().publish(
                StopAudioEvent(ev->channel(), StopAudioEvent::Type::MUSIC));
            continue;
        }

        EventManager::instance().get_channel<PlayMusicRequestEvent>().publish(
            PlayMusicRequestEvent(track, ev->channel(), pending_loop_, 1.0f));
    }

    // When courtroom first becomes active, start the pending music from the beginning
    if (active && !was_active_) {
        was_active_ = true;
        if (!pending_track_.empty() && pending_track_ != "~stop.mp3") {
            EventManager::instance().get_channel<PlayMusicRequestEvent>().publish(
                PlayMusicRequestEvent(pending_track_, pending_channel_, pending_loop_, 1.0f));
        }
    }
    if (!active)
        was_active_ = false;
}
