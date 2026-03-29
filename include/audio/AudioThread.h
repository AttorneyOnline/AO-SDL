#pragma once

#include "IAudioDevice.h"

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
    void audio_loop(std::stop_token st);

    /// Start streaming a music track from HTTP on a background download thread.
    void start_music_stream(const std::string& path, int channel, bool loop, float volume);

    IAudioDevice& device_;
    MountManager& mounts_;
    std::jthread thread_;

    // Active download threads for cleanup
    std::mutex downloads_mutex_;
    std::vector<std::jthread> download_threads_;
    void cleanup_downloads();
};
