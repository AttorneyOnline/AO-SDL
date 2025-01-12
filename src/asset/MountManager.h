#pragma once

#include <filesystem>
#include <shared_mutex>
#include <vector>

#include "Mount.h"

class MountManager {
  public:
    MountManager();

    void loadMounts(const std::vector<std::filesystem::path>& target_mount_path);
    std::optional<std::vector<uint8_t>> fetch_data(const std::string& relative_path);

  private:
    std::shared_mutex lock;
    std::vector<std::unique_ptr<Mount>> loaded_mounts;
};
