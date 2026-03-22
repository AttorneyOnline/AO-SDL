#pragma once

#include <string>

/// Manages courtroom music state: drains MusicChangeEvents, buffers pending
/// tracks until the courtroom is active, and publishes play/stop requests.
class AOMusicPlayer {
  public:
    /// Drain MusicChangeEvents and publish play/stop requests.
    /// @param active Whether the courtroom is currently visible.
    void tick(bool active);

  private:
    std::string pending_track_;
    int pending_channel_ = 0;
    bool pending_loop_ = true;
    bool was_active_ = false;
};
