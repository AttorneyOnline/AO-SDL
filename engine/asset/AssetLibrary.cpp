#include "asset/AssetLibrary.h"

#include "asset/ImageDecoder.h"
#include "asset/MountManager.h"
#include "utils/Log.h"

AssetLibrary::AssetLibrary(MountManager& mounts, size_t cache_max_bytes) : mounts(mounts), cache_(cache_max_bytes) {
}

std::shared_ptr<ImageAsset> AssetLibrary::image(const std::string& path) {
    auto cached = cache_.get(path);
    if (cached)
        return std::static_pointer_cast<ImageAsset>(cached);

    auto result = probe(path, supported_image_extensions());
    if (!result)
        return nullptr;

    auto& [resolved, data] = *result;
    std::string format = resolved.substr(resolved.rfind('.') + 1);

    // Try each decoder that claims this extension
    std::vector<ImageFrame> frames;
    for (const auto& decoder : image_decoders()) {
        for (const auto& ext : decoder->extensions()) {
            if (ext == format) {
                frames = decoder->decode(data.data(), data.size());
                if (!frames.empty())
                    goto decoded;
            }
        }
    }

    // Extension didn't match or matched decoder failed — try all decoders
    // as fallback (servers may mislabel formats, e.g. GIF with .webp extension)
    for (const auto& decoder : image_decoders()) {
        frames = decoder->decode(data.data(), data.size());
        if (!frames.empty())
            break;
    }

decoded:
    if (frames.empty())
        return nullptr;

    auto asset = std::make_shared<ImageAsset>(path, format, std::move(frames));
    cache_.insert(asset);
    return asset;
}

std::shared_ptr<Asset> AssetLibrary::audio(const std::string& path) {
    // todo: decode audio into an AudioAsset once that type exists
    auto result = probe(path, {"opus", "ogg", "mp3", "wav"});
    if (!result)
        return nullptr;

    // placeholder: return nullptr until AudioAsset is implemented
    return nullptr;
}

std::optional<IniDocument> AssetLibrary::config(const std::string& path) {
    auto data = mounts.fetch_data(path);
    if (!data)
        return std::nullopt;

    IniDocument doc;
    std::string current_section;
    std::string text(data->begin(), data->end());
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip carriage returns and leading/trailing whitespace
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
            continue;
        line = line.substr(start);

        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

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
                if (vs != std::string::npos)
                    val = val.substr(vs);
                doc[current_section][key] = val;
            }
        }
    }

    return doc;
}

std::shared_ptr<ShaderAsset> AssetLibrary::shader(const std::string& path) {
    const std::string& backend = shader_backend_;
    std::string cache_key = path + ":" + backend;
    auto cached = cache_.get(cache_key);
    if (cached)
        return std::dynamic_pointer_cast<ShaderAsset>(cached);

    std::string subdir = (backend == "Metal") ? "metal" : "glsl";
    std::string vert_path = path + "/" + subdir + "/vertex";
    std::string frag_path = path + "/" + subdir + "/fragment";

    auto vert_exts = (subdir == "metal") ? std::vector<std::string>{"metal"} : std::vector<std::string>{"glsl", "vert"};
    auto frag_exts = (subdir == "metal") ? std::vector<std::string>{"metal"} : std::vector<std::string>{"glsl", "frag"};

    auto vert_result = probe(vert_path, vert_exts);
    auto frag_result = probe(frag_path, frag_exts);
    if (!vert_result || !frag_result)
        return nullptr;

    std::string vert_src(vert_result->second.begin(), vert_result->second.end());
    std::string frag_src(frag_result->second.begin(), frag_result->second.end());

    auto asset = std::make_shared<ShaderAsset>(cache_key, subdir, std::move(vert_src), std::move(frag_src));
    cache_.insert(asset);
    return asset;
}

std::shared_ptr<Asset> AssetLibrary::font(const std::string& path) {
    auto data = mounts.fetch_data(path);
    if (!data)
        return nullptr;
    // todo: return a FontAsset once that type exists
    return nullptr;
}

std::optional<std::vector<uint8_t>> AssetLibrary::raw(const std::string& path) {
    return mounts.fetch_data(path);
}

std::vector<std::string> AssetLibrary::list(const std::string& directory) {
    // todo: MountManager needs a list() method — deferred
    return {};
}

void AssetLibrary::prefetch(const std::string& path, const std::vector<std::string>& extensions, int priority) {
    for (const auto& ext : extensions)
        mounts.prefetch(path + "." + ext, priority);
}

void AssetLibrary::prefetch_image(const std::string& path) {
    if (cache_.get(path))
        return;
    prefetch(path, {"webp", "apng", "gif", "png"});
}

void AssetLibrary::prefetch_image(const std::string& path, int asset_type, int priority) {
    if (cache_.get(path))
        return;
    auto exts = mounts.http_extensions(asset_type);
    if (exts.empty())
        exts = {"webp", "apng", "gif", "png"};
    prefetch(path, exts, priority);
}

void AssetLibrary::prefetch_config(const std::string& path) {
    mounts.prefetch(path);
}

void AssetLibrary::register_asset(std::shared_ptr<Asset> asset) {
    cache_.insert(std::move(asset));
}

void AssetLibrary::evict() {
    cache_.evict();
}

std::optional<std::pair<std::string, std::vector<uint8_t>>>
AssetLibrary::probe(const std::string& path, const std::vector<std::string>& extensions) {
    for (const auto& ext : extensions) {
        std::string candidate = path + "." + ext;
        auto data = mounts.fetch_data(candidate);
        if (data)
            return std::make_pair(candidate, std::move(*data));
    }
    return std::nullopt;
}
