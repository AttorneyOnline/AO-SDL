#include "net/nx/NXEndpoint.h"

#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"

#include <filesystem>
#include <fstream>
#include <json.hpp>

// Forward-declare ServerSettings so we can access the singleton.
// The full header lives in the kagami app layer, but the singleton
// and its JsonConfiguration base are linked into the same binary.
class ServerSettings;

/// Helpers shared by all three config endpoints.
namespace {

/// Check SUPER privilege. Returns an error response if denied, nullopt if OK.
std::optional<RestResponse> require_super(const RestRequest& req) {
    if (!req.session || !req.session->moderator)
        return RestResponse::error(403, "Admin privileges required");
    if (req.session->acl_role.empty() || req.session->acl_role == "SUPER")
        return std::nullopt; // SIMPLE auth mods or SUPER role
    return RestResponse::error(403, "SUPER privileges required");
}

/// Read the on-disk config file as a JSON object.
std::pair<nlohmann::json, std::string> read_disk_config(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return {{}, "Could not open " + path};
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto parsed = nlohmann::json::parse(raw, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object())
        return {{}, "Failed to parse " + path};
    return {std::move(parsed), {}};
}

/// Atomically write JSON to disk (temp + rename, fallback to direct write).
bool write_disk_config(const std::string& path, const nlohmann::json& config) {
    auto s = config.dump(4) + "\n";
    auto tmp_path = path + ".tmp";
    {
        std::ofstream tmp(tmp_path, std::ios::binary | std::ios::trunc);
        if (tmp.is_open()) {
            tmp.write(s.data(), static_cast<std::streamsize>(s.size()));
            tmp.close();
            std::error_code ec;
            std::filesystem::rename(tmp_path, path, ec);
            if (!ec)
                return true;
            std::filesystem::remove(tmp_path, ec);
        }
    }
    // Fallback: direct write (bind mounts, EXDEV)
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return false;
    file.write(s.data(), static_cast<std::streamsize>(s.size()));
    return true;
}

/// Resolve a key path (e.g. "content_moderation/heat/censor_threshold")
/// to a JSON pointer and look it up.
std::pair<nlohmann::json, bool> resolve_key(const nlohmann::json& config, const std::string& key) {
    if (key.empty())
        return {config, true};
    auto ptr = nlohmann::json::json_pointer("/" + key);
    if (!config.contains(ptr))
        return {{}, false};
    return {config.at(ptr), true};
}

/// Remove a key from a JSON object using a slash-delimited path.
bool remove_key(nlohmann::json& config, const std::string& key) {
    if (key.find('/') == std::string::npos) {
        if (!config.contains(key))
            return false;
        config.erase(key);
        return true;
    }
    auto sep = key.rfind('/');
    auto parent_path = key.substr(0, sep);
    auto child = key.substr(sep + 1);
    auto parent_ptr = nlohmann::json::json_pointer("/" + parent_path);
    if (!config.contains(parent_ptr))
        return false;
    auto& parent = config.at(parent_ptr);
    if (!parent.is_object() || !parent.contains(child))
        return false;
    parent.erase(child);
    return true;
}

// -------------------------------------------------------------------------
// GET /admin/config
// -------------------------------------------------------------------------
class AdminConfigGetEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/config";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        // Read current config from disk (source of truth).
        auto [config, error] = read_disk_config(cfg_path());
        if (!error.empty())
            return RestResponse::error(500, error);

        // Optional key filter via query parameter.
        auto it = req.query_params.find("key");
        if (it != req.query_params.end() && !it->second.empty()) {
            auto [value, found] = resolve_key(config, it->second);
            if (!found)
                return RestResponse::error(404, "Key not found: " + it->second);
            return RestResponse::json(200, {{"key", it->second}, {"value", value}});
        }

        return RestResponse::json(200, config);
    }
};

// -------------------------------------------------------------------------
// PATCH /admin/config
// -------------------------------------------------------------------------
class AdminConfigPatchEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "PATCH";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/config";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        if (!req.body || !req.body->is_object())
            return RestResponse::error(400, "Request body must be a JSON object");

        // Read current config from disk.
        auto [config, error] = read_disk_config(cfg_path());
        if (!error.empty())
            return RestResponse::error(500, error);

        // Apply JSON Merge Patch (RFC 7396).
        config.merge_patch(*req.body);

        // Write back to disk.
        if (!write_disk_config(cfg_path(), config))
            return RestResponse::error(500, "Failed to write config to disk");

        // Trigger hot-reload to apply changes to all subsystems.
        std::string summary;
        const auto& reload = room().reload_func();
        if (reload)
            summary = reload();

        return RestResponse::json(200, {{"reload_summary", summary}});
    }
};

// -------------------------------------------------------------------------
// DELETE /admin/config?key=...
// -------------------------------------------------------------------------
class AdminConfigDeleteEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "DELETE";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/config";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (auto err = require_super(req))
            return std::move(*err);

        auto it = req.query_params.find("key");
        if (it == req.query_params.end() || it->second.empty())
            return RestResponse::error(400, "Missing required query parameter: key");

        // Read current config from disk.
        auto [config, error] = read_disk_config(cfg_path());
        if (!error.empty())
            return RestResponse::error(500, error);

        // Remove the key.
        if (!remove_key(config, it->second))
            return RestResponse::error(404, "Key not found: " + it->second);

        // Write back to disk.
        if (!write_disk_config(cfg_path(), config))
            return RestResponse::error(500, "Failed to write config to disk");

        // Trigger hot-reload.
        std::string summary;
        const auto& reload = room().reload_func();
        if (reload)
            summary = reload();

        return RestResponse::json(200, {{"reload_summary", summary}});
    }
};

// -------------------------------------------------------------------------
// Registration
// -------------------------------------------------------------------------
EndpointRegistrar reg_get("GET /aonx/v1/admin/config",
                          [] { return std::make_unique<AdminConfigGetEndpoint>(); });
EndpointRegistrar reg_patch("PATCH /aonx/v1/admin/config",
                            [] { return std::make_unique<AdminConfigPatchEndpoint>(); });
EndpointRegistrar reg_delete("DELETE /aonx/v1/admin/config",
                             [] { return std::make_unique<AdminConfigDeleteEndpoint>(); });

} // namespace

void nx_ep_admin_config() {
}
