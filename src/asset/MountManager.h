#pragma once

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class Mount;

class MountManager {
  public:
    MountManager();

    void loadMounts(std::vector<std::filesystem::path> target_mount_path);
    std::optional<std::vector<std::byte>> fetch_data(std::string relative_path);

  private:
    std::mutex lock;
    std::vector<Mount*> loaded_mounts;

    void cleanupMounts();
};
