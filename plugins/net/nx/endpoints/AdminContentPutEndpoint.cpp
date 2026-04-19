#include "net/nx/NXEndpoint.h"

#include "game/ACLFlags.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"
#include "utils/Log.h"

#include <json.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace {

/// Hard cap on entries per content array. Prevents accidental (or
/// malicious) OOM from a runaway PUT. SUPER-gated, but still bounded.
constexpr size_t kMaxContentEntries = 10000;

/// Derive the content config directory from the kagami.json config path.
std::string content_dir_from_cfg_path(const std::string& cfg_path) {
    auto dir = std::filesystem::path(cfg_path).parent_path();
    return (dir / "config").string();
}

/// Atomically replace `path` with `contents`: write to `path.tmp`, fsync,
/// rename. Rename is atomic on POSIX and Windows (MoveFileExW with
/// MOVEFILE_REPLACE_EXISTING), so a mid-write crash leaves either the
/// old file intact or the fully-written new file — never a truncated one.
bool atomic_write(const std::filesystem::path& path, const std::string& contents) {
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
            return false;
        f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        f.flush();
        if (!f.good())
            return false;
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return false;
    }
    return true;
}

/// Join lines with '\n'.
std::string join_lines(const std::vector<std::string>& lines) {
    std::string out;
    out.reserve(lines.size() * 16);
    for (const auto& line : lines) {
        out.append(line);
        out.push_back('\n');
    }
    return out;
}

/// Serialize a music list (interleaved categories + song names) back into
/// akashi music.json format. Category headers are lines that don't look
/// like paths (no '.' or '/'); anything else is a song.
std::string build_music_json(const std::vector<std::string>& flat) {
    nlohmann::json root = nlohmann::json::array();
    nlohmann::json* current_cat = nullptr;

    auto ensure_default_cat = [&] {
        if (!current_cat) {
            root.push_back({{"category", ""}, {"songs", nlohmann::json::array()}});
            current_cat = &root.back();
        }
    };

    auto looks_like_category = [](const std::string& s) {
        return s.find('.') == std::string::npos && s.find('/') == std::string::npos;
    };

    for (const auto& item : flat) {
        if (looks_like_category(item)) {
            root.push_back({{"category", item}, {"songs", nlohmann::json::array()}});
            current_cat = &root.back();
        }
        else {
            ensure_default_cat();
            (*current_cat)["songs"].push_back({{"name", item}, {"length", -1}});
        }
    }

    return root.dump(2);
}

// -- areas.ini: minimal INI parser/serializer for read-modify-write -----------
//
// Preserves every section key that we don't explicitly know about, and
// retains the order of keys within each section. New areas inherit safe
// defaults (matching ContentConfig::load_areas_ini).

struct IniKV {
    std::string key;
    std::string value;
};

struct IniSection {
    std::string header; ///< "N:Name"
    std::vector<IniKV> kvs;

    std::string* find(const std::string& key) {
        for (auto& kv : kvs)
            if (kv.key == key)
                return &kv.value;
        return nullptr;
    }

    void upsert(const std::string& key, const std::string& value) {
        if (auto* v = find(key)) {
            *v = value;
            return;
        }
        kvs.push_back({key, value});
    }
};

