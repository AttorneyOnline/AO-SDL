#include "asset/MountDirectory.h"

#include <fstream>
#include <stdexcept>

MountDirectory::MountDirectory(const std::filesystem::path& dir_path) : Mount(dir_path) {
}

void MountDirectory::load() {
    if (!std::filesystem::is_directory(path)) {
        throw std::runtime_error("MountDirectory: path is not a directory: " + path.string());
    }
}

bool MountDirectory::seek_file(const std::string& relative_path) const {
    return std::filesystem::exists(path / relative_path);
}

std::vector<uint8_t> MountDirectory::fetch_data(const std::string& relative_path) {
    std::filesystem::path full_path = path / relative_path;

    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("MountDirectory: failed to open file: " + full_path.string());
    }

    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}
