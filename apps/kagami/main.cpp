#include "CloudWatchSink.h"
#include "LokiSink.h"
#include "ReplCommand.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "event/EventManager.h"
#include "game/ClientId.h"
#include "game/GameRoom.h"
#include "metrics/MetricsRegistry.h"
#include "metrics/ProcessMetrics.h"
#include "net/EndpointFactory.h"
#include "net/PlatformServerSocket.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "net/ao/AOServer.h"
#include "net/nx/NXEndpoint.h"
#include "utils/Log.h"
#include "utils/Version.h"

#include "net/Http.h"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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

/// Resolve the path to kagami.json next to the binary.
static std::string config_path(const char* argv0) {
    auto bin_dir = std::filesystem::path(argv0).parent_path();
    return (bin_dir / "kagami.json").string();
}

int main(int /*argc*/, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // --- Load configuration ---
    std::string cfg_path = config_path(argv[0]);
    ServerSettings::load_from_disk(cfg_path);

    auto& cfg = ServerSettings::instance();

    // --- Terminal UI (interactive mode only) ---
    bool interactive = IS_INTERACTIVE();
    TerminalUI ui;

    // --- Log level for stdout (and terminal UI in interactive mode) ---
    Log::set_stdout_level(cfg.console_log_level());

    if (interactive) {
        ui.init();
        Log::set_sink([&ui](LogLevel level, const std::string& timestamp,
                            const std::string& message) { ui.log(level, timestamp, message); },
                      cfg.console_log_level());
    }

    // --- File log sink ---
    std::shared_ptr<std::ofstream> log_file;
    if (!cfg.log_file().empty()) {
        log_file = std::make_shared<std::ofstream>(cfg.log_file(), std::ios::app);
        if (log_file->is_open()) {
            Log::add_sink(
                "file",
                [lf = log_file](LogLevel level, const std::string& timestamp, const std::string& message) {
                    *lf << "[" << timestamp << "][" << log_level_name(level) << "] " << message << "\n";
                    lf->flush();
                },
                cfg.file_log_level());
        }
        else {
            Log::log_print(WARNING, "Could not open log file: %s", cfg.log_file().c_str());
            log_file.reset();
        }
    }

    // --- CloudWatch log sink ---
    std::unique_ptr<CloudWatchSink> cw_sink;
    if (!cfg.cloudwatch_region().empty() && !cfg.cloudwatch_log_group().empty()) {
        CloudWatchSink::Config cw_cfg;
        cw_cfg.region = cfg.cloudwatch_region();
        cw_cfg.log_group = cfg.cloudwatch_log_group();
        cw_cfg.log_stream = cfg.cloudwatch_log_stream();
        cw_cfg.credentials.access_key_id = cfg.cloudwatch_access_key_id();
        cw_cfg.credentials.secret_access_key = cfg.cloudwatch_secret_access_key();
        cw_cfg.flush_interval_seconds = cfg.cloudwatch_flush_interval();

        if (cw_cfg.log_stream.empty())
            cw_cfg.log_stream = cfg.server_name();

        Log::log_print(INFO, "CloudWatch: group=%s stream=%s region=%s", cw_cfg.log_group.c_str(),
                       cw_cfg.log_stream.c_str(), cw_cfg.region.c_str());

        cw_sink = std::make_unique<CloudWatchSink>(std::move(cw_cfg));
        Log::add_sink(
            "cloudwatch",
            [&cw = *cw_sink](LogLevel level, const std::string& timestamp, const std::string& message) {
                cw.push(level, timestamp, message);
            },
            cfg.cloudwatch_log_level());
        cw_sink->start();
    }

    // --- Loki log sink ---
    std::unique_ptr<LokiSink> loki_sink;
    if (!cfg.loki_url().empty()) {
        LokiSink::Config loki_cfg;
        loki_cfg.url = cfg.loki_url();
        loki_sink = std::make_unique<LokiSink>(std::move(loki_cfg));
        Log::add_sink(
            "loki",
            [&lk = *loki_sink](LogLevel level, const std::string& timestamp, const std::string& message) {
                lk.push(level, timestamp, message);
            },
            cfg.console_log_level());
        loki_sink->start();
        Log::log_print(INFO, "Loki: pushing to %s", cfg.loki_url().c_str());
    }

    Log::log_print(INFO, "Server: %s", cfg.server_name().c_str());

    // --- Shared game state ---
    GameRoom room;
    room.server_name = cfg.server_name();
    room.server_description = cfg.server_description();
    room.max_players = cfg.max_players();
    room.characters = {"Phoenix", "Edgeworth", "Maya", "Godot", "Apollo"};
    room.music = {"Trial.opus", "Objection.opus", "Pursuit.opus"};
    room.areas = {"Lobby", "Courtroom 1", "Courtroom 2"};
    room.reset_taken();
    room.build_char_id_index();
    room.build_area_index();

    // --- Protocol backends ---
    AOServer ao_backend(room); // AO2: WebSocket bidirectional
    NXServer nx_backend(room); // AONX: REST + SSE (no WebSocket)
    nx_backend.set_motd(cfg.motd());
    nx_backend.set_session_ttl_seconds(cfg.session_ttl_seconds());
    // --- HTTP server (runs on its own thread pool) ---
    http::Server http;

    http.Get("/", [&](const http::Request&, http::Response& res) {
        res.set_content("Hello from " + cfg.server_name() + "\n", "text/plain");
    });

    // --- Metrics endpoint (infrastructure, no auth) ---
    // The serialized metrics text is cached and regenerated by a dedicated
    // background thread. The /metrics endpoint just serves the cached string,
    // costing near-zero CPU on the poll thread.
    auto server_start_time = std::chrono::steady_clock::now();
    struct MetricsTextCache {
        std::mutex mutex;
        std::shared_ptr<const std::string> text = std::make_shared<const std::string>();
        void store(std::shared_ptr<const std::string> t) {
            std::lock_guard lock(mutex);
            text = std::move(t);
        }
        std::shared_ptr<const std::string> load() {
            std::lock_guard lock(mutex);
            return text;
        }
    };
    auto metrics_cache = std::make_shared<MetricsTextCache>();

    if (cfg.metrics_enabled()) {
        http.GetInline(cfg.metrics_path(), [&metrics_cache](const http::Request&, http::Response& res) {
            auto text = metrics_cache->load();
            res.set_content(*text, "text/plain; version=0.0.4; charset=utf-8");
        });
    }

    // --- REST API (AONX endpoints registered via EndpointFactory) ---
    nx_register_endpoints();
    NXEndpoint::set_server(&nx_backend);

    RestRouter rest_router;
    rest_router.set_cors_origins(cfg.cors_origins());
    rest_router.set_auth_func(
        [&room](const std::string& token) -> ServerSession* { return room.find_session_by_token(token); });
    EndpointFactory::instance().populate(rest_router);
    rest_router.bind(http);

    // --- Metrics snapshot collectors (registered after room + rest_router exist) ---
    if (cfg.metrics_enabled()) {
        auto& reg = metrics::MetricsRegistry::instance();
        auto& uptime = reg.gauge("kagami_uptime_seconds", "Server uptime in seconds");
        auto& rss = reg.gauge("kagami_process_resident_bytes", "Resident memory in bytes");
        auto& sessions_g = reg.gauge("kagami_sessions", "Active sessions", {"protocol"});
        auto& sessions_joined = reg.gauge("kagami_sessions_joined", "Joined sessions");
        auto& sessions_mods = reg.gauge("kagami_sessions_moderators", "Online moderators");
        auto& area_players = reg.gauge("kagami_area_players", "Players per area", {"area", "status"});
        auto& chars_taken = reg.gauge("kagami_characters_taken", "Characters currently taken");
        auto& area_info = reg.gauge("kagami_area_info", "Area state", {"area", "status", "locked"});
        auto& server_info = reg.gauge("kagami_server_info", "Server build and configuration",
                                      {"version", "server_name", "max_players", "session_ttl", "http_port", "ws_port",
                                       "bind_address", "cors_origin"});
        auto& max_players = reg.gauge("kagami_max_players", "Configured max player slots");
        auto& event_publishes =
            reg.gauge("kagami_event_publishes", "Cumulative events published per channel", {"channel"});
        auto& session_bytes_sent = reg.gauge("kagami_session_bytes_sent", "Bytes sent per session",
                                             {"session_id", "display_name", "protocol", "area", "character"});
        auto& session_bytes_recv = reg.gauge("kagami_session_bytes_received", "Bytes received per session",
                                             {"session_id", "display_name", "protocol", "area", "character"});
        auto& session_packets_sent = reg.gauge("kagami_session_packets_sent", "Packets sent per session",
                                               {"session_id", "display_name", "protocol", "area", "character"});
        auto& session_packets_recv = reg.gauge("kagami_session_packets_received", "Packets received per session",
                                               {"session_id", "display_name", "protocol", "area", "character"});
        auto& session_idle = reg.gauge("kagami_session_idle_seconds", "Seconds since last activity",
                                       {"session_id", "display_name", "protocol", "area", "character"});
        auto& http_work_queue = reg.gauge("kagami_http_work_queue_depth", "Pending requests in worker queue");
        auto& http_result_queue = reg.gauge("kagami_http_result_queue_depth", "Pending results awaiting poll thread");
        auto& http_active_workers =
            reg.gauge("kagami_http_active_workers", "Worker threads currently executing handlers");
        auto& http_worker_count = reg.gauge("kagami_http_worker_count", "Total worker threads");
        auto& http_worker_idle_ns =
            reg.gauge("kagami_http_worker_idle_nanoseconds_total", "Cumulative worker idle time in nanoseconds");
        auto& http_worker_busy_ns =
            reg.gauge("kagami_http_worker_busy_nanoseconds_total", "Cumulative worker busy time in nanoseconds");
        auto& cow_copy_bytes =
            reg.gauge("kagami_cow_copy_bytes_total", "Cumulative bytes copied during COW session map mutations");

        auto cors = cfg.cors_origins();
        server_info
            .labels({ao_sdl_version(), cfg.server_name(), std::to_string(cfg.max_players()),
                     std::to_string(cfg.session_ttl_seconds()), std::to_string(cfg.http_port()),
                     std::to_string(cfg.ws_port()), cfg.bind_address(), cors.empty() ? "" : cors[0]})
            .set(1);
        max_players.get().set(cfg.max_players());

        // Dedicated metrics thread — rebuilds gauges and serializes to text
        // every 1 second. The /metrics endpoint serves the cached string.
        // This keeps both the poll thread and worker pool free from metrics
        // overhead regardless of session count.
        std::jthread metrics_thread([&uptime, &rss, &sessions_g, &sessions_joined, &sessions_mods, &area_players,
                                     &area_info, &chars_taken, &event_publishes, &session_bytes_sent,
                                     &session_bytes_recv, &session_packets_sent, &session_packets_recv, &session_idle,
                                     &http_work_queue, &http_result_queue, &http_active_workers, &http_worker_count,
                                     &http_worker_idle_ns, &http_worker_busy_ns, &cow_copy_bytes, &http_server = http,
                                     &room, &server_start_time, &reg, &metrics_cache](std::stop_token st) {
            while (!st.stop_requested()) {
                auto now = std::chrono::steady_clock::now();
                uptime.get().set(std::chrono::duration<double>(now - server_start_time).count());
                rss.get().set(static_cast<double>(metrics::process_rss_bytes()));

                // HTTP worker pool metrics (lock-free — read atomics directly)
                http_work_queue.get().set(static_cast<double>(http_server.work_queue_depth()));
                http_result_queue.get().set(static_cast<double>(http_server.result_queue_depth()));
                http_active_workers.get().set(static_cast<double>(http_server.active_workers()));
                http_worker_count.get().set(static_cast<double>(http_server.worker_count()));
                http_worker_idle_ns.get().set(static_cast<double>(http_server.worker_idle_ns()));
                http_worker_busy_ns.get().set(static_cast<double>(http_server.worker_busy_ns()));

                // Aggregate session stats (lock-free atomic reads)
                sessions_g.labels({"ao2"}).set(room.stats.sessions_ao.load(std::memory_order_relaxed));
                sessions_g.labels({"aonx"}).set(room.stats.sessions_nx.load(std::memory_order_relaxed));
                sessions_joined.get().set(room.stats.joined.load(std::memory_order_relaxed));
                sessions_mods.get().set(room.stats.moderators.load(std::memory_order_relaxed));
                chars_taken.get().set(room.stats.chars_taken.load(std::memory_order_relaxed));
                cow_copy_bytes.get().set(static_cast<double>(room.cow_copy_bytes()));

                // Per-session + area detail (lock-free via COW snapshot)
                auto snap = room.sessions_snapshot();

                area_players.clear();
                area_info.clear();
                for (auto& [id, state] : room.area_states()) {
                    int count = 0;
                    for (auto& [sid, s] : *snap.sessions)
                        if (s->area == state.name)
                            ++count;
                    area_players.labels({state.name, state.status}).set(count);
                    area_info.labels({state.name, state.status, state.locked ? "true" : "false"}).set(1);
                }

                session_bytes_sent.clear();
                session_bytes_recv.clear();
                session_packets_sent.clear();
                session_packets_recv.clear();
                session_idle.clear();

                for (auto& [id, s] : *snap.sessions) {
                    std::string char_name = (s->character_id >= 0 && s->character_id < (int)room.characters.size())
                                                ? room.characters[s->character_id]
                                                : "none";
                    std::vector<std::string> labels = {std::to_string(s->session_id), s->display_name, s->protocol,
                                                       s->area, std::move(char_name)};
                    session_bytes_sent.labels(labels).set(
                        static_cast<double>(s->bytes_sent.load(std::memory_order_relaxed)));
                    session_bytes_recv.labels(labels).set(
                        static_cast<double>(s->bytes_received.load(std::memory_order_relaxed)));
                    session_packets_sent.labels(labels).set(
                        static_cast<double>(s->packets_sent.load(std::memory_order_relaxed)));
                    session_packets_recv.labels(labels).set(
                        static_cast<double>(s->packets_received.load(std::memory_order_relaxed)));
                    session_idle.labels(labels).set(static_cast<double>(
                        std::chrono::duration_cast<std::chrono::seconds>(now - s->last_activity()).count()));
                }

                for (auto& cs : EventManager::instance().snapshot_channel_stats())
                    event_publishes.labels({cs.raw_name}).set(static_cast<double>(cs.count));

                // Serialize and cache (no collectors needed — we populated gauges above)
                auto text = std::make_shared<const std::string>(reg.collect());
                metrics_cache->store(std::move(text));

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    // Preflight handler for the SSE endpoint (registered outside RestRouter).
    http.Options("/aonx/v1/events", [](const http::Request&, http::Response& res) { res.status = 204; });

    // --- SSE endpoint (AONX Phase 5) ---
    http.SSE("/aonx/v1/events",
             [&rest_router, &room](const http::Request& req, http::Response& res) -> http::Server::SSEAcceptResult {
                 // Extract bearer token from Authorization header
                 auto auth = req.get_header_value("Authorization");
                 if (auth.size() <= 7 || auth.substr(0, 7) != "Bearer ") {
                     res.status = 401;
                     res.set_content(R"({"error":"Missing or invalid Authorization header"})", "application/json");
                     return {false, {}};
                 }
                 auto token = auth.substr(7);

                 // Validate session under dispatch lock
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

    // Wire keepalive session touch — refreshes TTL for SSE-connected sessions
    http.set_sse_session_touch([&rest_router, &room](const std::string& token) {
        rest_router.with_lock([&] {
            auto* session = room.find_session_by_token(token);
            if (session)
                session->touch();
        });
    });

    if (!http.bind_to_port(cfg.bind_address(), cfg.http_port())) {
        Log::log_print(ERR, "Failed to bind HTTP on %s:%d", cfg.bind_address().c_str(), cfg.http_port());
        return 1;
    }
    Log::log_print(INFO, "HTTP listening on %s:%d", cfg.bind_address().c_str(), cfg.http_port());

    std::jthread http_thread([&](std::stop_token) { http.listen_after_bind(); });

    // --- WebSocket server + protocol routing ---
    auto listener = std::make_unique<PlatformServerSocket>(cfg.bind_address());
    WebSocketServer ws(std::move(listener));
    // Wire AO2 send function to WebSocket transport.
    // AONX clients use REST+SSE — no WebSocket involvement.
    auto ws_send = [&ws](uint64_t id, const std::string& data) {
        ws.send(id, {reinterpret_cast<const uint8_t*>(data.data()), data.size()});
    };
    ao_backend.set_send_func(ws_send);

    // WS connection gauge (registered here because ws is created after the main metrics block)
    if (cfg.metrics_enabled()) {
        auto& reg = metrics::MetricsRegistry::instance();
        auto& ws_conns = reg.gauge("kagami_ws_connections", "Active WebSocket connections");
        reg.add_collector([&ws_conns, &ws] { ws_conns.get().set(static_cast<double>(ws.client_count())); });
    }

    // All GameRoom mutations (WS and REST) share the dispatch mutex to
    // prevent concurrent access. This couples WS latency to REST handler
    // latency — acceptable at current scale, but worth revisiting if REST
    // handlers ever do slow work (e.g., asset I/O).
    ws.on_client_connected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_connected(id); });
    });
    ws.on_client_disconnected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_disconnected(id); });
    });

    ws.start(static_cast<uint16_t>(cfg.ws_port()));
    Log::log_print(INFO, "WebSocket listening on %s:%d", cfg.bind_address().c_str(), cfg.ws_port());

    // --- WS poll + session expiry thread ---
    std::jthread ws_thread([&](std::stop_token st) {
        auto last_sweep = std::chrono::steady_clock::now();
        while (!st.stop_requested()) {
            // Block up to 10ms in the kernel waiting for socket activity.
            // Replaces sleep_for — wakes instantly when data arrives.
            auto frames = ws.poll(10);
            for (auto& [client_id, frame] : frames) {
                std::string data(frame.data.begin(), frame.data.end());
                Log::log_print(VERBOSE, "WS frame from %s: %s", format_client_id(client_id).c_str(), data.c_str());
                rest_router.with_lock([&] { ao_backend.on_client_message(client_id, data); });
            }

            // Periodic session expiry sweep (~every 30s).
            // Two-phase: scan under shared lock (cheap, doesn't block requests),
            // then destroy in batches under exclusive lock. Batching prevents
            // holding the lock for seconds when thousands of sessions expire.
            auto now = std::chrono::steady_clock::now();
            if (now - last_sweep > std::chrono::seconds(30)) {
                std::vector<uint64_t> to_expire;
                rest_router.with_shared_lock(
                    [&] { to_expire = room.find_expired_sessions(cfg.session_ttl_seconds()); });
                if (!to_expire.empty()) {
                    Log::log_print(INFO, "Expiring %zu sessions", to_expire.size());
                    constexpr size_t BATCH_SIZE = 64;
                    for (size_t i = 0; i < to_expire.size(); i += BATCH_SIZE) {
                        size_t end = std::min(i + BATCH_SIZE, to_expire.size());
                        rest_router.with_lock([&] {
                            for (size_t j = i; j < end; ++j)
                                room.destroy_session(to_expire[j]);
                            room.broadcast_chars_taken();
                        });
                        // Yield between batches so HTTP requests can be served
                    }
                    metrics::MetricsRegistry::instance()
                        .counter("kagami_sessions_expired_total", "Sessions expired by TTL")
                        .get()
                        .inc(to_expire.size());
                }
                last_sweep = now;
            }
        }
    });

    // --- REPL ---
    ReplCommandRegistry repl;

    repl.add({"/stop", "Shut down the server", [&](auto&) { stop_src.request_stop(); }});

    repl.add({"/help", "List available commands", [&](auto&) {
                  for (auto& [name, cmd] : repl.commands()) {
                      ui.print("  " + name + " — " + cmd.description);
                  }
              }});

    repl.add({"/sessions", "List active sessions", [&](auto&) {
                  rest_router.with_lock([&] {
                      if (room.session_count() == 0) {
                          ui.print("  No active sessions.");
                          return;
                      }
                      room.for_each_session([&](const ServerSession& s) {
                          std::string info = "  " + format_client_id(s.client_id);
                          if (!s.display_name.empty())
                              info += " \"" + s.display_name + "\"";
                          if (s.character_id >= 0)
                              info += " char=" + std::to_string(s.character_id);
                          if (!s.area.empty())
                              info += " area=" + s.area;
                          if (!s.client_software.empty())
                              info += " (" + s.client_software + ")";
                          ui.print(info);
                      });
                      ui.print("  Total: " + std::to_string(room.session_count()));
                  });
              }});

    repl.add({"/status", "Show server status", [&](auto&) {
                  ui.print("Server:     " + cfg.server_name());
                  ui.print("HTTP:       " + cfg.bind_address() + ":" + std::to_string(cfg.http_port()));
                  ui.print("WS:         " + cfg.bind_address() + ":" + std::to_string(cfg.ws_port()));
                  ui.print("WS clients: " + std::to_string(ws.client_count()));
              }});

    if (interactive) {
        std::string line;
        while (!stop_src.stop_requested() && std::getline(std::cin, line)) {
            if (!line.empty() && !repl.dispatch(line))
                ui.print("Unknown command: " + line + " (try /help)");
            ui.show_prompt();
        }
        ui.cleanup();
    }
    else {
        while (!stop_src.stop_requested())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // --- Shutdown ---
    stop_src.request_stop();
    Log::log_print(INFO, "Shutting down...");

    // Remove sinks before stopping backends so the callbacks can't fire on
    // a stopped/destroyed object. CloudWatch's stop() does a final flush()
    // which sends buffered events via HTTP and reports errors to stderr
    // directly (not through Log), so this ordering is safe.
    Log::remove_sink("loki");
    Log::remove_sink("cloudwatch");
    Log::remove_sink("file");
    Log::set_sink(nullptr);
    if (loki_sink)
        loki_sink->stop();
    if (cw_sink)
        cw_sink->stop();

    ws.stop();
    http.stop();

    ServerSettings::save_to_disk(cfg_path);

    return 0;
}
