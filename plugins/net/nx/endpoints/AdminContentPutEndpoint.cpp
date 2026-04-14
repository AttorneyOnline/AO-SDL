#include "net/nx/NXEndpoint.h"

#include "game/ACLFlags.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"
#include "utils/Log.h"

#include <json.hpp>

#include <filesystem>
#include <fstream>

namespace {

/// Derive the content config directory from the kagami.json config path.
/// kagami.json is next to the binary; content config is in config/ subdirectory.
std::string content_dir_from_cfg_path(const std::string& cfg_path) {
    auto dir = std::filesystem::path(cfg_path).parent_path();
    return (dir / "config").string();
}

/// Write a line-delimited text file.
bool write_lines(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        return false;
    for (const auto& line : lines)
        f << line << "\n";
    return true;
}

/// Write music.json in akashi format.
bool write_music_json(const std::string& path, const nlohmann::json& music_array) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        return false;
    f << music_array.dump(2);
    return true;
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

    RestResponse handle(const RestRequest& req) override {
        if (!req.session || !req.session->moderator)
            return RestResponse::error(403, "Authentication required");
        if (!has_permission(acl_permissions_for_role(req.session->acl_role), ACLPermission::SUPER))
            return RestResponse::error(403, "SUPER privileges required");

        if (!req.body || !req.body->is_object())
            return RestResponse::error(400, "Request body must be a JSON object");

        auto dir = content_dir_from_cfg_path(cfg_path());
        std::filesystem::create_directories(dir);

        std::vector<std::string> updated;

        // Write characters.txt
        if (req.body->contains("characters") && (*req.body)["characters"].is_array()) {
            std::vector<std::string> chars;
            for (const auto& c : (*req.body)["characters"])
                if (c.is_string())
                    chars.push_back(c.get<std::string>());
            if (write_lines(dir + "/characters.txt", chars))
                updated.push_back("characters (" + std::to_string(chars.size()) + ")");
            else
                return RestResponse::error(500, "Failed to write characters.txt");
        }

        // Write areas (simple line-delimited for now; areas.ini is complex)
        // TODO: full areas.ini writing with per-area settings
        if (req.body->contains("areas") && (*req.body)["areas"].is_array()) {
            std::vector<std::string> areas;
            for (const auto& a : (*req.body)["areas"])
                if (a.is_string())
                    areas.push_back(a.get<std::string>());

            // Write areas.ini with default settings per area
            std::ofstream f(dir + "/areas.ini", std::ios::binary | std::ios::trunc);
            if (!f.is_open())
                return RestResponse::error(500, "Failed to write areas.ini");
            for (size_t i = 0; i < areas.size(); ++i)
                f << "[" << i << ":" << areas[i] << "]\n"
                  << "background=gs4\n\n";
            updated.push_back("areas (" + std::to_string(areas.size()) + ")");
        }

        // Write music list
        if (req.body->contains("music") && (*req.body)["music"].is_array()) {
            // Music is stored as a flat list of category headers + song names
            // in the GameRoom. For now, write as plain music.json.
            std::vector<std::string> music;
            for (const auto& m : (*req.body)["music"])
                if (m.is_string())
                    music.push_back(m.get<std::string>());
            if (write_lines(dir + "/music.txt", music))
                updated.push_back("music (" + std::to_string(music.size()) + ")");
        }

        // Write backgrounds.txt
        if (req.body->contains("backgrounds") && (*req.body)["backgrounds"].is_array()) {
            std::vector<std::string> bgs;
            for (const auto& b : (*req.body)["backgrounds"])
                if (b.is_string())
                    bgs.push_back(b.get<std::string>());
            if (write_lines(dir + "/backgrounds.txt", bgs))
                updated.push_back("backgrounds (" + std::to_string(bgs.size()) + ")");
        }

        if (updated.empty())
            return RestResponse::error(400, "No valid content fields in request body");

        // Trigger hot-reload to apply content changes
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
