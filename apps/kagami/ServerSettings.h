#pragma once

#include "configuration/JsonConfiguration.h"

#include <algorithm>
#include <string>

/// Server-specific configuration backed by kagami.json.
///
/// Access via ServerSettings::instance(). All keys have sensible defaults
/// so the server runs out of the box with no config file.
///
/// To add a new setting: add it to the defaults in the constructor
/// and add an accessor.
class ServerSettings : public JsonConfiguration<ServerSettings> {
  public:
    std::string server_name() const {
        return value<std::string>("server_name");
    }
    std::string server_description() const {
        return value<std::string>("server_description");
    }

    int http_port() const {
        return value<int>("http_port");
    }
    int ws_port() const {
        return value<int>("ws_port");
    }
    std::string bind_address() const {
        return value<std::string>("bind_address");
    }

    int max_players() const {
        return value<int>("max_players");
    }
    std::string motd() const {
        return value<std::string>("motd");
    }

    /// Session TTL in seconds. 0 = no expiry.
    int session_ttl_seconds() const {
        return std::max(0, value<int>("session_ttl_seconds"));
    }

    /// Returns the configured CORS origins.
    /// Supports both a single string and an array of strings in config:
    ///   "cors_origin": "*"
    ///   "cors_origin": "https://example.com"
    ///   "cors_origin": ["https://a.com", "https://b.com"]
    std::vector<std::string> cors_origins() const {
        auto raw = value<nlohmann::json>("cors_origin");
        if (raw.is_string()) {
            auto s = raw.get<std::string>();
            if (s.empty())
                return {};
            return {s};
        }
        if (raw.is_array()) {
            std::vector<std::string> result;
            for (const auto& item : raw) {
                if (item.is_string())
                    result.push_back(item.get<std::string>());
            }
            return result;
        }
        return {};
    }

    static bool load_from_disk(const std::string& path);
    static bool save_to_disk(const std::string& path);

  private:
    friend class ConfigurationBase<ServerSettings>;
    ServerSettings() {
        set_defaults({
            {"server_name", "Kagami Server"},
            {"server_description", ""},
            {"http_port", 8080},
            {"ws_port", 8081},
            {"bind_address", "0.0.0.0"},
            {"max_players", 100},
            {"motd", ""},
            {"session_ttl_seconds", 300},
            {"cors_origin", "https://web.aceattorneyonline.com"},
        });
    }
};
