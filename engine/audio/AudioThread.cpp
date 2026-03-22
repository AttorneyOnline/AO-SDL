#include "audio/AudioThread.h"
#include "audio/AudioStream.h"

#include "asset/MountManager.h"
#include "event/EventManager.h"
#include "event/PlayBlipEvent.h"
#include "event/PlayMusicEvent.h"
#include "event/PlayMusicRequestEvent.h"
#include "event/PlaySFXEvent.h"
#include "event/StopAudioEvent.h"
#include "event/VolumeChangeEvent.h"
#include "utils/Log.h"

#include <chrono>
#include <sstream>

// Channel assignment:
//   0    = music
//   1    = ambience
//   2-6  = SFX (5 slots, round-robin)
//   7-11 = blips (5 slots, round-robin, future)
static constexpr int CH_MUSIC = 0;
static constexpr int CH_AMBIENCE = 1;
static constexpr int CH_SFX_BASE = 2;
static constexpr int CH_SFX_COUNT = 5;
static constexpr int CH_BLIP_BASE = 7;
static constexpr int CH_BLIP_COUNT = 5;

AudioThread::AudioThread(IAudioDevice& device, MountManager& mounts)
    : running_(true), device_(device), mounts_(mounts), thread_(&AudioThread::audio_loop, this) {
}

void AudioThread::stop() {
    running_ = false;
    if (thread_.joinable())
        thread_.join();

    // Join all download threads
    std::lock_guard lock(downloads_mutex_);
    for (auto& t : download_threads_) {
        if (t.joinable())
            t.join();
    }
    download_threads_.clear();
}

void AudioThread::cleanup_downloads() {
    std::lock_guard lock(downloads_mutex_);
    download_threads_.erase(std::remove_if(download_threads_.begin(), download_threads_.end(),
                                           [](std::thread& t) {
                                               if (t.joinable()) {
                                                   // Try to join completed threads (non-blocking check isn't
                                                   // directly available, so we accept a small leak of finished
                                                   // threads until cleanup is called with joinable threads)
                                                   return false;
                                               }
                                               return true;
                                           }),
                            download_threads_.end());
}

/// Parse an AO2 loop metadata .txt file.
/// Format: key=value lines with loop_start, loop_end/loop_length, seconds=true/false.
/// Returns loop points in PCM samples at 48kHz.
static std::pair<int64_t, int64_t> parse_loop_txt(const std::vector<uint8_t>& data) {
    std::string text(data.begin(), data.end());
    bool use_seconds = false;
    double loop_start = 0, loop_end = 0, loop_length = 0;

    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Trim
        while (!key.empty() && key.back() == ' ')
            key.pop_back();
        while (!val.empty() && val.front() == ' ')
            val.erase(val.begin());

        if (key == "seconds")
            use_seconds = (val == "true" || val == "1");
        else if (key == "loop_start")
            loop_start = std::stod(val);
        else if (key == "loop_end")
            loop_end = std::stod(val);
        else if (key == "loop_length")
            loop_length = std::stod(val);
    }

    if (loop_end == 0 && loop_length > 0)
        loop_end = loop_start + loop_length;

    // Convert to PCM samples at 48kHz
    constexpr double SAMPLE_RATE = 48000.0;
    int64_t start_samples, end_samples;
    if (use_seconds) {
        start_samples = static_cast<int64_t>(loop_start * SAMPLE_RATE);
        end_samples = static_cast<int64_t>(loop_end * SAMPLE_RATE);
    }
    else {
        // Values are already in samples (AO2 convention)
        start_samples = static_cast<int64_t>(loop_start);
        end_samples = static_cast<int64_t>(loop_end);
    }

    return {start_samples, end_samples};
}

