/**
 * @file ProtocolHandler.h
 * @brief Abstract interface for protocol plugins (e.g. AOClient).
 *
 * ProtocolHandler defines the callbacks that NetworkThread invokes as
 * transport-level events occur, plus a method to drain outgoing messages.
 */
#pragma once

#include <string>
#include <vector>

/**
 * @brief Abstract protocol handler that bridges game logic and the network transport.
 *
 * Concrete implementations (e.g. AOClient) translate between raw wire messages
 * and higher-level game events. NetworkThread owns a reference to this interface
 * and calls its methods from the network thread.
 */
class ProtocolHandler {
  public:
    virtual ~ProtocolHandler() = default;

    /**
     * @brief Called once after the transport connection is established.
     *
     * Implementations typically send an initial handshake or identification
     * message here.
     */
    virtual void on_connect() = 0;

    /**
     * @brief Called for each complete message received from the server.
     * @param msg The raw message string received from the server.
     */
    virtual void on_message(const std::string& msg) = 0;

    /**
     * @brief Called when the transport connection is lost.
     *
     * Implementations should clean up any session state and may post
     * a disconnect event to the game layer.
     */
    virtual void on_disconnect() = 0;

    /**
     * @brief Returns any outgoing messages queued since the last call, then clears the queue.
     *
     * NetworkThread drains this every tick and writes each message to the socket.
     *
     * @return A vector of serialized messages ready to be sent over the wire.
     */
    virtual std::vector<std::string> flush_outgoing() = 0;
};
