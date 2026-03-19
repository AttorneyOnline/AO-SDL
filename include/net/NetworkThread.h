/**
 * @file NetworkThread.h
 * @brief Spawns a detached network thread that polls a ProtocolHandler each tick.
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
 * On construction, a std::thread is spawned (and detached) running net_loop().
 * The thread first waits for a ServerConnectEvent to determine which server
 * to connect to, then opens a WebSocket connection and enters the main poll
 * loop. Each iteration drains ProtocolHandler::flush_outgoing() and delivers
 * received messages via ProtocolHandler::on_message().
 *
 * @note The spawned thread is detached -- there is no join or explicit
 *       shutdown mechanism exposed by this class.
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

  private:
    /**
     * @brief Main loop executed on the network thread.
     *
     * Waits for a ServerConnectEvent, connects the WebSocket, then polls for
     * incoming/outgoing messages until the connection is lost.
     */
    void net_loop();

    ProtocolHandler& handler; /**< Protocol handler driven by this thread. */
    std::thread net_thread;   /**< The background network thread. */
};
