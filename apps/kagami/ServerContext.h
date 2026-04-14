#pragma once

#include <stop_token>
#include <string>

class DatabaseManager;
class ServerSettings;
class TerminalUI;
class GameRoom;
class AOServer;
class NXServer;
class RestRouter;
class WebSocketServer;
class ReplCommandRegistry;

namespace http {
class Server;
}

namespace net {
class RateLimiter;
}

/// Non-owning bag of references to shared server state.
/// Passed to REPL commands and other subsystems that need cross-cutting access.
struct ServerContext {
    std::stop_source& stop_src;
    DatabaseManager& db;
    ServerSettings& cfg;
    TerminalUI& ui;
    GameRoom& room;
    AOServer& ao_backend;
    NXServer& nx_backend;
    http::Server& http;
    RestRouter& rest_router;
    WebSocketServer* ws;
    ReplCommandRegistry* repl;

    std::string cfg_path;            ///< Path to kagami.json on disk.
    std::string content_dir;         ///< Path to content config directory.
    net::RateLimiter* rate_limiter;  ///< Rate limiter for hot-reload.
};
