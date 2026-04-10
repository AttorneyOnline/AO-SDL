#include "ContentConfig.h"
#include "LogSinkSetup.h"
#include "MasterServerAdvertiser.h"
#include "MetricsCollector.h"
#include "ReplCommand.h"
#include "ReplCommandFactory.h"
#include "ServerContext.h"
#include "ServerSettings.h"
#include "TerminalUI.h"
#include "WsWorkerPool.h"

#include "game/ASNReputationManager.h"
#include "game/BanManager.h"
#include "game/DatabaseManager.h"
#include "game/FirewallManager.h"
#include "game/GameRoom.h"
#include "game/IPReputationService.h"
#include "game/SpamDetector.h"
#include "metrics/MetricsRegistry.h"
#include "moderation/ContentModerator.h"
#include "moderation/EmbeddingBackend.h"
#include "moderation/HfModelFetcher.h"
#include "moderation/ModerationAuditLog.h"
#include "moderation/TextListFetcher.h"
#include "net/EndpointFactory.h"
#include "net/Http.h"
#include "net/PlatformServerSocket.h"
#include "net/RateLimiter.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "net/ao/AOServer.h"
#include "net/nx/NXEndpoint.h"
#include "utils/Log.h"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <io.h>
#define IS_INTERACTIVE() (_isatty(_fileno(stdin)))
#else
#include <unistd.h>
#define IS_INTERACTIVE() (isatty(fileno(stdin)))
#endif

static std::stop_source stop_src;

static void signal_handler(int) {
    stop_src.request_stop();
}

static std::string config_path(const char* argv0) {
    auto bin_dir = std::filesystem::path(argv0).parent_path();
    return (bin_dir / "kagami.json").string();
}

