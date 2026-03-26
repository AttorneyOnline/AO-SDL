/**
 * @file NetworkThread.h
 * @brief Spawns a joinable network thread that polls a ProtocolHandler each tick.
 *
 * The constructor launches a background thread that blocks on a
 * ServerConnectEvent before establishing the connection, then enters a
 * poll loop that reads incoming messages and flushes outgoing ones.
 */
#pragma once

#include "ProtocolHandler.h"

#include <thread>

/**
 * @brief Manages a dedicated network I/O thread for a ProtocolHandler.
 *
 * On construction, a std::jthread is spawned running net_loop(). The thread
 * first waits for a ServerConnectEvent to determine which server to connect
 * to, then opens a WebSocket connection and enters the main poll loop. Each
 * iteration drains ProtocolHandler::flush_outgoing() and delivers received
 * messages via ProtocolHandler::on_message().
 *
 * Call stop() to signal the thread to exit and join it, or let the destructor
 * handle it automatically. The ProtocolHandler must outlive the NetworkThread.
 */
class NetworkThread {
  public:
    /**
     * @brief Construct a NetworkThread and immediately spawn the network thread.
     *
     * The thread is started in the constructor. It blocks internally on a
     * ServerConnectEvent before attempting to connect.
     *
     * @param handler Reference to the ProtocolHandler whose callbacks will be
     *                invoked from the network thread. The handler must outlive
     *                the NetworkThread.
     */
    explicit NetworkThread(ProtocolHandler& handler);

    /**
     * @brief Signal the network loop to stop and join the thread.
     */
    void stop();

  private:
    void net_loop(std::stop_token st);

    ProtocolHandler& handler;
    std::jthread net_thread;
};
