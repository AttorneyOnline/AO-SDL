#ifndef MOUNTARCHIVE_H
#define MOUNTARCHIVE_H

#include "Mount.h"

#include <include/bit7z/bit7zlibrary.hpp>
#include <include/bit7z/bitarchivereader.hpp>

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
    void load_cache() override;
    void save_cache() override;

    void reset_reader();

    std::unordered_map<std::string, uint32_t> static_cache;
    const bit7z::Bit7zLibrary library;
    bit7z::BitArchiveReader* reader = nullptr;
};

#endif // MOUNTARCHIVE_H
