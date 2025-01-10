#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class Mount {
  public:
    Mount(std::filesystem::path target_path) : path{target_path} {};
    virtual ~Mount();

    enum MountType { ARCHIVE };

    std::filesystem::path get_path() {
        return path;
    };

    virtual MountType get_type() = 0;
    virtual bool load() = 0;

    virtual bool contains_file(std::string path) = 0;
    virtual std::vector<std::byte> fetch_data(std::string path) = 0;

  protected:
    std::filesystem::path path;

  private:
    virtual bool load_cache() = 0;
    virtual void save_cache() = 0;
};
