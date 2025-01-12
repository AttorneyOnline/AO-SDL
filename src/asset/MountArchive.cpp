#include "MountArchive.h"

#include "bit7z/bitarchivereader.hpp"
#include "utils/Log.h"

#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

MountArchive::MountArchive(std::filesystem::path archive_path)
    : Mount(archive_path), reader(new bit7z::BitArchiveReader(library, path.string())) {
}

MountArchive::~MountArchive() {
    MountArchive::save_cache();
}

Mount::MountType MountArchive::get_type() {
    return ARCHIVE;
}

bool MountArchive::load() {
    reset_reader();

    try {
        reader = new bit7z::BitArchiveReader(library, get_path().string());
        reader->test();
    }
    catch (const std::exception& exception) {
        return false;
    }

    load_cache();

    return true;
}

bool MountArchive::contains_file(std::string path) {
    if (static_cache.contains(path)) {
        return true;
    }
    return false;
}

std::vector<std::byte> MountArchive::fetch_data(std::string path) {
    uint32_t index = static_cache.at(path);
    std::vector<std::byte> buffer;

    try {
        reader->extractTo(buffer, index);
    }
    catch (const std::exception& exception) {
        Log::log_print(ERR,
                       std::format("Failed to load data due to the following errror {}", exception.what()).c_str());
    }

    return buffer;
}

void MountArchive::load_cache() {
    const std::filesystem::path cache_file_path = get_path().replace_filename("cache");

    if (std::filesystem::exists(cache_file_path)) {
        return;
    }

    const auto items = reader->items();
    for (const auto& item : items) {
        static_cache[item.path().replace(0, 2, "/")] = item.index();
    }
}

void MountArchive::save_cache() {
    // Unused
}

void MountArchive::reset_reader() {
    try {
        if (reader) {
            delete reader;
            reader = nullptr;
        }
    }
    catch (const std::exception& e) {
        Log::log_print(ERR, e.what());
    }
}
