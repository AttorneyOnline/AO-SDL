#ifndef MOUNTARCHIVE_H
#define MOUNTARCHIVE_H

#include "Mount.h"
#include <cstdint>
#include <filesystem>
#include <unordered_map>

class MountArchive : public Mount {
  public:
    explicit MountArchive(std::filesystem::path archive_path);
    ~MountArchive();

    MountType get_type() override;
    bool load() override;

    bool contains_file(std::string path) override;
    std::vector<std::byte> fetch_data(std::string path) override;

  private:
    bool load_cache() override;
    void save_cache() override;

    std::unordered_map<std::string, uint32_t> static_cache;
};

#endif // MOUNTARCHIVE_H
