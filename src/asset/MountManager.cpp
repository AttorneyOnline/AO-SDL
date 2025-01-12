#include "MountManager.h"

#include "MountArchive.h"

#include "Mount.h"
#include <cstddef>
#include <exception>
#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

MountManager::MountManager() {
}

void MountManager::loadMounts(std::vector<std::filesystem::path> target_mount_path) {
    if (!loaded_mounts.empty()) {
        cleanupMounts();
    }

    for (const std::filesystem::path& mount_target : target_mount_path) {
        try {
            // TODO: Add backend selector so the right type of mount is created!
            Mount* mount = new MountArchive(mount_target);
            mount->load();
        }
        catch (std::exception exception) {
            throw exception;
        }
    }
}

std::optional<std::vector<std::byte>> MountManager::fetch_data(std::string relative_path) {
    std::shared_lock locker(lock);
    for (const auto mount : loaded_mounts) {
        if (mount->contains_file(relative_path)) {
            return std::optional<std::vector<std::byte>>(mount->fetch_data(relative_path));
        }
        continue;
    }
    return std::nullopt;
}

void MountManager::cleanupMounts() {
    std::unique_lock locker(lock);
    for (Mount* mount : loaded_mounts) {
        delete mount;
    }
}
