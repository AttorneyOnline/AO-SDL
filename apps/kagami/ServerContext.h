#pragma once

#include <stop_token>

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

/// Non-owning bag of references to shared server state.
/// Passed to REPL commands and other subsystems that need cross-cutting access.
struct ServerContext {
    std::stop_source& stop_src;
    ServerSettings& cfg;
    TerminalUI& ui;
    GameRoom& room;
    AOServer& ao_backend;
    NXServer& nx_backend;
    http::Server& http;
    RestRouter& rest_router;
    WebSocketServer* ws;
    ReplCommandRegistry* repl;
};
