#include "asset/MountEmbedded.h"

MountEmbedded::MountEmbedded() : Mount("embedded://") {
}

void MountEmbedded::load() {
    for (const auto& file : embedded_assets())
        index_[file.path] = &file;
}

bool MountEmbedded::seek_file(const std::string& path) const {
    return index_.find(path) != index_.end();
}

std::vector<uint8_t> MountEmbedded::fetch_data(const std::string& path) {
    auto it = index_.find(path);
    if (it == index_.end())
        return {};
    return {it->second->data, it->second->data + it->second->size};
}
