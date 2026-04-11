#pragma once

#include "configuration/JsonConfiguration.h"
#include "game/ASNReputationManager.h"
#include "game/FirewallManager.h"
#include "game/IPReputationService.h"
#include "game/SpamDetector.h"
#include "moderation/ContentModerationConfig.h"
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
    /// Public domain name. Used for Caddyfile generation and advertiser hostname.
    std::string domain() const {
        return value<std::string>("domain");
    }

    int http_port() const {
        return value<int>("http_port");
    }
    int ws_port() const {
        return value<int>("ws_port");
    }
    /// TLS WebSocket port (Caddy listens here, proxies to ws_port).
    int wss_port() const {
        return value<int>("wss_port");
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
    /// Asset URL sent to AO2 clients via the ASS packet. Empty = none.
    std::string asset_url() const {
        return value<std::string>("asset_url");
    }

    /// Moderator password for simple auth. Empty = auth disabled.
    std::string mod_password() const {
        return value<std::string>("mod_password");
    }

    /// Session TTL in seconds. 0 = no expiry.
    int session_ttl_seconds() const {
        return std::max(0, value<int>("session_ttl_seconds"));
    }

    // -- Message limits --

    /// Maximum length of an IC (in-character) message in characters. 0 = no limit.
    int max_ic_message_length() const {
        return std::max(0, value<int>("max_ic_message_length"));
    }
    /// Maximum length of an OOC (out-of-character) message in characters. 0 = no limit.
    int max_ooc_message_length() const {
        return std::max(0, value<int>("max_ooc_message_length"));
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

    // -- Content Moderation --

    moderation::ContentModerationConfig content_moderation_config() const {
        moderation::ContentModerationConfig cfg;
        cfg.enabled = value<bool>("content_moderation/enabled");
        cfg.check_ic = value<bool>("content_moderation/check_ic");
        cfg.check_ooc = value<bool>("content_moderation/check_ooc");
        cfg.message_sample_length = value<int>("content_moderation/message_sample_length");

        // Unicode layer
        cfg.unicode.enabled = value<bool>("content_moderation/unicode/enabled");
        cfg.unicode.combining_mark_threshold = value<double>("content_moderation/unicode/combining_mark_threshold");
        cfg.unicode.exotic_script_threshold = value<double>("content_moderation/unicode/exotic_script_threshold");
        cfg.unicode.format_char_threshold = value<double>("content_moderation/unicode/format_char_threshold");
        cfg.unicode.max_score = value<double>("content_moderation/unicode/max_score");

        // URL layer
        cfg.urls.enabled = value<bool>("content_moderation/urls/enabled");
        cfg.urls.blocked_score = value<double>("content_moderation/urls/blocked_score");
        cfg.urls.unknown_url_score = value<double>("content_moderation/urls/unknown_url_score");
        auto block_j = value<nlohmann::json>("content_moderation/urls/blocklist");
        if (block_j.is_array()) {
            for (auto& s : block_j)
                if (s.is_string())
                    cfg.urls.blocklist.push_back(s.get<std::string>());
        }
        auto allow_j = value<nlohmann::json>("content_moderation/urls/allowlist");
        if (allow_j.is_array()) {
            for (auto& s : allow_j)
                if (s.is_string())
                    cfg.urls.allowlist.push_back(s.get<std::string>());
        }

        // Slur wordlist layer (Layer 1c)
        cfg.slurs.enabled = value<bool>("content_moderation/slurs/enabled");
        cfg.slurs.wordlist_url = value<std::string>("content_moderation/slurs/wordlist_url");
        cfg.slurs.exceptions_url = value<std::string>("content_moderation/slurs/exceptions_url");
        cfg.slurs.cache_dir = value<std::string>("content_moderation/slurs/cache_dir");
        cfg.slurs.match_score = value<double>("content_moderation/slurs/match_score");

        // Remote classifier layer (Phase 2 wiring — config only in Phase 1)
        cfg.remote.enabled = value<bool>("content_moderation/remote/enabled");
        cfg.remote.provider = value<std::string>("content_moderation/remote/provider");
        cfg.remote.api_key = value<std::string>("content_moderation/remote/api_key");
        cfg.remote.endpoint = value<std::string>("content_moderation/remote/endpoint");
        cfg.remote.model = value<std::string>("content_moderation/remote/model");
        cfg.remote.timeout_ms = value<int>("content_moderation/remote/timeout_ms");
        cfg.remote.fail_open = value<bool>("content_moderation/remote/fail_open");
        cfg.remote.cache_enabled = value<bool>("content_moderation/remote/cache_enabled");
        cfg.remote.cache_ttl_seconds = value<int>("content_moderation/remote/cache_ttl_seconds");
        cfg.remote.cache_max_entries = value<int>("content_moderation/remote/cache_max_entries");

        // Safe-hint shortcut (Layer 2 optimization)
        cfg.safe_hint.enabled = value<bool>("content_moderation/safe_hint/enabled");
        cfg.safe_hint.anchors_url = value<std::string>("content_moderation/safe_hint/anchors_url");
        cfg.safe_hint.cache_dir = value<std::string>("content_moderation/safe_hint/cache_dir");
        cfg.safe_hint.similarity_threshold = value<double>("content_moderation/safe_hint/similarity_threshold");

        // Embeddings layer (Phase 3 wiring — config only in Phase 1)
        cfg.embeddings.enabled = value<bool>("content_moderation/embeddings/enabled");
        cfg.embeddings.hf_model_id = value<std::string>("content_moderation/embeddings/hf_model_id");
        cfg.embeddings.cache_dir = value<std::string>("content_moderation/embeddings/cache_dir");
        cfg.embeddings.ring_size = value<int>("content_moderation/embeddings/ring_size");
        cfg.embeddings.similarity_threshold = value<double>("content_moderation/embeddings/similarity_threshold");
        cfg.embeddings.cluster_threshold = value<int>("content_moderation/embeddings/cluster_threshold");
        cfg.embeddings.window_seconds = value<int>("content_moderation/embeddings/window_seconds");

        // Heat ladder
        cfg.heat.decay_half_life_seconds = value<double>("content_moderation/heat/decay_half_life_seconds");
        cfg.heat.censor_threshold = value<double>("content_moderation/heat/censor_threshold");
        cfg.heat.drop_threshold = value<double>("content_moderation/heat/drop_threshold");
        cfg.heat.mute_threshold = value<double>("content_moderation/heat/mute_threshold");
        cfg.heat.kick_threshold = value<double>("content_moderation/heat/kick_threshold");
        cfg.heat.ban_threshold = value<double>("content_moderation/heat/ban_threshold");
        cfg.heat.perma_ban_threshold = value<double>("content_moderation/heat/perma_ban_threshold");
        cfg.heat.mute_duration_seconds = value<int>("content_moderation/heat/mute_duration_seconds");
        cfg.heat.ban_duration_seconds = value<int>("content_moderation/heat/ban_duration_seconds");
        cfg.heat.weight_visual_noise = value<double>("content_moderation/heat/weight_visual_noise");
        cfg.heat.weight_link_risk = value<double>("content_moderation/heat/weight_link_risk");
        cfg.heat.weight_slurs = value<double>("content_moderation/heat/weight_slurs");
        cfg.heat.weight_toxicity = value<double>("content_moderation/heat/weight_toxicity");
        cfg.heat.weight_hate = value<double>("content_moderation/heat/weight_hate");
        cfg.heat.weight_sexual = value<double>("content_moderation/heat/weight_sexual");
        cfg.heat.weight_sexual_minors = value<double>("content_moderation/heat/weight_sexual_minors");
        cfg.heat.weight_violence = value<double>("content_moderation/heat/weight_violence");
        cfg.heat.weight_self_harm = value<double>("content_moderation/heat/weight_self_harm");
        cfg.heat.weight_semantic_echo = value<double>("content_moderation/heat/weight_semantic_echo");

        // Trust bank (negative heat accrual for API-call skip)
        cfg.trust_bank.enabled = value<bool>("content_moderation/trust_bank/enabled");
        cfg.trust_bank.clean_reward = value<double>("content_moderation/trust_bank/clean_reward");
        cfg.trust_bank.max_trust = value<double>("content_moderation/trust_bank/max_trust");
        cfg.trust_bank.api_skip_threshold = value<double>("content_moderation/trust_bank/api_skip_threshold");
        cfg.trust_bank.min_sample_rate = value<double>("content_moderation/trust_bank/min_sample_rate");

        // Local linear classifier (bundled weights, Layer 2 shortcut)
        cfg.local_classifier.enabled = value<bool>("content_moderation/local_classifier/enabled");
        cfg.local_classifier.confidence_high_skip =
            value<double>("content_moderation/local_classifier/confidence_high_skip");
        cfg.local_classifier.confidence_low_clean =
            value<double>("content_moderation/local_classifier/confidence_low_clean");

        // Audit sinks
        cfg.audit.stdout_enabled = value<bool>("content_moderation/audit/stdout_enabled");
        cfg.audit.file_path = value<std::string>("content_moderation/audit/file_path");
        cfg.audit.loki_url = value<std::string>("content_moderation/audit/loki_url");
        cfg.audit.loki_stream_label = value<std::string>("content_moderation/audit/loki_stream_label");
        cfg.audit.cloudwatch_log_group = value<std::string>("content_moderation/audit/cloudwatch_log_group");
        cfg.audit.cloudwatch_log_stream = value<std::string>("content_moderation/audit/cloudwatch_log_stream");
        cfg.audit.sqlite_enabled = value<bool>("content_moderation/audit/sqlite_enabled");
        cfg.audit.min_action = value<std::string>("content_moderation/audit/min_action");
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

    // -- Master server advertiser --

    bool advertiser_enabled() const {
        return value<bool>("advertiser/enabled");
    }
    /// Master server list URL (e.g. "https://servers.aceattorneyonline.com/servers").
    std::string advertiser_url() const {
        return value<std::string>("advertiser/url");
    }
    /// Override WS port for advertising (0 = use ws_port). Useful when Caddy
    /// serves WS on port 80 but kagami listens on 27016 internally.
    int advertiser_ws_port() const {
        return value<int>("advertiser/ws_port");
    }
    /// Override WSS port for advertising (0 = use wss_port).
    int advertiser_wss_port() const {
        return value<int>("advertiser/wss_port");
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
            {"domain", ""},
            {"http_port", 8080},
            {"ws_port", 27016},
            {"wss_port", 27015},
            {"bind_address", "0.0.0.0"},
            {"max_players", 100},
            {"motd", ""},
            {"asset_url", ""},
            {"mod_password", ""},
            {"session_ttl_seconds", 300},
            {"max_ic_message_length", 256},
            {"max_ooc_message_length", 256},
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
            {"advertiser",
             nlohmann::json{
                 {"enabled", false},
                 {"url", "https://servers.aceattorneyonline.com/servers"},
                 {"ws_port", 0},
                 {"wss_port", 0},
             }},
            {"deploy",
             nlohmann::json{
                 {"tls", "auto"},
                 {"observability", false},
                 {"metrics_allow", ""},
                 {"image", "ghcr.io/attorneyonline/kagami:latest"},
                 {"grafana_user", "admin"},
                 {"grafana_password", "changeme"},
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
            {"content_moderation",
             nlohmann::json{
                 // Master kill switch. All layers are opt-in; even when
                 // the subsystem is enabled each sub-layer has its own
                 // flag. The defaults here produce zero behavior change.
                 {"enabled", false},
                 {"check_ic", true},
                 {"check_ooc", true},
                 {"message_sample_length", 200},
                 {"unicode",
                  nlohmann::json{
                      {"enabled", false},
                      {"combining_mark_threshold", 0.3},
                      {"exotic_script_threshold", 0.3},
                      {"format_char_threshold", 0.1},
                      {"max_score", 1.0},
                  }},
                 {"urls",
                  nlohmann::json{
                      {"enabled", false},
                      {"blocklist", nlohmann::json::array()},
                      {"allowlist", nlohmann::json::array()},
                      {"blocked_score", 1.0},
                      {"unknown_url_score", 0.0},
                  }},
                 {"slurs",
                  nlohmann::json{
                      // Layer 1c: word-boundary wordlist with
                      // Scunthorpe-safe matching. Inert by default:
                      // an empty wordlist_url keeps SlurFilter quiet
                      // even if enabled=true. Operators supply both
                      // URLs via CFN parameters, or leave blank to
                      // skip the layer entirely.
                      {"enabled", false},
                      {"wordlist_url", ""},
                      {"exceptions_url", ""},
                      {"cache_dir", "/tmp/kagami-moderation"},
                      {"match_score", 1.0},
                  }},
                 {"remote",
                  nlohmann::json{
                      // Remote classifier (OpenAI omni-moderation).
                      // Opt-in: requires enabled=true AND a non-empty
                      // api_key. An empty api_key automatically disables
                      // the layer regardless of the enabled flag.
                      {"enabled", false},
                      {"provider", "openai"},
                      {"api_key", ""},
                      {"endpoint", "https://api.openai.com/v1/moderations"},
                      {"model", "omni-moderation-latest"},
                      // 3s default leaves headroom for OpenAI's cold-
                      // start TLS handshake (observed ~1.6s on fresh
                      // connections from EC2). Keep-alive reuse drops
                      // subsequent calls to ~200ms; dropping timeout
                      // below ~2000 causes cold-start false failures.
                      {"timeout_ms", 3000},
                      {"fail_open", true},
                      // Dedup cache for repeat messages. Off by
                      // default; see RemoteDedupCache.h.
                      {"cache_enabled", false},
                      {"cache_ttl_seconds", 300},
                      {"cache_max_entries", 1000},
                  }},
                 {"safe_hint",
                  nlohmann::json{
                      // Layer 2 shortcut. Requires the embeddings layer
                      // backend to be loaded; otherwise inert. Anchors
                      // are fetched from the operator-supplied URL at
                      // startup in the same background thread as the
                      // slur wordlist. A 0.7 threshold is calibrated
                      // for bge-small-en-v1.5 — tune per-model.
                      {"enabled", false},
                      {"anchors_url", ""},
                      {"cache_dir", "/tmp/kagami-moderation"},
                      {"similarity_threshold", 0.7},
                  }},
                 {"embeddings",
                  nlohmann::json{
                      // Phase 3: wired later via llama.cpp. Empty model
                      // id = layer inert. No model bundled with binary.
                      {"enabled", false},
                      {"hf_model_id", ""},
                      // Model cache dir. Shared with slurs/safe_hint in
                      // deploy layouts so a single bind-mounted volume
                      // persists all moderation downloads across
                      // container recreates. Default matches the other
                      // moderation layer caches for consistency.
                      {"cache_dir", "/tmp/kagami-moderation"},
                      {"ring_size", 500},
                      {"similarity_threshold", 0.9},
                      {"cluster_threshold", 3},
                      {"window_seconds", 60},
                  }},
                 {"heat",
                  nlohmann::json{
                      {"decay_half_life_seconds", 600.0},
                      {"censor_threshold", 1.0},
                      {"drop_threshold", 3.0},
                      {"mute_threshold", 6.0},
                      {"kick_threshold", 10.0},
                      {"ban_threshold", 15.0},
                      {"perma_ban_threshold", 25.0},
                      {"mute_duration_seconds", 15 * 60},
                      {"ban_duration_seconds", 24 * 60 * 60},
                      {"weight_visual_noise", 0.5},
                      {"weight_link_risk", 5.0},
                      // 6.0 × match_score 1.0 = 6.0, which is
                      // mute_threshold — a single wordlist hit is
                      // an instant mute. See HeatConfig::weight_slurs
                      // inline docs for tuning options.
                      {"weight_slurs", 6.0},
                      // Toxicity and violence weights default to the
                      // roleplay-friendly tuning (1.0). The per-axis
                      // floors in ContentModerator.cpp
                      // (kAxisFloorToxicity=0.85, kAxisFloorViolence=0.85)
                      // already filter out dramatic in-character
                      // language, so a 1.0 weight gives a single
                      // genuinely-hostile message roughly 1.0 heat —
                      // LOG level, needs repetition to escalate. General
                      // chat servers that want stricter defaults can
                      // raise these to 2.0-3.0 via config.
                      {"weight_toxicity", 1.0},
                      {"weight_hate", 4.0},
                      {"weight_sexual", 1.5},
                      {"weight_sexual_minors", 100.0},
                      {"weight_violence", 1.0},
                      {"weight_self_harm", 1.0},
                      {"weight_semantic_echo", 2.0},
                  }},
                 {"trust_bank",
                  nlohmann::json{
                      // Orthogonal use of the heat counter: clean
                      // messages accrue NEGATIVE heat (trust credit)
                      // which probabilistically skips the expensive
                      // remote classifier call. Any positive delta
                      // resets trust to zero before penalty — trust
                      // only accelerates the skip decision, never
                      // the enforcement decision. Opt-in.
                      {"enabled", false},
                      {"clean_reward", 0.1},
                      {"max_trust", 10.0},
                      {"api_skip_threshold", 5.0},
                      {"min_sample_rate", 0.05},
                  }},
                 {"local_classifier",
                  nlohmann::json{
                      // Thin linear classifier on top of the existing
                      // embedding vector. Weights are bundled into
                      // the binary by cmake/EmbedAssets.cmake from
                      // assets/moderation/classifier-weights-v1.bin.
                      // High-confidence outputs (either side) skip
                      // the remote call; the middle band escalates.
                      // Opt-in: operators enable after rebuilding
                      // with fresh weights from scripts/train_classifier.py.
                      {"enabled", false},
                      {"confidence_high_skip", 0.9},
                      {"confidence_low_clean", 0.2},
                  }},
                 {"audit",
                  nlohmann::json{
                      {"stdout_enabled", false},
                      {"file_path", ""},
                      {"loki_url", ""},
                      {"loki_stream_label", "kagami_mod_audit"},
                      {"cloudwatch_log_group", ""},
                      {"cloudwatch_log_stream", ""},
                      {"sqlite_enabled", true},
                      {"min_action", "log"},
                  }},
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
                 {"nx:ic", nlohmann::json{{"rate", 3.0}, {"burst", 6.0}}},
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
