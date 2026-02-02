#include "MountArchive.h"

#include "bit7z/bitarchivereader.hpp"
#include "utils/Log.h"

MountArchive::MountArchive(const std::filesystem::path& archive_path)
    : Mount(archive_path), reader(std::make_unique<bit7z::BitArchiveReader>(library, path.string())) {
}

void MountArchive::load() {
    reset_reader();

    try {
        reader = std::make_unique<bit7z::BitArchiveReader>(library, get_path().string());
        reader->test();
    }
    catch (const std::exception& exception) {
        throw exception;
    }

    load_cache();
}

bool MountArchive::seek_file(const std::string& path) const {
    return static_cache.contains(path);
}

std::vector<uint8_t> MountArchive::fetch_data(const std::string& path) {
    uint32_t index = static_cache.at(path);
    std::vector<uint8_t> buffer;

    try {
        reader->extractTo(buffer, index);
    }
    catch (const std::exception& exception) {
        throw exception;
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
        // 7zip returns files with a double-backslash prefix
        // These are stripped to be consistent with filesystem relative paths
        static_cache.at(item.path().replace(0, 2, "/")) = item.index();
    }
}

void MountArchive::save_cache() {
    // TODO: Implement writing cache to disk
}

void MountArchive::reset_reader() {
    try {
        if (reader) {
            reader.reset();
        }
    }
    catch (const std::exception& e) {
        Log::log_print(ERR, e.what());
    }
}
