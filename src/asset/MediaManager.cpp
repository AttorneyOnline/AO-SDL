#include "MediaManager.h"

#include "MountManager.h"

MediaManager::MediaManager() : mount_manager(std::make_unique<MountManager>()) {
}

MediaManager& MediaManager::instance() {
    static MediaManager instance;
    return instance;
}