void AudioThread::start_music_stream(const std::string& path, int channel, bool loop, float volume) {
    auto stream = std::make_shared<AudioStream>();
    stream->set_looping(loop);

    // Start playback immediately — the stream will output silence until data arrives
    device_.play_stream(channel, stream, false, volume); // loop=false at device level: AudioStream handles looping

    // Spawn download thread
    auto stream_ref = stream; // capture shared_ptr
    std::lock_guard lock(downloads_mutex_);
    download_threads_.emplace_back([this, stream_ref, path, loop]() {
        // Try to load loop point metadata (.txt file alongside the music).
        // AO2 convention: "track.opus.txt" or "track.txt" next to the audio file.
        // Only check local mounts — don't block on HTTP for a metadata file.
        if (loop) {
            std::string base = path;
            auto dot = base.rfind('.');
            if (dot != std::string::npos)
                base = base.substr(0, dot);

            // Try "track.opus.txt" first (AO2 convention), then "track.txt"
            auto txt_data = mounts_.fetch_data("sounds/music/" + path + ".txt");
            if (!txt_data)
                txt_data = mounts_.fetch_data("music/" + path + ".txt");
            if (!txt_data)
                txt_data = mounts_.fetch_data("sounds/music/" + base + ".txt");
            if (!txt_data)
                txt_data = mounts_.fetch_data("music/" + base + ".txt");

            if (txt_data && !txt_data->empty()) {
                auto [start, end] = parse_loop_txt(*txt_data);
                if (start > 0 || end > 0) {
                    Log::log_print(INFO, "AudioThread: loop points for '%s': start=%lld end=%lld samples", path.c_str(),
                                   (long long)start, (long long)end);
                    stream_ref->set_loop_points(start, end);
                }
            }
        }

        // Try local mounts first — feed all data and start decoding immediately.
        auto local = mounts_.fetch_data("sounds/music/" + path);
        if (!local)
            local = mounts_.fetch_data("music/" + path);

        if (local) {
            stream_ref->feed(local->data(), local->size());
            stream_ref->mark_complete();
            Log::log_print(INFO, "AudioThread: loaded music from disk: '%s' (%zu bytes)", path.c_str(), local->size());
        }
        else {
            // Fall back to HTTP streaming
            bool found = mounts_.fetch_streaming("sounds/music/" + path, [&](const uint8_t* data, size_t len) -> bool {
                stream_ref->feed(data, len);
                return !stream_ref->is_cancelled();
            });

            if (!found) {
                found = mounts_.fetch_streaming("music/" + path, [&](const uint8_t* data, size_t len) -> bool {
                    stream_ref->feed(data, len);
                    return !stream_ref->is_cancelled();
                });
            }

            stream_ref->mark_complete();

            if (!found) {
                Log::log_print(WARNING, "AudioThread: music not found: '%s'", path.c_str());
                stream_ref->cancel();
            }
            else {
                Log::log_print(INFO, "AudioThread: music download complete: '%s'", path.c_str());
            }
        }
    });
}

