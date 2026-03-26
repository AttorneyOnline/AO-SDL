#include "ProtocolRouter.h"
#include "ReplCommand.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "net/KissnetServerSocket.h"
#include "net/WebSocketServer.h"
#include "utils/Log.h"

#include <httplib.h>

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

    if (interactive) {
        ui.init();
        // Route all log output through the terminal UI
        Log::set_sink([&ui](LogLevel level, const std::string& timestamp, const std::string& message) {
            ui.log(level, timestamp, message);
        });
    }

    Log::log_print(INFO, "Server: %s", cfg.server_name().c_str());

    // --- HTTP server (runs on its own thread pool) ---
    httplib::Server http;

    http.Get("/", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content("Hello from " + cfg.server_name() + "\n", "text/plain");
    });

    std::jthread http_thread([&](std::stop_token) {
        Log::log_print(INFO, "HTTP listening on %s:%d", cfg.bind_address().c_str(), cfg.http_port());
        http.listen(cfg.bind_address(), cfg.http_port());
    });

    // --- WebSocket server + protocol routing ---
    auto listener = std::make_unique<KissnetServerSocket>(cfg.bind_address());
    WebSocketServer ws(std::move(listener));
    ws.set_supported_subprotocols({"aonx", "ao2"});

    AOServer ao_backend;
    NXServer nx_backend;
    ProtocolRouter router(ao_backend, nx_backend);

    router.set_subprotocol_func([&ws](uint64_t id) { return ws.get_client_subprotocol(id); });

    // Configure AO2 game state
    auto& gs = ao_backend.game_state();
    gs.server_name = cfg.server_name();
    gs.server_description = cfg.server_description();
    gs.max_players = cfg.max_players();
    gs.characters = {"Phoenix", "Edgeworth", "Maya", "Godot", "Apollo"};
    gs.music = {"Trial.opus", "Objection.opus", "Pursuit.opus"};
    gs.areas = {"Lobby", "Courtroom 1", "Courtroom 2"};
    gs.reset_taken();

    // Wire send functions to WebSocket transport
    ao_backend.set_send_func([&ws](uint64_t id, const std::string& data) {
        ws.send(id, {reinterpret_cast<const uint8_t*>(data.data()), data.size()});
    });
    nx_backend.set_broadcast_func([&ws](uint64_t id, const std::string& data) {
        ws.send(id, {reinterpret_cast<const uint8_t*>(data.data()), data.size()});
    });

    ws.on_client_connected([&router](WebSocketServer::ClientId id) { router.on_client_connected(id); });
    ws.on_client_disconnected([&router](WebSocketServer::ClientId id) { router.on_client_disconnected(id); });

    ws.start(static_cast<uint16_t>(cfg.ws_port()));
    Log::log_print(INFO, "WebSocket listening on %s:%d", cfg.bind_address().c_str(), cfg.ws_port());

    // --- WS poll thread ---
    std::jthread ws_thread([&](std::stop_token st) {
        while (!st.stop_requested()) {
            auto frames = ws.poll();
            for (auto& [client_id, frame] : frames) {
                std::string data(frame.data.begin(), frame.data.end());
                Log::log_print(VERBOSE, "WS frame from %llu: %s", (unsigned long long)client_id, data.c_str());
                router.on_client_message(client_id, data);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
    Log::set_sink(nullptr);
    Log::log_print(INFO, "Shutting down...");
    ws.stop();
    http.stop();

    ServerSettings::save_to_disk(cfg_path);

    return 0;
}
