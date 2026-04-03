#pragma once

#include "configuration/JsonConfiguration.h"
#include "utils/Log.h"

#include <algorithm>
#include <string>

/// Parse a log level string (case-insensitive). Returns VERBOSE on unrecognized input.
inline LogLevel parse_log_level(const std::string& s) {
    if (s == "verbose" || s == "VERBOSE")
        return VERBOSE;
    if (s == "debug" || s == "DEBUG")
        return DEBUG;
    if (s == "info" || s == "INFO")
        return INFO;
    if (s == "warning" || s == "WARNING" || s == "warn" || s == "WARN")
        return WARNING;
    if (s == "error" || s == "ERROR" || s == "err" || s == "ERR")
        return ERR;
    if (s == "fatal" || s == "FATAL")
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

    /// Session TTL in seconds. 0 = no expiry.
    int session_ttl_seconds() const {
        return std::max(0, value<int>("session_ttl_seconds"));
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
            {"log_level", "verbose"},
            {"log_file", ""},
            {"log_file_level", "verbose"},
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
        });
    }
};
