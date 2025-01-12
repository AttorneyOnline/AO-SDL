#pragma once

#include <filesystem>

class Mount {
  public:
    Mount(const std::filesystem::path& target_path) : path{target_path} {};

    std::filesystem::path get_path() {
        return path;
    };

    virtual void load() = 0;

    virtual bool seek_file(const std::string& path) = 0;
    virtual std::vector<uint8_t> fetch_data(const std::string& path) = 0;

  protected:
    const std::filesystem::path path;
    virtual void load_cache() = 0;
    virtual void save_cache() = 0;
};
