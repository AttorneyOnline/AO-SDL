#pragma once

#include "Mount.h"

class MountDirectory : public Mount {
  public:
    explicit MountDirectory(const std::filesystem::path& dir_path);

    void load() override;
    bool seek_file(const std::string& path) const override;
    std::vector<uint8_t> fetch_data(const std::string& path) override;

  private:
    void load_cache() override {}
    void save_cache() override {}
};
