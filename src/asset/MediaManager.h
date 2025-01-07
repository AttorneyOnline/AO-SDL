#pragma once

#include <memory>

class MountManager;

class MediaManager {
  public:
    static MediaManager* get();

    MediaManager(MediaManager&) = delete;
    void operator=(MediaManager const&) = delete;

  private:
    MediaManager();
    std::unique_ptr<MountManager> mount_manager;
};
