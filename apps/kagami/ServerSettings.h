#pragma once

#include "configuration/JsonConfiguration.h"
#include "game/ASNReputationManager.h"
#include "game/FirewallManager.h"
#include "game/IPReputationService.h"
#include "game/SpamDetector.h"
#include "net/ReverseProxyConfig.h"
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

    // -- IP Reputation --

    ReputationConfig reputation_config() const {
        ReputationConfig cfg;
        cfg.enabled = value<bool>("reputation/enabled");
        cfg.cache_ttl_hours = value<int>("reputation/cache_ttl_hours");
        cfg.cache_failure_ttl_minutes = value<int>("reputation/cache_failure_ttl_minutes");
        cfg.ip_api_enabled = value<bool>("reputation/ip_api_enabled");
        cfg.abuseipdb_api_key = value<std::string>("reputation/abuseipdb_api_key");
        cfg.abuseipdb_daily_budget = value<int>("reputation/abuseipdb_daily_budget");
        cfg.auto_block_proxy = value<bool>("reputation/auto_block_proxy");
        cfg.auto_block_datacenter = value<bool>("reputation/auto_block_datacenter");
        return cfg;
    }

    // -- ASN Reputation --

    ASNReputationConfig asn_reputation_config() const {
        ASNReputationConfig cfg;
        cfg.enabled = value<bool>("asn_reputation/enabled");
        cfg.watch_threshold = value<int>("asn_reputation/watch_threshold");
        cfg.rate_limit_threshold = value<int>("asn_reputation/rate_limit_threshold");
        cfg.block_threshold = value<int>("asn_reputation/block_threshold");
        cfg.window_minutes = value<int>("asn_reputation/window_minutes");
        cfg.auto_block_duration = value<std::string>("asn_reputation/auto_block_duration");
        auto whitelist = value<nlohmann::json>("asn_reputation/whitelist_asns");
        if (whitelist.is_array()) {
            for (auto& item : whitelist) {
                if (item.is_number_unsigned())
                    cfg.whitelist_asns.push_back(item.get<uint32_t>());
            }
        }
        cfg.whitelist_multiplier = value<int>("asn_reputation/whitelist_multiplier");
        return cfg;
    }

    // -- Spam Detection --

    SpamDetectorConfig spam_detection_config() const {
        SpamDetectorConfig cfg;
        cfg.enabled = value<bool>("spam_detection/enabled");
        cfg.echo_threshold = value<int>("spam_detection/echo_threshold");
        cfg.echo_window_seconds = value<int>("spam_detection/echo_window_seconds");
        cfg.burst_threshold = value<int>("spam_detection/burst_threshold");
        cfg.burst_window_seconds = value<int>("spam_detection/burst_window_seconds");
        cfg.join_spam_max_seconds = value<int>("spam_detection/join_spam_max_seconds");
        cfg.name_pattern_threshold = value<int>("spam_detection/name_pattern_threshold");
        cfg.name_pattern_min_prefix = value<int>("spam_detection/name_pattern_min_prefix");
        cfg.name_pattern_window_seconds = value<int>("spam_detection/name_pattern_window_seconds");
        cfg.ghost_threshold = value<int>("spam_detection/ghost_threshold");
        cfg.hwid_reuse_threshold = value<int>("spam_detection/hwid_reuse_threshold");
        return cfg;
    }

    // -- Firewall --

    FirewallConfig firewall_config() const {
        FirewallConfig cfg;
        cfg.enabled = value<bool>("firewall/enabled");
        cfg.helper_path = value<std::string>("firewall/helper_path");
        cfg.cleanup_on_shutdown = value<bool>("firewall/cleanup_on_shutdown");
        return cfg;
    }

    // -- Reverse proxy --

    ReverseProxyConfig reverse_proxy_config() const {
        ReverseProxyConfig cfg;
        cfg.enabled = value<bool>("reverse_proxy/enabled");
        cfg.proxy_protocol = value<bool>("reverse_proxy/proxy_protocol");
        auto proxies = value<nlohmann::json>("reverse_proxy/trusted_proxies");
        if (proxies.is_array()) {
            for (auto& item : proxies) {
                if (item.is_string())
                    cfg.trusted_proxies.push_back(item.get<std::string>());
            }
        }
        auto headers = value<nlohmann::json>("reverse_proxy/header_priority");
        if (headers.is_array()) {
            cfg.header_priority.clear();
            for (auto& item : headers) {
                if (item.is_string())
                    cfg.header_priority.push_back(item.get<std::string>());
            }
        }
        return cfg;
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
            {"reverse_proxy",
             nlohmann::json{
                 {"enabled", false},
                 {"trusted_proxies", nlohmann::json::array()},
                 {"header_priority", nlohmann::json::array({"X-Forwarded-For", "X-Real-IP"})},
                 {"proxy_protocol", false},
             }},
            {"reputation",
             nlohmann::json{
                 {"enabled", true},
                 {"cache_ttl_hours", 24},
                 {"cache_failure_ttl_minutes", 5},
                 {"ip_api_enabled", true},
                 {"abuseipdb_api_key", ""},
                 {"abuseipdb_daily_budget", 1000},
                 {"auto_block_proxy", false},
                 {"auto_block_datacenter", false},
             }},
            {"asn_reputation",
             nlohmann::json{
                 {"enabled", true},
                 {"watch_threshold", 2},
                 {"rate_limit_threshold", 3},
                 {"block_threshold", 5},
                 {"window_minutes", 60},
                 {"auto_block_duration", "24h"},
                 {"whitelist_asns", nlohmann::json::array()},
                 {"whitelist_multiplier", 5},
             }},
            {"spam_detection",
             nlohmann::json{
                 {"enabled", true},
                 {"echo_threshold", 3},
                 {"echo_window_seconds", 60},
                 {"burst_threshold", 20},
                 {"burst_window_seconds", 30},
                 {"join_spam_max_seconds", 5},
                 {"name_pattern_threshold", 3},
                 {"name_pattern_min_prefix", 4},
                 {"name_pattern_window_seconds", 300},
                 {"ghost_threshold", 5},
                 {"hwid_reuse_threshold", 3},
             }},
            {"firewall",
             nlohmann::json{
                 {"enabled", false},
                 {"helper_path", ""},
                 {"cleanup_on_shutdown", true},
             }},
            {"rate_limits",
             nlohmann::json{
                 // Transport-level
                 {"session_create", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"ws_frame", nlohmann::json{{"rate", 30.0}, {"burst", 60.0}}},
                 {"ws_bytes", nlohmann::json{{"rate", 32768.0}, {"burst", 65536.0}}},
                 // AO protocol: high-frequency gameplay
                 {"ao:MS", nlohmann::json{{"rate", 5.0}, {"burst", 10.0}}},
                 {"ao:CT", nlohmann::json{{"rate", 3.0}, {"burst", 6.0}}},
                 {"ao:CC", nlohmann::json{{"rate", 2.0}, {"burst", 4.0}}},
                 {"ao:MC", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"ao:CH", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 // AO protocol: area/evidence mutations
                 {"ao:HP", nlohmann::json{{"rate", 2.0}, {"burst", 4.0}}},
                 {"ao:BN", nlohmann::json{{"rate", 2.0}, {"burst", 4.0}}},
                 {"ao:RT", nlohmann::json{{"rate", 3.0}, {"burst", 6.0}}},
                 {"ao:PE", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"ao:EE", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"ao:DE", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 // AO protocol: moderation/announcements
                 {"ao:ZZ", nlohmann::json{{"rate", 1.0}, {"burst", 3.0}}},
                 {"ao:CASEA", nlohmann::json{{"rate", 0.5}, {"burst", 2.0}}},
                 {"ao:SETCASE", nlohmann::json{{"rate", 1.0}, {"burst", 3.0}}},
                 // AONX REST endpoints
                 {"nx:ooc", nlohmann::json{{"rate", 3.0}, {"burst", 6.0}}},
                 {"nx:area_join", nlohmann::json{{"rate", 2.0}, {"burst", 5.0}}},
                 {"nx:char_select", nlohmann::json{{"rate", 2.0}, {"burst", 4.0}}},
                 {"nx:session_renew", nlohmann::json{{"rate", 1.0}, {"burst", 3.0}}},
                 // Timeouts
                 {"ws_handshake_deadline_sec", 10},
                 {"ws_idle_timeout_sec", 120},
                 {"ws_partial_frame_timeout_sec", 30},
                 {"ws_max_frame_size", 65536},
             }},
        });
    }
};
