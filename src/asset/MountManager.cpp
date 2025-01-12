#include "MountManager.h"

#include "MountArchive.h"
#include "utils/Log.h"

#include <format>

MountManager::MountManager() {
}

void MountManager::loadMounts(const std::vector<std::filesystem::path>& target_mount_path) {
    std::unique_lock<std::shared_mutex> locker(lock);

    if (!loaded_mounts.empty()) {
        loaded_mounts.clear();
    }

    for (const std::filesystem::path& mount_path : target_mount_path) {

        try {
            std::unique_ptr<MountArchive> archive = std::make_unique<MountArchive>(mount_path);
            archive.get()->load();
            loaded_mounts.push_back(std::move(archive));
        }
        catch (std::exception exception) {
            // TODO: Add exception handling here
        }
    }
}

std::optional<std::vector<uint8_t>> MountManager::fetch_data(const std::string& relative_path) {
    Log::log_print(INFO, std::format("Loading file at {}", relative_path).c_str());
    for (auto& mount : loaded_mounts) {
        if (mount->seek_file(relative_path)) {
            try {
                return std::make_optional<std::vector<uint8_t>>(mount->fetch_data(relative_path));
            }
            catch (std::exception exception) {
                Log::log_print(
                    ERR, std::format("Failed to load data due to the following errror {}", exception.what()).c_str());
            }
        }
    }
    return std::nullopt;
}
