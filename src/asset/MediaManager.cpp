#include "MediaManager.h"

#include "MountAccessManager.h"

MediaManager::MediaManager() : mount_access(std::unique_ptr<MountAccessManager>(new MountAccessManager())) {
}
