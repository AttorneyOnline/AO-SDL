#pragma once

#include "net/ITcpSocket.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

// Test double for ITcpSocket.
//
// Feed bytes to simulate server-sent data:
//   mock->feed({0x81, 0x02, 'h', 'i'});
//
// Inspect what the WebSocket sent:
//   mock->sent()  → all bytes written via send() in order
//
// React to outgoing data (e.g., to dynamically inject a handshake response):
//   mock->on_send = [mock](const std::vector<uint8_t>& cumulative) { mock->feed(response); };
class MockTcpSocket : public ITcpSocket {
  public:
    // Queue a chunk; returned by the next recv() call.
    void feed(std::vector<uint8_t> chunk) {
        recv_queue.push_back(std::move(chunk));
    }

    // All bytes passed to send(), concatenated in order.
    const std::vector<uint8_t>& sent() const { return sent_bytes; }

    // Optional callback fired on every send() call.
    // Receives the full cumulative sent buffer so far.
    // Useful for computing dynamic handshake responses.
    std::function<void(const std::vector<uint8_t>&)> on_send;

    // ITcpSocket interface ------------------------------------------------
    void connect() override {}
    void set_non_blocking(bool) override {}

    void send(const uint8_t* data, size_t size) override {
        sent_bytes.insert(sent_bytes.end(), data, data + size);
        if (on_send) {
            on_send(sent_bytes);
        }
    }

    std::vector<uint8_t> recv() override {
        if (recv_queue.empty()) {
            return {};
        }
        auto chunk = std::move(recv_queue.front());
        recv_queue.pop_front();
        return chunk;
    }

    bool bytes_available() override {
        return !recv_queue.empty();
    }

  private:
    std::deque<std::vector<uint8_t>> recv_queue;
    std::vector<uint8_t> sent_bytes;
};
