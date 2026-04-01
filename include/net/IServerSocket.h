/**
 * @file IServerSocket.h
 * @brief Abstract TCP listener socket interface.
 *
 * Represents a bound listening socket that accepts incoming connections.
 * Each accepted connection is returned as an ITcpSocket.
 */
#pragma once

#include "net/ITcpSocket.h"

#include <cstdint>
#include <memory>

/**
 * @brief Abstract interface for a TCP server/listener socket.
 *
 * Implementations bind to a port, listen for incoming connections,
 * and return connected ITcpSocket instances via accept().
 */
class IServerSocket {
  public:
    virtual ~IServerSocket() = default;

    /**
     * @brief Bind to the configured address/port and start listening.
     * @throws std::runtime_error on bind or listen failure.
     */
    virtual void bind_and_listen(uint16_t port, int backlog = 128) = 0;

    /**
     * @brief Accept an incoming connection.
     *
     * In blocking mode, waits until a connection arrives.
     * In non-blocking mode, returns nullptr if no connection is pending.
     *
     * @return A connected ITcpSocket, or nullptr if non-blocking and no connection ready.
     */
    virtual std::unique_ptr<ITcpSocket> accept() = 0;

    /**
     * @brief Toggle non-blocking mode on the listener socket.
     */
    virtual void set_non_blocking(bool non_blocking) = 0;

    /**
     * @brief Close the listener socket.
     */
    virtual void close() = 0;

    /**
     * @brief Raw file descriptor for use with platform::Poller.
     * @return The OS listener fd, or -1 if not available.
     */
    virtual int fd() const {
        return -1;
    }
};