std::string trim_copy(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<IniSection> parse_areas_ini(const std::filesystem::path& path) {
    std::vector<IniSection> sections;
    std::ifstream f(path);
    if (!f.is_open())
        return sections;
    IniSection* current = nullptr;
    std::string line;
    while (std::getline(f, line)) {
        auto t = trim_copy(line);
        if (t.empty() || t.front() == ';' || t.front() == '#')
            continue;
        if (t.front() == '[' && t.back() == ']') {
            sections.push_back({t.substr(1, t.size() - 2), {}});
            current = &sections.back();
            continue;
        }
        if (!current)
            continue;
        auto eq = t.find('=');
        if (eq == std::string::npos)
            continue;
        current->kvs.push_back({trim_copy(t.substr(0, eq)), trim_copy(t.substr(eq + 1))});
    }
    return sections;
}

std::string serialize_areas_ini(const std::vector<IniSection>& sections) {
    std::string out;
    for (const auto& sec : sections) {
        out.push_back('[');
        out.append(sec.header);
        out.append("]\n");
        for (const auto& kv : sec.kvs) {
            out.append(kv.key);
            out.push_back('=');
            out.append(kv.value);
            out.push_back('\n');
        }
        out.push_back('\n');
    }
    return out;
}

/// Merge a new area name list onto existing areas.ini, preserving
/// per-section keys (background, evidence_mod, iniswap_allowed, …).
///
/// Matching is by area NAME (the suffix after "N:"). Sections for
/// missing names are dropped. New names get a default section with the
/// same defaults ContentConfig uses when a key is absent.
///
/// Indices are reassigned to match the new ordering so that saves
/// survive round-trips with reorderings.
std::vector<IniSection> merge_areas(std::vector<IniSection> existing, const std::vector<std::string>& new_names) {
    // Index existing by name for O(1) lookup.
    std::unordered_map<std::string, size_t> by_name;
    by_name.reserve(existing.size());
    for (size_t i = 0; i < existing.size(); ++i) {
        const auto& h = existing[i].header;
        auto colon = h.find(':');
        auto name = (colon == std::string::npos) ? h : h.substr(colon + 1);
        by_name.emplace(name, i);
    }

    std::vector<IniSection> out;
    out.reserve(new_names.size());
    for (size_t i = 0; i < new_names.size(); ++i) {
        const auto& name = new_names[i];
        auto it = by_name.find(name);
        IniSection sec;
        if (it != by_name.end()) {
            sec = std::move(existing[it->second]);
        }
        else {
            sec.kvs.push_back({"background", "gs4"});
        }
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%zu:", i);
        sec.header = std::string(buf) + name;
        out.push_back(std::move(sec));
    }
    return out;
}

class AdminContentPutEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "PUT";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/content";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (!req.session || !req.session->moderator)
            return RestResponse::error(403, "Authentication required");
        if (!has_permission(acl_permissions_for_role(req.session->acl_role), ACLPermission::SUPER))
            return RestResponse::error(403, "SUPER privileges required");

        if (!req.body || !req.body->is_object())
            return RestResponse::error(400, "Request body must be a JSON object");

        // Size-check every array up front so partial writes are impossible
        // once we start touching the filesystem.
        auto validate_array = [&](const char* name, std::vector<std::string>& out) -> std::optional<RestResponse> {
            if (!req.body->contains(name))
                return std::nullopt;
            if (!(*req.body)[name].is_array())
                return RestResponse::error(400, std::string(name) + " must be an array");
            if ((*req.body)[name].size() > kMaxContentEntries)
                return RestResponse::error(413, std::string(name) + " exceeds " + std::to_string(kMaxContentEntries) +
                                                    " entries");
            for (const auto& v : (*req.body)[name])
                if (v.is_string())
                    out.push_back(v.get<std::string>());
            return std::nullopt;
        };

        std::vector<std::string> chars, areas, music, bgs;
        bool has_chars = req.body->contains("characters");
        bool has_areas = req.body->contains("areas");
        bool has_music = req.body->contains("music");
        bool has_bgs = req.body->contains("backgrounds");

        if (auto err = validate_array("characters", chars); err)
            return std::move(*err);
        if (auto err = validate_array("areas", areas); err)
            return std::move(*err);
        if (auto err = validate_array("music", music); err)
            return std::move(*err);
        if (auto err = validate_array("backgrounds", bgs); err)
            return std::move(*err);

        if (!(has_chars || has_areas || has_music || has_bgs))
            return RestResponse::error(400, "No valid content fields in request body");

        namespace fs = std::filesystem;
        fs::path dir = content_dir_from_cfg_path(cfg_path());
        std::error_code ec;
        fs::create_directories(dir, ec);

        std::vector<std::string> updated;

        if (has_chars) {
            if (!atomic_write(dir / "characters.txt", join_lines(chars)))
                return RestResponse::error(500, "Failed to write characters.txt");
            updated.push_back("characters (" + std::to_string(chars.size()) + ")");
        }

        if (has_areas) {
            // Read-modify-write: preserve per-area settings.
            auto existing = parse_areas_ini(dir / "areas.ini");
            auto merged = merge_areas(std::move(existing), areas);
            if (!atomic_write(dir / "areas.ini", serialize_areas_ini(merged)))
                return RestResponse::error(500, "Failed to write areas.ini");
            updated.push_back("areas (" + std::to_string(areas.size()) + ")");
        }

        if (has_music) {
            // ContentConfig reads music.json, not music.txt. Serialize back
            // into the akashi category/songs schema.
            if (!atomic_write(dir / "music.json", build_music_json(music)))
                return RestResponse::error(500, "Failed to write music.json");
            updated.push_back("music (" + std::to_string(music.size()) + ")");
        }

        if (has_bgs) {
            if (!atomic_write(dir / "backgrounds.txt", join_lines(bgs)))
                return RestResponse::error(500, "Failed to write backgrounds.txt");
            updated.push_back("backgrounds (" + std::to_string(bgs.size()) + ")");
        }

        std::string summary;
        const auto& reload = room().reload_func();
        if (reload)
            summary = reload();

        Log::log_print(INFO, "Admin: content updated: %s", nlohmann::json(updated).dump().c_str());
        return RestResponse::json(200, {{"updated", updated}, {"reload_summary", summary}});
    }
};

EndpointRegistrar reg("PUT /aonx/v1/admin/content", [] { return std::make_unique<AdminContentPutEndpoint>(); });

} // namespace

void nx_ep_admin_content_put() {
}
