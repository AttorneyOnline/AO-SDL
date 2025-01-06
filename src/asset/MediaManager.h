#pragma once

#include <memory>

class MountAccessManager;

class MediaManager {
  public:
    static MediaManager* get();

    MediaManager(MediaManager&) = delete;
    void operator=(MediaManager const&) = delete;

  private:
    MediaManager();
    std::unique_ptr<MountAccessManager> mount_access;
};
