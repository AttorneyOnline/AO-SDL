#include "AssetLibrary.h"

#include "MountManager.h"

#include "stb_image.h"

AssetLibrary::AssetLibrary(MountManager& mounts, size_t cache_max_bytes)
    : m_mounts(mounts), m_cache(cache_max_bytes) {
}

std::shared_ptr<ImageAsset> AssetLibrary::image(const std::string& path) {
    auto cached = m_cache.get(path);
    if (cached) return std::static_pointer_cast<ImageAsset>(cached);

    auto result = probe(path, {"webp", "apng", "gif", "png"});
    if (!result) return nullptr;

    auto& [resolved, data] = *result;
    std::string format = resolved.substr(resolved.rfind('.') + 1);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* pixels = stbi_load_from_memory(data.data(), (int)data.size(), &width, &height, &channels, 4);
    stbi_set_flip_vertically_on_load(false);
    if (!pixels) return nullptr;

    ImageFrame frame;
    frame.width = width;
    frame.height = height;
    frame.duration_ms = 0;
    frame.pixels.assign(pixels, pixels + width * height * 4);
    stbi_image_free(pixels);

    auto asset = std::make_shared<ImageAsset>(path, format, std::vector<ImageFrame>{std::move(frame)});
    m_cache.insert(asset);
    return asset;
}

std::shared_ptr<Asset> AssetLibrary::audio(const std::string& path) {
    // todo: decode audio into an AudioAsset once that type exists
    auto result = probe(path, {"opus", "ogg", "mp3", "wav"});
    if (!result) return nullptr;

    // placeholder: return nullptr until AudioAsset is implemented
    return nullptr;
}

std::optional<IniDocument> AssetLibrary::config(const std::string& path) {
    auto data = m_mounts.fetch_data(path);
    if (!data) return std::nullopt;

    IniDocument doc;
    std::string current_section;
    std::string text(data->begin(), data->end());
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip carriage returns and leading/trailing whitespace
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                current_section = line.substr(1, end - 1);
            }
        }
        else {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                // Trim key/value
                key.erase(key.find_last_not_of(" \t") + 1);
                size_t vs = val.find_first_not_of(" \t");
                if (vs != std::string::npos) val = val.substr(vs);
                doc[current_section][key] = val;
            }
        }
    }

    return doc;
}

std::shared_ptr<Asset> AssetLibrary::shader(const std::string& path) {
    auto data = m_mounts.fetch_data(path);
    if (!data) return nullptr;
    // todo: return a ShaderAsset once that type exists
    return nullptr;
}

std::shared_ptr<Asset> AssetLibrary::font(const std::string& path) {
    auto data = m_mounts.fetch_data(path);
    if (!data) return nullptr;
    // todo: return a FontAsset once that type exists
    return nullptr;
}

std::optional<std::vector<uint8_t>> AssetLibrary::raw(const std::string& path) {
    return m_mounts.fetch_data(path);
}

std::vector<std::string> AssetLibrary::list(const std::string& directory) {
    // todo: MountManager needs a list() method — deferred
    return {};
}

void AssetLibrary::evict_unused() {
    m_cache.evict_unused();
}

std::optional<std::pair<std::string, std::vector<uint8_t>>>
AssetLibrary::probe(const std::string& path, const std::vector<std::string>& extensions) {
    for (const auto& ext : extensions) {
        std::string candidate = path + "." + ext;
        auto data = m_mounts.fetch_data(candidate);
        if (data) return std::make_pair(candidate, std::move(*data));
    }
    return std::nullopt;
}
