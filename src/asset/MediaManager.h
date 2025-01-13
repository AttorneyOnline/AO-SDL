#pragma once

#include <memory>

class MountManager;

class MediaManager {
  public:
    static MediaManager& instance();

  private:
    MediaManager();

    // Delete copy and move semantics
    MediaManager(MediaManager&) = delete;
    void operator=(MediaManager const&) = delete;

    // Access to the underlying storage system
    std::unique_ptr<MountManager> mount_manager;
};
