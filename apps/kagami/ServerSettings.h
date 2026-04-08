#pragma once

#include "configuration/JsonConfiguration.h"
#include "utils/Log.h"

#include <algorithm>
#include <string>

/// Parse a log level string (case-insensitive). Returns VERBOSE on unrecognized input.
inline LogLevel parse_log_level(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "verbose")
        return VERBOSE;
    if (lower == "debug")
        return DEBUG;
    if (lower == "info")
        return INFO;
    if (lower == "warning" || lower == "warn")
        return WARNING;
    if (lower == "error" || lower == "err")
        return ERR;
    if (lower == "fatal")
        return FATAL;
    return VERBOSE;
}

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

    /// Moderator password for simple auth. Empty = auth disabled.
    std::string mod_password() const {
        return value<std::string>("mod_password");
    }

    /// Session TTL in seconds. 0 = no expiry.
    int session_ttl_seconds() const {
        return std::max(0, value<int>("session_ttl_seconds"));
    }

    // -- Rate limiting --

    /// Raw rate_limits JSON object for configuring the RateLimiter.
    nlohmann::json rate_limit_config() const {
        return value<nlohmann::json>("rate_limits");
    }

    // -- Logging --

    /// Minimum log level for stdout / terminal UI. Default: "verbose".
    LogLevel console_log_level() const {
        return parse_log_level(value<std::string>("log_level"));
    }

    /// Path to a log file. Empty = no file logging.
    std::string log_file() const {
        return value<std::string>("log_file");
    }

    /// Minimum log level for the file sink. Default: "verbose".
    LogLevel file_log_level() const {
        return parse_log_level(value<std::string>("log_file_level"));
    }

    // -- CloudWatch logging --

    /// AWS region for CloudWatch Logs (e.g. "us-east-1"). Empty = disabled.
    std::string cloudwatch_region() const {
        return value<std::string>("cloudwatch/region");
    }
    std::string cloudwatch_log_group() const {
        return value<std::string>("cloudwatch/log_group");
    }
    std::string cloudwatch_log_stream() const {
        return value<std::string>("cloudwatch/log_stream");
    }
    std::string cloudwatch_access_key_id() const {
        return value<std::string>("cloudwatch/access_key_id");
    }
    std::string cloudwatch_secret_access_key() const {
        return value<std::string>("cloudwatch/secret_access_key");
    }
    /// Flush interval in seconds (how often buffered logs are sent).
    int cloudwatch_flush_interval() const {
        return std::max(1, value<int>("cloudwatch/flush_interval"));
    }
    /// Minimum log level for CloudWatch. Default: "info".
    LogLevel cloudwatch_log_level() const {
        return parse_log_level(value<std::string>("cloudwatch/log_level"));
    }

    // -- Loki logging --

    /// Loki push URL. Empty = disabled.
    std::string loki_url() const {
        return value<std::string>("loki_url");
    }

    // -- Metrics --

    bool metrics_enabled() const {
        return value<bool>("metrics_enabled");
    }
    std::string metrics_path() const {
        return value<std::string>("metrics_path");
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
            {"mod_password", ""},
            {"session_ttl_seconds", 300},
            {"cors_origin", "https://web.aceattorneyonline.com"},
            {"log_level", "verbose"},
            {"log_file", ""},
            {"log_file_level", "verbose"},
            {"loki_url", ""},
            {"metrics_enabled", true},
            {"metrics_path", "/metrics"},
            {"cloudwatch",
             nlohmann::json{
                 {"region", ""},
                 {"log_group", ""},
                 {"log_stream", ""},
                 {"access_key_id", ""},
                 {"secret_access_key", ""},
                 {"flush_interval", 5},
                 {"log_level", "info"},
             }},
            {"rate_limits",
             nlohmann::json{
                 {"session_create", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"ws_frame", nlohmann::json{{"rate", 30.0}, {"burst", 60.0}}},
                 {"ws_bytes", nlohmann::json{{"rate", 32768.0}, {"burst", 65536.0}}},
                 {"ao:MS", nlohmann::json{{"rate", 5.0}, {"burst", 10.0}}},
                 {"ao:CT", nlohmann::json{{"rate", 3.0}, {"burst", 6.0}}},
                 {"ao:CC", nlohmann::json{{"rate", 2.0}, {"burst", 4.0}}},
                 {"ws_handshake_deadline_sec", 10},
                 {"ws_idle_timeout_sec", 120},
                 {"ws_partial_frame_timeout_sec", 30},
                 {"ws_max_frame_size", 65536},
             }},
        });
    }
};