int main(int /*argc*/, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // --- Configuration ---
    std::string cfg_path = config_path(argv[0]);
    ServerSettings::load_from_disk(cfg_path);
    auto& cfg = ServerSettings::instance();

    // --- Terminal UI + log sinks ---
    bool interactive = IS_INTERACTIVE();
    TerminalUI ui;
    LogSinkSetup log_sinks;

    // The moderation audit log is constructed here (before log_sinks
    // init) so its destructor runs AFTER log_sinks.teardown(). This
    // ensures the audit flushers stop before the audit log itself
    // goes out of scope.
    moderation::ModerationAuditLog mod_audit_log;
    log_sinks.init(cfg, ui, interactive, &mod_audit_log);

    Log::log_print(INFO, "Server: %s", cfg.server_name().c_str());

    // --- Content config (akashi-compatible) ---
    ContentConfig content;
    {
        auto bin_dir = std::filesystem::path(argv[0]).parent_path();
        auto config_dir = (bin_dir / "config").string();
        if (content.load(config_dir)) {
            Log::log_print(INFO, "Loaded content config from %s", config_dir.c_str());
        }
        else {
            Log::log_print(WARNING, "No content config in %s — using built-in defaults", config_dir.c_str());
            content.characters = {"Phoenix", "Edgeworth", "Maya", "Godot", "Apollo"};
            content.music = {"Trial.opus", "Objection.opus", "Pursuit.opus"};
            content.area_names = {"Lobby", "Courtroom 1", "Courtroom 2"};
        }
    }

    // --- Game state ---
    GameRoom room;
    room.server_name = cfg.server_name();
    room.server_description = cfg.server_description();
    room.max_players = cfg.max_players();
    room.max_ic_message_length = cfg.max_ic_message_length();
    room.max_ooc_message_length = cfg.max_ooc_message_length();
    room.asset_url = cfg.asset_url();
    room.mod_password = cfg.mod_password();
    room.characters = std::move(content.characters);
    room.music = std::move(content.music);
    room.areas = std::move(content.area_names);
    room.reset_taken();
    room.build_char_id_index();
    room.build_area_index();

    // Apply per-area settings from areas.ini
    for (auto& ac : content.area_configs) {
        auto* area = room.find_area_by_name(ac.name);
        if (!area)
            continue;
        area->background.name = ac.background;
        area->bg_locked = ac.bg_locked;
    }

    // --- Database ---
    DatabaseManager db;
    if (!db.open(DEFAULT_DB_FILE))
        Log::log_print(WARNING, "Database unavailable — running without persistent storage");
    room.set_db_manager(&db);

    // Auto-detect ADVANCED auth mode if the users table has entries.
    if (db.is_open()) {
        auto users = db.list_users().get();
        if (!users.empty()) {
            room.auth_type = AuthType::ADVANCED;
            Log::log_print(INFO, "Auth: ADVANCED mode (%zu users in database)", users.size());
        }
    }

    // --- IP Reputation ---
    IPReputationService reputation;
    reputation.configure(cfg.reputation_config());
    reputation.load("ip_reputation_cache.json");
    room.set_reputation_service(&reputation);

    // --- ASN Reputation ---
    ASNReputationManager asn_reputation;
    asn_reputation.configure(cfg.asn_reputation_config());
    asn_reputation.load("asn_reputation.json");
    room.set_asn_reputation(&asn_reputation);

    // --- Spam Detector ---
    SpamDetector spam_detector;
    spam_detector.configure(cfg.spam_detection_config());
    room.set_spam_detector(&spam_detector);

    // --- Content Moderator (opt-in) ---
    //
    // Disabled by default. Enabling requires both `content_moderation/enabled`
    // AND at least one sub-layer's `enabled` flag in kagami.json. Even with
    // everything on, check() runs only when both the layer and the channel
    // (ic/ooc) are enabled, so there is no way to "accidentally" broadcast
    // behavior to production without explicit config.
    //
    // The audit log (mod_audit_log, declared above next to log_sinks) is a
    // separate instance from the regular Log sinks — moderation events can
    // be shipped to their own file / Loki stream / CloudWatch stream without
    // dragging every INFO log along. Loki and CloudWatch audit sinks are
    // initialized by log_sinks.init() when configured. The sqlite sink (if
    // enabled) also writes rows to the moderation_events table, which is
    // queryable via DatabaseManager::query_moderation_events().
    // Heap-allocate via shared_ptr so the background fetch threads
    // (HF model + slur wordlist + safe-hint anchors below) can capture
    // it by value. The previous design captured a stack-local by
    // reference and .detach()ed, which was a use-after-free if main()
    // ever returned while a fetch was still in flight — a HF model
    // download can take minutes on first boot. The shared_ptr lets the
    // moderator outlive main() until the last fetch thread drops its
    // reference, eliminating the race entirely.
    auto content_moderator = std::make_shared<moderation::ContentModerator>();
    {
        auto cm_cfg = cfg.content_moderation_config();
        content_moderator->configure(cm_cfg);
        content_moderator->set_database(&db);
        content_moderator->set_audit_log(&mod_audit_log);

        // Parse min_action string once to avoid string compares per event.
        auto parse_action = [](const std::string& s) -> moderation::ModerationAction {
            using A = moderation::ModerationAction;
            if (s == "none")
                return A::NONE;
            if (s == "log")
                return A::LOG;
            if (s == "censor")
                return A::CENSOR;
            if (s == "drop")
                return A::DROP;
            if (s == "mute")
                return A::MUTE;
            if (s == "kick")
                return A::KICK;
            if (s == "ban")
                return A::BAN;
            if (s == "perma_ban")
                return A::PERMA_BAN;
            return A::LOG;
        };
        mod_audit_log.set_min_action(parse_action(cm_cfg.audit.min_action));

        // Register the audit sinks configured in kagami.json. Each is
        // independently opt-in; the whole set defaults to empty so a
        // fresh config produces zero log traffic.
        if (cm_cfg.audit.stdout_enabled)
            mod_audit_log.add_sink("stderr", moderation::make_stderr_sink());
        if (!cm_cfg.audit.file_path.empty()) {
            auto sink = moderation::make_file_sink(cm_cfg.audit.file_path);
            if (sink)
                mod_audit_log.add_sink("file", std::move(sink));
        }

        if (cm_cfg.enabled)
            Log::log_print(
                INFO, "ContentModerator: enabled (unicode=%s urls=%s slurs=%s remote=%s safe_hint=%s embeddings=%s)",
                cm_cfg.unicode.enabled ? "on" : "off", cm_cfg.urls.enabled ? "on" : "off",
                (cm_cfg.slurs.enabled && !cm_cfg.slurs.wordlist_url.empty()) ? "pending" : "off",
                (cm_cfg.remote.enabled && !cm_cfg.remote.api_key.empty()) ? "on" : "off",
                (cm_cfg.safe_hint.enabled && !cm_cfg.safe_hint.anchors_url.empty()) ? "pending" : "off",
                (cm_cfg.embeddings.enabled && !cm_cfg.embeddings.hf_model_id.empty()) ? "on" : "off");

        // Register the Prometheus collector even when the subsystem is
        // disabled — scrapes for kagami_moderation_* will return zero,
        // which is useful for dashboards that want to show a flatline
        // rather than a missing series.
        if (cfg.metrics_enabled())
            moderation::register_moderator_metrics(*content_moderator);

        // Phase 3: local embeddings layer. Completely optional — only
        // runs when the layer is enabled AND a HuggingFace model id is
        // configured. Model fetch + load happens on a background
        // thread so main() can reach listen() without waiting for a
        // ~1 GB download on first boot. Until the backend is ready,
        // the clusterer has a NullEmbeddingBackend installed (the
        // default from SemanticClusterer::configure) and Layer 3
        // contributes no heat — identical behavior to the layer
        // being disabled. Once the background fetch completes, the
        // backend is swapped in atomically and subsequent check()
        // calls start populating semantic_echo.
        //
        // A blocking fetch on the main thread would stall liveness
        // probes during first boot and can cause CFN stack creation
        // failures on slow networks (observed ~45s for bge-small
        // Q8_0, ~5min for larger models). The background-thread
        // approach trades "Layer 3 comes up immediately" for "no
        // deadline the load has to meet".
        if (cm_cfg.enabled && cm_cfg.embeddings.enabled && !cm_cfg.embeddings.hf_model_id.empty()) {
            auto cache_dir = (std::filesystem::path(argv[0]).parent_path() / "models").string();
            auto safe_hint_cache = cm_cfg.safe_hint.cache_dir.empty()
                                       ? (std::filesystem::path(argv[0]).parent_path() / "safe_hints").string()
                                       : cm_cfg.safe_hint.cache_dir;
            const bool safe_hint_on = cm_cfg.enabled && cm_cfg.safe_hint.enabled && !cm_cfg.safe_hint.anchors_url.empty();
            Log::log_print(INFO, "ContentModerator: scheduling embedding model load (%s)",
                           cm_cfg.embeddings.hf_model_id.c_str());
            // Capture the shared_ptr by value (NOT by reference) so the
            // moderator survives even if main() unwinds before the fetch
            // completes. The detached thread keeps a reference until
            // the lambda returns.
            std::thread([hf_id = cm_cfg.embeddings.hf_model_id, cache_dir, safe_hint_cache,
                         safe_hint_url = cm_cfg.safe_hint.anchors_url, safe_hint_on, content_moderator]() mutable {
                auto fetch = moderation::resolve_hf_model(hf_id, cache_dir);
                if (!fetch.ok) {
                    Log::log_print(WARNING, "ContentModerator: embedding fetch failed (%s); layer stays inert",
                                   fetch.error.c_str());
                    return;
                }
                auto backend = moderation::make_embedding_backend(fetch.local_path);
                if (backend && backend->is_ready()) {
                    Log::log_print(INFO, "ContentModerator: embedding backend=%s dim=%d (%s)", backend->name(),
                                   backend->dimension(), fetch.downloaded ? "downloaded" : "cached");
                    content_moderator->set_embedding_backend(std::move(backend));
                }
                else {
                    Log::log_print(WARNING, "ContentModerator: embedding backend not ready — "
                                            "was kagami built with -DKAGAMI_WITH_LLAMA_CPP=ON?");
                    return;
                }

                // Chain the safe-hint anchor fetch onto the same
                // thread. This MUST happen after set_embedding_backend
                // because SafeHintLayer::load_anchors needs a ready
                // backend to compute the anchor vectors. Running it
                // in a separate thread would race against the backend
                // install and need extra synchronization for no
                // benefit — this thread is already detached and the
                // total added latency is ~5ms × 50 anchors = 250ms.
                if (safe_hint_on) {
                    Log::log_print(INFO, "ContentModerator: scheduling safe-hint anchor fetch (%s)",
                                   safe_hint_url.c_str());
                    auto anchors = moderation::fetch_text_list(safe_hint_url, safe_hint_cache, "safe_anchors.txt");
                    if (anchors.ok) {
                        size_t loaded = content_moderator->set_safe_hint_anchors(anchors.entries);
                        Log::log_print(INFO, "ContentModerator: safe-hint layer armed (%zu/%zu anchors embedded, %s)",
                                       loaded, anchors.entries.size(),
                                       anchors.from_cache ? "cached" : (anchors.downloaded ? "fresh" : "?"));
                    }
                    else {
                        Log::log_print(WARNING, "ContentModerator: safe-hint anchor fetch failed (%s); layer inert",
                                       anchors.error.c_str());
                    }
                }
            }).detach();
        }

        // --- Layer 1c: slur wordlist fetch ------------------------------
        // Same background-thread pattern as the HF model fetch: main()
        // doesn't block on the network round-trip, and the filter stays
        // inert until the list lands. A fresh boot with no cache and a
        // dead S3 URL just leaves SlurFilter::is_active() false — Layer
        // 1c silently disables itself and the other layers continue.
        //
        // Two fetches run back-to-back on the same thread (not in
        // parallel) because they share the cache_dir and we want their
        // log lines serialized for readability. The exception list is
        // small enough (<1 KB) that the extra 200ms is irrelevant.
        if (cm_cfg.enabled && cm_cfg.slurs.enabled && !cm_cfg.slurs.wordlist_url.empty()) {
            auto slur_cache = cm_cfg.slurs.cache_dir.empty()
                                  ? (std::filesystem::path(argv[0]).parent_path() / "slurs").string()
                                  : cm_cfg.slurs.cache_dir;
            Log::log_print(INFO, "ContentModerator: scheduling slur wordlist fetch (%s)",
                           cm_cfg.slurs.wordlist_url.c_str());
            std::thread([wordlist_url = cm_cfg.slurs.wordlist_url, exceptions_url = cm_cfg.slurs.exceptions_url,
                         cache_dir = slur_cache, content_moderator]() mutable {
                auto wordlist = moderation::fetch_text_list(wordlist_url, cache_dir, "slurs.txt");
                if (wordlist.ok) {
                    content_moderator->set_slur_wordlist(wordlist.entries);
                    Log::log_print(INFO, "ContentModerator: slur wordlist loaded (%zu entries, %s)",
                                   wordlist.entries.size(),
                                   wordlist.from_cache ? "cached" : (wordlist.downloaded ? "fresh" : "?"));
                }
                else {
                    Log::log_print(WARNING, "ContentModerator: slur wordlist fetch failed (%s); layer stays inert",
                                   wordlist.error.c_str());
                }
                if (!exceptions_url.empty()) {
                    auto exc = moderation::fetch_text_list(exceptions_url, cache_dir, "slur_exceptions.txt");
                    if (exc.ok) {
                        content_moderator->set_slur_exceptions(exc.entries);
                        Log::log_print(INFO, "ContentModerator: slur exceptions loaded (%zu entries, %s)",
                                       exc.entries.size(),
                                       exc.from_cache ? "cached" : (exc.downloaded ? "fresh" : "?"));
                    }
                    else {
                        Log::log_print(WARNING,
                                       "ContentModerator: slur exceptions fetch failed (%s); no exceptions active",
                                       exc.error.c_str());
                    }
                }
            }).detach();
        }

        room.set_content_moderator(content_moderator.get());
    }

    // --- Firewall Manager ---
    FirewallManager firewall;
    firewall.configure(cfg.firewall_config());
    firewall.load("firewall_rules.json");
    room.set_firewall(&firewall);

    // --- Ban manager (DB-backed, akashi-compatible schema) ---
    // Placed after firewall so bans can sync kernel-level blocks on startup.
    BanManager ban_manager;
    ban_manager.set_db(&db);
    ban_manager.set_firewall(&firewall);
    ban_manager.load_from_db();
    room.set_ban_manager(&ban_manager);

    // Wire spam detector → ASN reputation reporting + auto-ban on multi-IP heuristics.
    //
    // H1 (echo), H3 (join_spam), H5 (name_pattern) are unambiguous coordinated
    // attacks with negligible false-positive risk: a threshold of 3+ distinct
    // IPs means no single human could plausibly trigger them. For these we
    // issue a 24h IP ban on every participant, which propagates to nftables
    // via the firewall integration — all active bot sessions go silent within
    // milliseconds and reinforcement traffic is dropped at the kernel level.
    //
    // H2 (burst), H6 (ghost), H7 (hwid_reuse) are single-signal heuristics with
    // plausible benign explanations (rush events, health checks, shared devices),
    // so those only report to ASN reputation — no auto-ban.
    spam_detector.set_callback([&asn_reputation, &ban_manager](const std::string& /*ipid*/, uint32_t asn,
                                                               const SpamVerdict& verdict) {
        // Report to ASN reputation (existing behavior)
        if (asn != 0) {
            std::string as_org;
            asn_reputation.report_abuse(asn, verdict.detail, as_org, verdict.heuristic + ": " + verdict.detail);
        }

        // Auto-ban the whole participant cluster for multi-IP indefensible spam.
        const bool auto_ban_heuristic =
            (verdict.heuristic == "echo" || verdict.heuristic == "join_spam" || verdict.heuristic == "name_pattern");
        if (!auto_ban_heuristic || verdict.participants.empty())
            return;

        const int64_t now_sec =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        const int64_t ban_duration = 24 * 3600; // 24h
        const std::string reason = "[auto] " + verdict.heuristic + ": " + verdict.detail;

        for (auto& [pid, pip] : verdict.participants) {
            // Skip if already banned (idempotent — add_ban overwrites, but
            // avoiding the firewall churn is cleaner).
            if (ban_manager.find_ban(pid))
                continue;

            BanEntry entry;
            entry.ipid = pid;
            entry.ip = pip;
            entry.reason = reason;
            entry.moderator = "SpamDetector";
            entry.timestamp = now_sec;
            entry.duration = ban_duration;
            ban_manager.add_ban(std::move(entry));
        }

        Log::log_print(WARNING, "SpamDetector: auto-banned %zu IPs for %s (%s)", verdict.participants.size(),
                       verdict.heuristic.c_str(), verdict.detail.c_str());
    });

    // Wire ASN reputation escalation → firewall blocking
    asn_reputation.set_status_callback([&firewall](uint32_t asn, ASNReputationEntry::Status /*old_status*/,
                                                   ASNReputationEntry::Status new_status, const std::string& reason) {
        if (new_status == ASNReputationEntry::Status::BLOCKED && firewall.is_enabled()) {
            Log::log_print(WARNING, "ASN %u blocked — firewall integration would block CIDR ranges here", asn);
            // TODO: resolve ASN → CIDR ranges and call firewall.block_range()
            // This requires additional ASN-to-prefix mapping data
        }
    });

    // --- Protocol backends ---
    AOServer ao_backend(room);
    NXServer nx_backend(room);
    nx_backend.set_motd(cfg.motd());
    nx_backend.set_session_ttl_seconds(cfg.session_ttl_seconds());

    // --- HTTP server ---
    http::Server http;
    http.Get("/", [&](const http::Request&, http::Response& res) {
        res.set_content("Hello from " + cfg.server_name() + "\n", "text/plain");
    });

    // Wire reverse proxy config into HTTP server
    {
        auto rp = cfg.reverse_proxy_config();
        if (rp.enabled)
            http.set_reverse_proxy_config(rp);
    }

    // --- REST API ---
    nx_register_endpoints();
    NXEndpoint::set_server(&nx_backend);

    RestRouter rest_router;
    rest_router.set_cors_origins(cfg.cors_origins());
    rest_router.set_auth_func(
        [&room](const std::string& token) -> ServerSession* { return room.find_session_by_token(token); });
    EndpointFactory::instance().populate(rest_router);
    rest_router.bind(http);

    // --- Metrics ---
    auto server_start_time = std::chrono::steady_clock::now();
    MetricsCollector metrics(room, rest_router, cfg, server_start_time);

    // --- SSE endpoint (AONX) ---
    http.Options("/aonx/v1/events", [](const http::Request&, http::Response& res) { res.status = 204; });

    http.SSE("/aonx/v1/events",
             [&rest_router, &room](const http::Request& req, http::Response& res) -> http::Server::SSEAcceptResult {
                 auto auth = req.get_header_value("Authorization");
                 if (auth.size() <= 7 || auth.substr(0, 7) != "Bearer ") {
                     res.status = 401;
                     res.set_content(R"({"error":"Missing or invalid Authorization header"})", "application/json");
                     return {false, {}};
                 }
                 auto token = auth.substr(7);

                 bool accepted = false;
                 rest_router.with_lock([&] {
                     auto* session = room.find_session_by_token(token);
                     if (session) {
                         session->touch();
                         accepted = true;
                     }
                 });
                 if (!accepted) {
                     res.status = 401;
                     res.set_content(R"({"error":"Invalid or expired session"})", "application/json");
                     return {false, {}};
                 }
                 return {true, token};
             });

    http.set_sse_session_touch([&rest_router, &room](const std::string& token) {
        rest_router.with_lock([&] {
            auto* session = room.find_session_by_token(token);
            if (session)
                session->touch();
        });
    });

    // --- HTTP listen ---
    if (!http.bind_to_port(cfg.bind_address(), cfg.http_port())) {
        Log::log_print(ERR, "Failed to bind HTTP on %s:%d", cfg.bind_address().c_str(), cfg.http_port());
        return 1;
    }
    Log::log_print(INFO, "HTTP listening on %s:%d", cfg.bind_address().c_str(), cfg.http_port());
    std::jthread http_thread([&](std::stop_token) { http.listen_after_bind(); });

    // --- WebSocket server ---
    auto listener = std::make_unique<PlatformServerSocket>(cfg.bind_address());
    WebSocketServer ws(std::move(listener));
    metrics.set_ws(&ws);

    if (cfg.metrics_enabled()) {
        auto& ws_conns =
            metrics::MetricsRegistry::instance().gauge("kagami_ws_connections", "Active WebSocket connections");
        metrics::MetricsRegistry::instance().add_collector(
            [&ws_conns, &ws] { ws_conns.get().set(static_cast<double>(ws.client_count())); });
    }

    ws.on_client_connected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_connected(id); });
    });
    ws.on_client_disconnected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_disconnected(id); });
    });

    {
        auto rl_cfg = cfg.rate_limit_config();
        WebSocketServer::TimeoutConfig wst;
        wst.handshake_sec = rl_cfg.value("ws_handshake_deadline_sec", 10);
        wst.idle_sec = rl_cfg.value("ws_idle_timeout_sec", 120);
        wst.partial_frame_sec = rl_cfg.value("ws_partial_frame_timeout_sec", 30);
        ws.set_timeouts(wst);
    }
    // --- Reverse proxy support ---
    auto rp_cfg = cfg.reverse_proxy_config();
    if (rp_cfg.enabled) {
        ws.set_reverse_proxy_config(rp_cfg);
        Log::log_print(INFO, "Reverse proxy enabled: %zu trusted proxies, PROXY protocol %s",
                       rp_cfg.trusted_proxies.size(), rp_cfg.proxy_protocol ? "on" : "off");
    }

    ws.set_cors_origins(cfg.cors_origins());
    ws.start(static_cast<uint16_t>(cfg.ws_port()));
    ao_backend.set_ws(&ws);
    Log::log_print(INFO, "WebSocket listening on %s:%d", cfg.bind_address().c_str(), cfg.ws_port());

    // --- Rate limiter ---
    net::RateLimiter rate_limiter;
    {
        auto rl_cfg = cfg.rate_limit_config();
        for (auto& [action, params] : rl_cfg.items()) {
            if (!params.is_object() || !params.contains("rate"))
                continue; // skip non-rule entries like ws_handshake_deadline_sec
            rate_limiter.configure(action, {params.value("rate", 10.0), params.value("burst", 20.0)});
        }
        rest_router.set_rate_limiter(&rate_limiter);
        NXEndpoint::set_rate_limiter(&rate_limiter);
        Log::log_print(INFO, "Rate limiter: %zu actions configured", rate_limiter.action_count());
    }

    // --- WS worker pool ---
    WsWorkerPool ws_pool(ws, ao_backend, rest_router, room, cfg, &rate_limiter);
    ws_pool.start();
    metrics.set_ws_pool(&ws_pool);
    metrics.start(http);

    // --- Master server advertiser ---
    MasterServerAdvertiser advertiser(cfg, room);
    advertiser.start();

    // --- REPL ---
    kagami_register_commands();
    ReplCommandRegistry repl;
    ReplCommandFactory::instance().populate(repl);

    ServerContext ctx{stop_src, db, cfg, ui, room, ao_backend, nx_backend, http, rest_router, &ws, &repl};

    if (interactive) {
        std::string line;
        while (!stop_src.stop_requested() && std::getline(std::cin, line)) {
            if (!line.empty() && !repl.dispatch(ctx, line))
                ui.print("Unknown command: " + line + " (try /help)");
            ui.show_prompt();
        }
        ui.cleanup();
    }
    else {
        while (!stop_src.stop_requested())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // --- Shutdown (order matters) ---
    stop_src.request_stop();
    Log::log_print(INFO, "Shutting down...");

    advertiser.stop();
    log_sinks.teardown();
    ws.stop();
    http.stop();

    // Sync runtime state back to settings before persisting.
    cfg.set_value("mod_password", std::any(room.mod_password));
    cfg.set_value("server_description", std::any(room.server_description));
    ServerSettings::save_to_disk(cfg_path);
    return 0;
}
