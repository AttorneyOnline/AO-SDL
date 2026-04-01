/**
 * @file WSClientThread.h
 * @brief Dedicated thread for a WebSocket client connection.
 *
 * Launches a background thread that waits for a ServerConnectEvent,
 * establishes a WebSocket (ws:// or wss://) connection, then enters
 * a poll-driven loop that reads incoming messages and flushes outgoing ones.
 */
#pragma once

#include "ProtocolHandler.h"

#include <cstdint>
#include <string>
#include <thread>

/// Parsed ws:// or wss:// URL components.
struct ParsedWsUrl {
    std::string host;
    uint16_t port = 0;
    bool ssl = false;
};

/// Parse a WebSocket URL into host, port, and TLS flag.
/// Handles ws://, wss://, embedded host:port, IPv6, and trailing paths.
/// If port_in is non-zero it overrides the port in the URL.
ParsedWsUrl parse_ws_url(const std::string& host_in, uint16_t port_in);

/**
 * @brief Manages a dedicated WebSocket client I/O thread.
 *
 * On construction, a std::jthread is spawned running ws_loop(). The thread
 * waits for a ServerConnectEvent, connects (upgrading to TLS for wss://),
 * then uses a platform::Poller to efficiently wait for incoming data.
 *
 * The ProtocolHandler must outlive the WSClientThread.
 */
class WSClientThread {
  public:
    explicit WSClientThread(ProtocolHandler& handler);

    void stop();

  private:
    void ws_loop(std::stop_token st);

    ProtocolHandler& handler;
    std::jthread thread_;
};
