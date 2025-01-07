#include "MountArchive.h"

#include "utils/Log.h"

#include <cstddef>
#include <vector>

// TODO : Add Bit7z Lib to actually suppor this

MountArchive::MountArchive(std::filesystem::path archive_path) : Mount(archive_path) {
}

MountArchive::~MountArchive() {
}

Mount::MountType MountArchive::get_type() {
    return ARCHIVE;
}

bool MountArchive::load() {
    if (load_cache()) {
        Log::log_print(INFO, std::format("Restored cache of archive mount {}", get_path().string()).c_str());
    }
    return true;
}

bool MountArchive::contains_file(std::string path) {
    if (static_cache.contains(path)) {
        return true;
    }
    return false;
}

std::vector<std::byte> MountArchive::fetch_data(std::string path) {
    return std::vector<std::byte>();
}

bool MountArchive::load_cache() {
    return true;
}

void MountArchive::save_cache() {
}
