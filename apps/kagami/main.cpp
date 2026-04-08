#include "LogSinkSetup.h"
#include "MetricsCollector.h"
#include "ReplCommand.h"
#include "ReplCommandFactory.h"
#include "ServerContext.h"
#include "ServerSettings.h"
#include "TerminalUI.h"
#include "WsWorkerPool.h"

#include "game/GameRoom.h"
#include "metrics/MetricsRegistry.h"
#include "net/EndpointFactory.h"
#include "net/Http.h"
#include "net/PlatformServerSocket.h"
#include "net/RateLimiter.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "net/ao/AOServer.h"
#include "net/nx/NXEndpoint.h"
#include "utils/Log.h"

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
    log_sinks.init(cfg, ui, interactive);

    Log::log_print(INFO, "Server: %s", cfg.server_name().c_str());

    // --- Game state ---
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
    AOServer ao_backend(room);
    NXServer nx_backend(room);
    nx_backend.set_motd(cfg.motd());
    nx_backend.set_session_ttl_seconds(cfg.session_ttl_seconds());

    // --- HTTP server ---
    http::Server http;
    http.Get("/", [&](const http::Request&, http::Response& res) {
        res.set_content("Hello from " + cfg.server_name() + "\n", "text/plain");
    });

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

    ws.start(static_cast<uint16_t>(cfg.ws_port()));
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

    // --- REPL ---
    kagami_register_commands();
    ReplCommandRegistry repl;
    ReplCommandFactory::instance().populate(repl);

    ServerContext ctx{stop_src, cfg, ui, room, ao_backend, nx_backend, http, rest_router, &ws, &repl};

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

    log_sinks.teardown();
    ws.stop();
    http.stop();

    ServerSettings::save_to_disk(cfg_path);
    return 0;
}
