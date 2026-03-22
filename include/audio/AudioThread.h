#pragma once

#include "IAudioDevice.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class MountManager;

/**
 * @brief Drives audio playback on a dedicated background thread.
 *
 * Drains audio event channels and forwards commands to an IAudioDevice.
 * For music, handles HTTP downloading and streaming decode so that the
 * game thread is never blocked on audio I/O.
 */
class AudioThread {
  public:
    AudioThread(IAudioDevice& device, MountManager& mounts);

    /// Signal the audio loop to stop and join the thread.
    void stop();

  private:
    void audio_loop();

    /// Start streaming a music track from HTTP on a background download thread.
    void start_music_stream(const std::string& path, int channel, bool loop, float volume);

    std::atomic<bool> running_;
    IAudioDevice& device_;
    MountManager& mounts_;
    std::thread thread_;

    // Active download threads for cleanup
    std::mutex downloads_mutex_;
    std::vector<std::thread> download_threads_;
    void cleanup_downloads();
};
