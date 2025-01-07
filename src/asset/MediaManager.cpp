#include "MediaManager.h"

#include "MountManager.h"

MediaManager::MediaManager() : mount_manager(std::unique_ptr<MountManager>(new MountManager())) {
}

MediaManager* MediaManager::get() {
    static MediaManager instance;
    return &instance;
}
