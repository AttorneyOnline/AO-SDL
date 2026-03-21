#include "asset/MountManager.h"

#include "asset/MountArchive.h"
#include "asset/MountDirectory.h"
#include "asset/MountHttp.h"
#include "utils/Log.h"

#include <format>
#include <shared_mutex>

MountManager::MountManager() {
}

void MountManager::load_mounts(const std::vector<std::filesystem::path>& target_mount_path) {
    std::unique_lock<std::shared_mutex> locker(lock);

    loaded_mounts.clear();

    for (const std::filesystem::path& mount_path : target_mount_path) {
        try {
            std::unique_ptr<Mount> mount;

            if (std::filesystem::is_directory(mount_path)) {
                mount = std::make_unique<MountDirectory>(mount_path);
            }
            else {
                mount = std::make_unique<MountArchive>(mount_path);
            }

            mount->load();
            loaded_mounts.push_back(std::move(mount));
        }
        catch (const std::exception& e) {
            Log::log_print(WARNING,
                           std::format("Failed to create mount at {}: {}", mount_path.string(), e.what()).c_str());
        }
    }
}

void MountManager::add_mount(std::unique_ptr<Mount> mount) {
    std::unique_lock<std::shared_mutex> locker(lock);
    mount->load();
    loaded_mounts.push_back(std::move(mount));
}

void MountManager::prefetch(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);

    // If any local (non-HTTP) mount has the file, skip — no need to fetch remotely
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (!http && mount->seek_file(relative_path))
            return;
    }

    // Trigger HTTP downloads
    for (auto& mount : loaded_mounts) {
        auto* http = dynamic_cast<MountHttp*>(mount.get());
        if (http)
            http->request(relative_path);
    }
}

std::optional<std::vector<uint8_t>> MountManager::fetch_data(const std::string& relative_path) {
    std::shared_lock<std::shared_mutex> locker(lock);

    for (auto& mount : loaded_mounts) {
        if (mount->seek_file(relative_path)) {
            try {
                return mount->fetch_data(relative_path);
            }
            catch (const std::exception& e) {
                Log::log_print(ERR, std::format("Failed to fetch {}: {}", relative_path, e.what()).c_str());
            }
        }
    }

    return std::nullopt;
}