void AudioThread::audio_loop() {
    int sfx_slot = 0;
    int blip_slot = 0;

    // Per-category master volumes (0.0 - 1.0).
    // Default 0.125 matches the UI slider default of 50% with cubic curve.
    float music_volume = 0.125f;
    float sfx_volume = 0.125f;
    float blip_volume = 0.125f;

    while (running_) {
        auto tick_start = std::chrono::steady_clock::now();

        // --- Play SFX (pre-decoded) ---
        auto& play_sfx_ch = EventManager::instance().get_channel<PlaySFXEvent>();
        while (auto ev = play_sfx_ch.get_event()) {
            int channel = CH_SFX_BASE + (sfx_slot++ % CH_SFX_COUNT);
            Log::log_print(DEBUG, "AudioThread: play SFX on ch%d (%.1fs)", channel,
                           ev->asset() ? ev->asset()->duration_seconds() : 0.0f);
            device_.play(channel, ev->asset(), ev->loop(), ev->volume() * sfx_volume);
        }

        // --- Play Blips (pre-decoded, round-robin on blip channels) ---
        auto& play_blip_ch = EventManager::instance().get_channel<PlayBlipEvent>();
        while (auto ev = play_blip_ch.get_event()) {
            int channel = CH_BLIP_BASE + (blip_slot++ % CH_BLIP_COUNT);
            device_.play(channel, ev->asset(), false, ev->volume() * blip_volume);
        }

        // --- Play Music (pre-decoded, legacy path) ---
        auto& play_music_ch = EventManager::instance().get_channel<PlayMusicEvent>();
        while (auto ev = play_music_ch.get_event()) {
            int channel = (ev->channel() == 1) ? CH_AMBIENCE : CH_MUSIC;
            Log::log_print(INFO, "AudioThread: play music (pre-decoded) on ch%d loop=%d (%.1fs)", channel, ev->loop(),
                           ev->asset() ? ev->asset()->duration_seconds() : 0.0f);
            device_.play(channel, ev->asset(), ev->loop(), ev->volume() * music_volume);
        }

        // --- Play Music (streaming) ---
        auto& music_req_ch = EventManager::instance().get_channel<PlayMusicRequestEvent>();
        while (auto ev = music_req_ch.get_event()) {
            int channel = (ev->channel() == 1) ? CH_AMBIENCE : CH_MUSIC;
            Log::log_print(INFO, "AudioThread: streaming music '%s' on ch%d loop=%d", ev->path().c_str(), channel,
                           ev->loop());
            // Stop any current stream/sound on this channel first
            device_.stop(channel);
            start_music_stream(ev->path(), channel, ev->loop(), ev->volume() * music_volume);
        }

        // --- Stop Audio ---
        auto& stop_ch = EventManager::instance().get_channel<StopAudioEvent>();
        while (auto ev = stop_ch.get_event()) {
            if (ev->type() == StopAudioEvent::Type::MUSIC) {
                int channel = (ev->channel() == 1) ? CH_AMBIENCE : CH_MUSIC;
                device_.stop(channel);
            }
            else if (ev->type() == StopAudioEvent::Type::SFX) {
                for (int i = 0; i < CH_SFX_COUNT; ++i)
                    device_.stop(CH_SFX_BASE + i);
            }
            else if (ev->type() == StopAudioEvent::Type::BLIP) {
                for (int i = 0; i < CH_BLIP_COUNT; ++i)
                    device_.stop(CH_BLIP_BASE + i);
            }
        }

        // --- Volume Changes ---
        auto& vol_ch = EventManager::instance().get_channel<VolumeChangeEvent>();
        while (auto ev = vol_ch.get_event()) {
            float vol = ev->volume();
            switch (ev->category()) {
            case VolumeChangeEvent::Category::MUSIC:
                music_volume = vol;
                device_.set_volume(CH_MUSIC, vol);
                device_.set_volume(CH_AMBIENCE, vol);
                break;
            case VolumeChangeEvent::Category::SFX:
                sfx_volume = vol;
                for (int i = 0; i < CH_SFX_COUNT; ++i)
                    device_.set_volume(CH_SFX_BASE + i, vol);
                break;
            case VolumeChangeEvent::Category::BLIP:
                blip_volume = vol;
                for (int i = 0; i < CH_BLIP_COUNT; ++i)
                    device_.set_volume(CH_BLIP_BASE + i, vol);
                break;
            case VolumeChangeEvent::Category::MASTER:
                music_volume = sfx_volume = blip_volume = vol;
                for (int i = 0; i < 12; ++i)
                    device_.set_volume(i, vol);
                break;
            }
        }

        // Sleep remainder of ~16ms tick
        auto elapsed = std::chrono::steady_clock::now() - tick_start;
        auto remaining = std::chrono::milliseconds(16) - elapsed;
        if (remaining.count() > 0)
            std::this_thread::sleep_for(remaining);
    }
}
