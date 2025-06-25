#ifndef MOUNTARCHIVE_H
#define MOUNTARCHIVE_H

#include "Mount.h"

#include <include/bit7z/bit7zlibrary.hpp>
#include <include/bit7z/bitarchivereader.hpp>

class MountArchive : public Mount {
  public:
    explicit MountArchive(const std::filesystem::path& archive_path);
    ~MountArchive() = default;

    void load() override;

    bool seek_file(const std::string& path) const override;
    std::vector<uint8_t> fetch_data(const std::string& path) override;

  private:
    void load_cache() override;
    void save_cache() override;

    void reset_reader();

    std::unordered_map<std::string, uint32_t> static_cache;
    const bit7z::Bit7zLibrary library;
    std::unique_ptr<bit7z::BitArchiveReader> reader;
};

#endif // MOUNTARCHIVE_H
