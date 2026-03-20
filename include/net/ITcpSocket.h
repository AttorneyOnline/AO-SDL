/**
 * @file ITcpSocket.h
 * @brief Abstract TCP socket interface and WebSocketException.
 *
 * Provides a minimal TCP socket abstraction used by WebSocket.
 * Abstracting the socket allows tests to inject a mock implementation.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Exception thrown by socket operations on fatal transport errors.
 */
class WebSocketException : public std::runtime_error {
  public:
    /**
     * @brief Construct a WebSocketException with a descriptive message.
     * @param message Human-readable description of the error.
     */
    explicit WebSocketException(const std::string& message) : std::runtime_error(message) {
    }
};

/**
 * @brief Minimal TCP socket interface used by WebSocket.
 *
 * Abstracting this out allows tests to inject a mock instead of a real socket.
 */
class ITcpSocket {
  public:
    virtual ~ITcpSocket() = default;

    /**
     * @brief Establish the TCP connection to the remote host.
     * @throws WebSocketException on connection failure.
     */
    virtual void connect() = 0;

    /**
     * @brief Toggle non-blocking mode on the underlying socket.
     * @param non_blocking True to enable non-blocking I/O, false for blocking.
     */
    virtual void set_non_blocking(bool non_blocking) = 0;

    /**
     * @brief Send raw bytes over the connection.
     * @param data Pointer to the byte buffer to send.
     * @param size Number of bytes to send.
     * @throws WebSocketException on a hard socket error.
     */
    virtual void send(const uint8_t* data, size_t size) = 0;

    /**
     * @brief Read available bytes from the connection.
     * @return A vector of received bytes, or an empty vector if nothing is available.
     * @throws WebSocketException on a hard socket error.
     */
    virtual std::vector<uint8_t> recv() = 0;

    /**
     * @brief Check whether data can be read without blocking.
     * @return True if one or more bytes are available for reading.
     */
    virtual bool bytes_available() = 0;
};
