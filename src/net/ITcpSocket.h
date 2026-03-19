#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class WebSocketException : public std::runtime_error {
  public:
    explicit WebSocketException(const std::string& message) : std::runtime_error(message) {}
};

// Minimal TCP socket interface used by WebSocket.
// Abstracting this out allows tests to inject a mock instead of a real socket.
class ITcpSocket {
  public:
    virtual ~ITcpSocket() = default;

    virtual void connect() = 0;
    virtual void set_non_blocking(bool non_blocking) = 0;

    // Send raw bytes. No-throw; implementations may throw WebSocketException.
    virtual void send(const uint8_t* data, size_t size) = 0;

    // Read available bytes. Returns an empty vector if nothing is available.
    // Throws WebSocketException on a hard socket error.
    virtual std::vector<uint8_t> recv() = 0;

    // True if more data can be read without blocking.
    virtual bool bytes_available() = 0;
};
