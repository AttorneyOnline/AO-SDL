#pragma once

#include <filesystem>
#include <string>
#include <vector>

enum MountType { Filesystem, Archive };

class MountAccess {
  public:
    MountAccess(std::filesystem::path location) : mount_path{location} {};

    using MediaData = std::optional<std::vector<std::byte>>;

    virtual MediaData fetch_data(std::string relative_path) = 0;
    virtual std::filesystem::path path() = 0;
    virtual MountType type() = 0;

  private:
    virtual bool loadCache() = 0;
    virtual bool saveCache() = 0;

    std::filesystem::path mount_path;
};
