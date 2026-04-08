/**
 * @file WebSocketServer.h
 * @brief RFC 6455 WebSocket server for accepting and broadcasting to clients.
 *
 * Designed for server-to-client push: accepts WebSocket connections, performs
 * the server-side handshake, and broadcasts messages to all connected clients.
 * Supports subprotocol negotiation for AO2/v2 transport selection.
 */
#pragma once

#include "net/IServerSocket.h"
#include "net/ITcpSocket.h"
#include "net/WebSocketFrame.h"
#include "platform/Poll.h"

#include <atomic>
#include <deque>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

/**
 * @brief Server-side WebSocket that accepts and manages multiple client connections.
 *
 * Usage:
 *   1. Construct with an IServerSocket.
 *   2. Call start() to bind and begin listening.
 *   3. Call poll() periodically to accept new connections and read client frames.
 *   4. Call broadcast() to push messages to all connected clients.
 *
 * Thread safety: all public methods are mutex-protected and may be called
 * from any thread. Typical usage is single-threaded poll + broadcast.
 */
class WebSocketServer {
  public:
    using ClientId = uint64_t;

    struct ClientFrame {
        ClientId client_id;
        WebSocketFrame frame;
    };

    /**
     * @brief Construct a WebSocket server.
     * @param listener An IServerSocket implementation for accepting connections.
     */
    struct TimeoutConfig {
        int handshake_sec = 10;
        int idle_sec = 120;
        int partial_frame_sec = 30;
    };

    explicit WebSocketServer(std::unique_ptr<IServerSocket> listener);

    ~WebSocketServer();

    void set_timeouts(const TimeoutConfig& t) {
        timeouts_ = t;
    }

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    /**
     * @brief Bind and start listening on the given port.
     * @param port TCP port to listen on.
     */
    void start(uint16_t port);

    /**
     * @brief Stop the server and close all client connections.
     */
    void stop();

    /**
     * @brief Accept pending connections and read frames from all clients.
     *
     * Blocks up to timeout_ms waiting for socket activity via platform::Poller.
     * Returns data frames received from clients (TEXT/BINARY).
     * Control frames (PING, CLOSE) are handled internally.
     *
     * @param timeout_ms Max milliseconds to wait. 0 = non-blocking, -1 = indefinite.
     * @return Vector of client frames received since the last poll.
     */
    std::vector<ClientFrame> poll(int timeout_ms = 0);

    /**
     * @brief Send a text message to a specific client.
     * @param client_id The client to send to.
     * @param data The payload bytes.
     */
    void send(ClientId client_id, std::span<const uint8_t> data);

    /**
     * @brief Broadcast a text message to all connected clients.
     * @param data The payload bytes.
     */
    void broadcast(std::span<const uint8_t> data);

    /**
     * @brief Close a specific client connection.
     * @param client_id The client to disconnect.
     * @param code WebSocket close status code.
     * @param reason Optional close reason string.
     */
    void close_client(ClientId client_id, uint16_t code = 1000, const std::string& reason = "");

    /**
     * @brief Set the subprotocols this server supports.
     *
     * During the handshake, the server selects the first client-requested
     * subprotocol that appears in this list.
     */
    void set_supported_subprotocols(const std::vector<std::string>& protocols);

    /**
     * @brief Get the negotiated subprotocol for a client.
     * @return The selected subprotocol string, or empty if none.
     */
    std::string get_client_subprotocol(ClientId client_id) const;

    /**
     * @brief Get the number of currently connected clients.
     */
    size_t client_count() const;

    /**
     * @brief Set a callback invoked when a client completes the handshake.
     */
    void on_client_connected(std::function<void(ClientId)> callback);

    /**
     * @brief Set a callback invoked when a client disconnects.
     */
    void on_client_disconnected(std::function<void(ClientId)> callback);

    /// io_uring diagnostic stats for the WS server's poller.
    platform::Poller::IoStats io_stats() const {
        return poller_.io_stats();
    }

    /// Get the remote address of a connected client. Returns empty string if unknown.
    std::string get_client_addr(ClientId client_id) const;

    /// Queue a send for a client. The data is copied and will be sent
    /// on the next flush_sends() call from the poll thread.
    /// Thread-safe — workers call this from any thread.
    void queue_send(ClientId client_id, std::vector<uint8_t> data);

    /// Queue a deferred close for a client. Thread-safe.
    /// The actual close (and on_disconnected callback) happens on the
    /// poll thread during flush_sends(), outside any dispatch locks.
    void close_client_deferred(ClientId client_id, uint16_t code = 1000, const std::string& reason = "");

    /// Flush all queued sends and deferred closes. Called by the poll
    /// thread after workers have enqueued their results.
    void flush_sends();

  private:
    struct ClientConnection {
        ClientId id = 0;
        std::unique_ptr<ITcpSocket> socket;
        std::string remote_addr;                            ///< Peer IP address.
        std::chrono::steady_clock::time_point connected_at; ///< When TCP was accepted.
        std::chrono::steady_clock::time_point last_data_at; ///< Last time data was received.
        bool handshake_complete = false;
        std::vector<uint8_t> extra_data;
        std::vector<uint8_t> fragment_buf;
        std::vector<uint8_t> recv_buf; ///< Data delivered by io_uring completions.
        bool closed = false;           ///< Set by HangUp/Error events.
        Opcode fragment_opcode = TEXT;
        bool in_fragment = false;
        std::string selected_subprotocol;
        std::deque<std::vector<uint8_t>> send_queue; ///< Per-client send queue.
    };

    // Thread-safe send queue: workers push, poll thread drains
    struct PendingSend {
        ClientId client_id;
        std::vector<uint8_t> data;
    };
    std::mutex send_queue_mutex_;
    std::vector<PendingSend> global_send_queue_;

    struct PendingClose {
        ClientId client_id;
        uint16_t code;
        std::string reason;
    };
    std::vector<PendingClose> deferred_close_queue_; ///< Guarded by send_queue_mutex_.

    /// Drain recv_buf + socket into a single byte vector.
    std::vector<uint8_t> drain_client(ClientConnection& client);

    void accept_new_clients();
    bool perform_server_handshake(ClientConnection& client);
    std::vector<WebSocketFrame> read_client_frames(ClientConnection& client);
    void send_frame(ClientConnection& client, const WebSocketFrame& frame);
    void remove_client(ClientId id);

    std::unique_ptr<IServerSocket> listener_;
    std::map<ClientId, ClientConnection> clients_;
    std::unordered_map<int, ClientId> fd_to_client_; ///< fd → client ID for event dispatch.
    std::vector<std::string> supported_subprotocols_;
    uint64_t next_client_id_ = 1;
    bool running_ = false;
    mutable std::mutex mutex_;
    platform::Poller poller_;

    std::atomic<int> handshaked_count_{0};

    std::function<void(ClientId)> on_connected_;
    std::function<void(ClientId)> on_disconnected_;

    TimeoutConfig timeouts_;
};
