#pragma once

#include <string>
#include <vector>

class MountAccess {
  public:
    enum MountType { Filesystem, Archive };

    virtual std::vector<std::byte> fetch_data(std::string relative_path) = 0;
};
