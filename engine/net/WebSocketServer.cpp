#include "net/WebSocketServer.h"

#include "metrics/MetricsRegistry.h"
#include "net/WebSocketCommon.h"
#include "utils/Log.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <algorithm>
#include <cstring>
#include <format>
#include <sstream>

// -- WS metrics (file-scope — self-register at program startup) ---------------

static auto& ws_frames_in_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_frames_in_total", "WebSocket frames received");
static auto& ws_frames_out_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_frames_out_total", "WebSocket frames sent");
static auto& ws_bytes_in_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_bytes_in_total", "WebSocket bytes received");
static auto& ws_bytes_out_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_bytes_out_total", "WebSocket bytes sent");
static auto& ws_handshake_failures_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_handshake_failures_total", "WebSocket handshake failures");
static auto& ws_send_failures_ =
    metrics::MetricsRegistry::instance().counter("kagami_ws_send_failures_total", "WebSocket send failures (dropped clients)");
static auto& ws_connections_ =
    metrics::MetricsRegistry::instance().gauge("kagami_ws_connections", "Active WebSocket connections");

WebSocketServer::WebSocketServer(std::unique_ptr<IServerSocket> listener) : listener_(std::move(listener)) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start(uint16_t port) {
    std::lock_guard lock(mutex_);
    listener_->bind_and_listen(port);
    listener_->set_non_blocking(true);
    running_ = true;

    // Register the listener with the poller for accept readiness.
    int lfd = listener_->fd();
    if (lfd >= 0)
        poller_.add(lfd, platform::Poller::Readable);
}

void WebSocketServer::stop() {
    std::vector<ClientId> all_ids;
    {
        std::lock_guard lock(mutex_);
        if (!running_)
            return;
        running_ = false;

        for (auto& [id, client] : clients_) {
            if (client.handshake_complete)
                all_ids.push_back(id);
            try {
                WebSocketFrame close_frame;
                close_frame.fin = true;
                close_frame.opcode = CLOSE;
                close_frame.mask = false;
                uint16_t net_code = htons(1001); // Going Away
                close_frame.data.resize(2);
                std::memcpy(close_frame.data.data(), &net_code, 2);
                close_frame.len = close_frame.data.size();
                close_frame.len_code = (uint8_t)close_frame.len;
                send_frame(client, close_frame);
            }
            catch (...) {
            }
        }
        clients_.clear();
        listener_->close();
    }

    for (auto id : all_ids) {
        if (on_disconnected_)
            on_disconnected_(id);
    }
}

std::vector<WebSocketServer::ClientFrame> WebSocketServer::poll(int timeout_ms) {
    // Poll for events — on io_uring this delivers completed recv data,
    // on epoll/kqueue it delivers readiness notifications.
    constexpr int MAX_POLL_EVENTS = 256;
    platform::Poller::Event events[MAX_POLL_EVENTS];
    int nevents = poller_.poll(events, MAX_POLL_EVENTS, timeout_ms);

    std::vector<ClientFrame> result;
    std::vector<ClientId> newly_connected;
    std::vector<ClientId> newly_disconnected;

    {
        std::lock_guard lock(mutex_);
        if (!running_)
            return {};

        // Dispatch completion/readiness events to client recv buffers
        for (int i = 0; i < nevents; ++i) {
            auto& ev = events[i];

            // Listener socket — accept new clients
            if (ev.fd == listener_->fd()) {
                accept_new_clients();
                continue;
            }

            // Find the client for this fd
            auto fd_it = fd_to_client_.find(ev.fd);
            if (fd_it == fd_to_client_.end())
                continue;
            auto cl_it = clients_.find(fd_it->second);
            if (cl_it == clients_.end())
                continue;
            auto& client = cl_it->second;

            // Completion data from io_uring
            if (ev.data_len > 0) {
                client.recv_buf.insert(client.recv_buf.end(),
                                       static_cast<const uint8_t*>(ev.data),
                                       static_cast<const uint8_t*>(ev.data) + ev.data_len);
                poller_.recycle_buffer(ev.buffer_id);
            }

            if (ev.flags & (platform::Poller::HangUp | platform::Poller::Error))
                client.closed = true;
        }

        // Process all clients that have data or need handshake/read
        std::vector<ClientId> dead_clients;

        for (auto& [id, client] : clients_) {
            if (client.closed) {
                dead_clients.push_back(id);
                continue;
            }

            if (!client.handshake_complete) {
                try {
                    if (!perform_server_handshake(client))
                        continue;
                    newly_connected.push_back(id);
                }
                catch (const std::exception& e) {
                    Log::log_print(WARNING, "WS handshake failed for client %llu: %s",
                                   (unsigned long long)id, e.what());
                    ws_handshake_failures_.get().inc();
                    dead_clients.push_back(id);
                    continue;
                }
                catch (...) {
                    ws_handshake_failures_.get().inc();
                    dead_clients.push_back(id);
                    continue;
                }
            }

            try {
                auto frames = read_client_frames(client);
                for (auto& frame : frames) {
                    ws_frames_in_.get().inc();
                    ws_bytes_in_.get().inc(frame.data.size());
                    result.push_back({id, std::move(frame)});
                }
            }
            catch (...) {
                dead_clients.push_back(id);
            }
        }

        for (auto id : dead_clients) {
            newly_disconnected.push_back(id);
            remove_client(id);
        }
    }

    for (auto id : newly_connected) {
        if (on_connected_)
            on_connected_(id);
    }
    for (auto id : newly_disconnected) {
        if (on_disconnected_)
            on_disconnected_(id);
    }

    return result;
}

void WebSocketServer::send(ClientId client_id, std::span<const uint8_t> data) {
    ws_frames_out_.get().inc();
    ws_bytes_out_.get().inc(data.size());

    bool dead = false;
    {
        std::lock_guard lock(mutex_);
        auto it = clients_.find(client_id);
        if (it == clients_.end() || !it->second.handshake_complete)
            return;

        WebSocketFrame frame;
        frame.fin = true;
        frame.rsv = 0;
        frame.opcode = TEXT;
        frame.mask = false; // Server MUST NOT mask
        frame.len = data.size();

        if (frame.len <= 125)
            frame.len_code = (uint8_t)(frame.len & 0xFF);
        else if (frame.len <= UINT16_MAX)
            frame.len_code = 126;
        else
            frame.len_code = 127;

        frame.data.assign(data.begin(), data.end());

        try {
            send_frame(it->second, frame);
        }
        catch (...) {
            remove_client(client_id);
            dead = true;
        }
    }

    if (dead && on_disconnected_)
        on_disconnected_(client_id);
}

void WebSocketServer::broadcast(std::span<const uint8_t> data) {
    // Threading assumption: poll(), send(), broadcast(), and close_client()
    // are called from a single thread (the WS poll thread). If multiple
    // threads call these concurrently, a client could be removed by one
    // thread while another is about to fire its disconnect callback,
    // resulting in a double callback. The mutex protects the data structure
    // but not the callback-firing window. This is safe under the current
    // single-writer model.
    std::vector<ClientId> dead_clients;
    {
        std::lock_guard lock(mutex_);

        WebSocketFrame frame;
        frame.fin = true;
        frame.rsv = 0;
        frame.opcode = TEXT;
        frame.mask = false;
        frame.len = data.size();

        if (frame.len <= 125)
            frame.len_code = (uint8_t)(frame.len & 0xFF);
        else if (frame.len <= UINT16_MAX)
            frame.len_code = 126;
        else
            frame.len_code = 127;

        frame.data.assign(data.begin(), data.end());
        auto wire_bytes = frame.serialize();

        int send_count = 0;
        for (auto& [id, client] : clients_) {
            if (!client.handshake_complete)
                continue;
            try {
                client.socket->send(wire_bytes.data(), wire_bytes.size());
                ++send_count;
            }
            catch (...) {
                ws_send_failures_.get().inc();
                dead_clients.push_back(id);
            }
        }
        ws_frames_out_.get().inc(send_count);
        ws_bytes_out_.get().inc(data.size() * send_count);

        for (auto id : dead_clients)
            remove_client(id);
    }

    for (auto id : dead_clients) {
        if (on_disconnected_)
            on_disconnected_(id);
    }
}

void WebSocketServer::close_client(ClientId client_id, uint16_t code, const std::string& reason) {
    {
        std::lock_guard lock(mutex_);
        auto it = clients_.find(client_id);
        if (it == clients_.end())
            return;

        try {
            WebSocketFrame frame;
            frame.fin = true;
            frame.opcode = CLOSE;
            frame.mask = false;

            uint16_t net_code = htons(code);
            frame.data.resize(2);
            std::memcpy(frame.data.data(), &net_code, 2);
            if (!reason.empty()) {
                size_t reason_len = std::min(reason.size(), (size_t)123);
                frame.data.insert(frame.data.end(), reason.begin(), reason.begin() + reason_len);
            }
            frame.len = frame.data.size();
            frame.len_code = (uint8_t)frame.len;

            send_frame(it->second, frame);
        }
        catch (...) {
        }

        remove_client(client_id);
    }

    if (on_disconnected_)
        on_disconnected_(client_id);
}

void WebSocketServer::set_supported_subprotocols(const std::vector<std::string>& protocols) {
    std::lock_guard lock(mutex_);
    supported_subprotocols_ = protocols;
}

std::string WebSocketServer::get_client_subprotocol(ClientId client_id) const {
    std::lock_guard lock(mutex_);
    auto it = clients_.find(client_id);
    if (it == clients_.end())
        return "";
    return it->second.selected_subprotocol;
}

size_t WebSocketServer::client_count() const {
    std::lock_guard lock(mutex_);
    return clients_.size();
}

void WebSocketServer::on_client_connected(std::function<void(ClientId)> callback) {
    std::lock_guard lock(mutex_);
    on_connected_ = std::move(callback);
}

void WebSocketServer::on_client_disconnected(std::function<void(ClientId)> callback) {
    std::lock_guard lock(mutex_);
    on_disconnected_ = std::move(callback);
}

// --- Private ---

std::vector<uint8_t> WebSocketServer::drain_client(ClientConnection& client) {
    std::vector<uint8_t> bytes;

    // 1. Consume any data delivered by io_uring completions
    if (!client.recv_buf.empty()) {
        bytes.swap(client.recv_buf);
    }

    // 2. Also drain the socket (readiness mode, or io_uring fallback data)
    try {
        do {
            auto chunk = client.socket->recv();
            if (chunk.empty())
                break;
            bytes.insert(bytes.end(), chunk.begin(), chunk.end());
        } while (client.socket->bytes_available());
    }
    catch (...) {
        if (bytes.empty())
            throw;
        // If we already have data, process it before propagating the error
    }

    return bytes;
}

void WebSocketServer::accept_new_clients() {
    // Accept all pending connections (non-blocking)
    while (auto client_socket = listener_->accept()) {
        client_socket->set_non_blocking(true);

        int cfd = client_socket->fd();
        if (cfd >= 0)
            poller_.add(cfd, platform::Poller::Readable);

        ClientConnection conn;
        conn.id = next_client_id_++;
        if (cfd >= 0)
            fd_to_client_[cfd] = conn.id;
        conn.socket = std::move(client_socket);
        clients_.emplace(conn.id, std::move(conn));
    }
}

bool WebSocketServer::perform_server_handshake(ClientConnection& client) {
    auto bytes = drain_client(client);

    if (bytes.empty() && client.extra_data.empty())
        return false;

    // Parse HTTP request lines
    auto lines = ws::get_lines(bytes, client.extra_data);

    // Check if we have a complete request (empty line terminates headers)
    if (std::find(lines.begin(), lines.end(), "") == lines.end())
        return false; // Incomplete, wait for more data

    // Parse request line
    if (lines.empty())
        throw WebSocketException("Empty handshake request");

    auto req_line = ws::parse_request_line(lines[0]);
    if (req_line.method != "GET")
        throw WebSocketException(std::format("Expected GET, got {}", req_line.method));

    // Parse headers
    HTTPHeaders headers;
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].empty())
            break;
        auto kv = ws::parse_http_header(lines[i]);
        headers.emplace(kv);
    }

    // Validate WebSocket upgrade request.
    // RFC 6455 §4.2.1: the Connection header must *contain* the "Upgrade"
    // token (case-insensitive), not necessarily be exactly "Upgrade".
    // Proxies like ALB send "keep-alive, Upgrade".
    if (!ws::case_insensitive_equal(headers["Upgrade"], "websocket"))
        throw WebSocketException("Missing or invalid Upgrade header");
    {
        bool has_upgrade_token = false;
        std::istringstream conn_stream(headers["Connection"]);
        std::string token;
        while (std::getline(conn_stream, token, ',')) {
            token = ws::trim(token);
            if (ws::case_insensitive_equal(token, "Upgrade")) {
                has_upgrade_token = true;
                break;
            }
        }
        if (!has_upgrade_token)
            throw WebSocketException("Missing or invalid Connection header");
    }

    auto key_it = headers.find("Sec-WebSocket-Key");
    if (key_it == headers.end() || key_it->second.empty())
        throw WebSocketException("Missing Sec-WebSocket-Key");

    std::string accept_value = ws::compute_accept_key(key_it->second);

    // Subprotocol negotiation
    std::string selected_protocol;
    auto proto_it = headers.find("Sec-WebSocket-Protocol");
    if (proto_it != headers.end() && !supported_subprotocols_.empty()) {
        // Parse comma-separated list of client protocols
        std::istringstream ss(proto_it->second);
        std::string proto;
        while (std::getline(ss, proto, ',')) {
            proto = ws::trim(proto);
            if (std::find(supported_subprotocols_.begin(), supported_subprotocols_.end(), proto) !=
                supported_subprotocols_.end()) {
                selected_protocol = proto;
                break;
            }
        }
    }
    client.selected_subprotocol = selected_protocol;

    // Build 101 response
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n";
    response += std::format("Sec-WebSocket-Accept: {}\r\n", accept_value);
    if (!selected_protocol.empty())
        response += std::format("Sec-WebSocket-Protocol: {}\r\n", selected_protocol);
    response += "\r\n";

    client.socket->send(reinterpret_cast<const uint8_t*>(response.data()), response.size());
    client.handshake_complete = true;
    ws_connections_.get().set(static_cast<double>(
        std::count_if(clients_.begin(), clients_.end(),
                      [](const auto& p) { return p.second.handshake_complete; })));
    return true;
}

std::vector<WebSocketFrame> WebSocketServer::read_client_frames(ClientConnection& client) {
    std::vector<WebSocketFrame> messages;

    // Drain completion buffer + socket into working buffer
    auto bytes = drain_client(client);

    // Prepend any leftover partial frame data from last call
    if (!client.extra_data.empty()) {
        client.extra_data.insert(client.extra_data.end(), bytes.begin(), bytes.end());
        bytes.swap(client.extra_data);
        client.extra_data.clear();
    }

    if (bytes.empty())
        return messages;

    // Parse frames
    size_t offset = 0;
    while (offset < bytes.size()) {
        // Need at least 2 bytes for the header
        if (bytes.size() - offset < 2) {
            client.extra_data.assign(bytes.begin() + offset, bytes.end());
            return messages;
        }

        uint8_t byte0 = bytes[offset];
        uint8_t byte1 = bytes[offset + 1];

        bool masked = (byte1 & 0x80) != 0;
        uint64_t pl_len = byte1 & 0x7F;
        size_t header_size = 2;

        if (pl_len == 126) {
            header_size += 2;
            if (bytes.size() - offset < header_size) {
                client.extra_data.assign(bytes.begin() + offset, bytes.end());
                return messages;
            }
            uint16_t net_len;
            std::memcpy(&net_len, &bytes[offset + 2], 2);
            pl_len = ntohs(net_len);
        }
        else if (pl_len == 127) {
            header_size += 8;
            if (bytes.size() - offset < header_size) {
                client.extra_data.assign(bytes.begin() + offset, bytes.end());
                return messages;
            }
            uint64_t net_len;
            std::memcpy(&net_len, &bytes[offset + 2], 8);
            pl_len = net_to_host_64(net_len);
        }

        if (masked)
            header_size += 4;

        uint64_t total_needed = header_size + pl_len;
        if (bytes.size() - offset < total_needed) {
            client.extra_data.assign(bytes.begin() + offset, bytes.end());
            return messages;
        }

        // Read mask key if present
        uint32_t mask_key = 0;
        if (masked) {
            size_t mask_offset = header_size - 4;
            uint32_t mask_net;
            std::memcpy(&mask_net, &bytes[offset + mask_offset], 4);
            mask_key = ntohl(mask_net);
        }

        // Extract and unmask payload
        size_t data_start = offset + header_size;
        std::vector<uint8_t> payload(bytes.begin() + data_start, bytes.begin() + data_start + pl_len);
        if (masked && !payload.empty())
            apply_mask(payload.data(), payload.size(), mask_key);

        WebSocketFrame frame;
        frame.complete = true;
        frame.fin = (byte0 & 0x80) != 0;
        frame.rsv = (byte0 & 0x70) >> 4;
        frame.opcode = static_cast<Opcode>(byte0 & 0x0F);
        frame.mask = masked;
        frame.len_code = byte1 & 0x7F;
        frame.len = pl_len;
        frame.mask_key = mask_key;
        frame.data = std::move(payload);

        offset += total_needed;

        // Handle control frames
        if (frame.opcode == CLOSE) {
            // Echo close back (unmasked)
            WebSocketFrame close_resp;
            close_resp.fin = true;
            close_resp.opcode = CLOSE;
            close_resp.mask = false;
            if (frame.data.size() >= 2) {
                close_resp.data.assign(frame.data.begin(), frame.data.begin() + 2);
            }
            else {
                uint16_t code = htons(1000);
                close_resp.data.resize(2);
                std::memcpy(close_resp.data.data(), &code, 2);
            }
            close_resp.len = close_resp.data.size();
            close_resp.len_code = (uint8_t)close_resp.len;
            try {
                send_frame(client, close_resp);
            }
            catch (...) {
            }
            throw WebSocketException("Client closed connection");
        }
        else if (frame.opcode == PING) {
            // Respond with PONG (unmasked, echo payload)
            WebSocketFrame pong;
            pong.fin = true;
            pong.opcode = PONG;
            pong.mask = false;
            pong.data = frame.data;
            pong.len = pong.data.size();
            pong.len_code = pong.len <= 125 ? (uint8_t)pong.len : 126;
            send_frame(client, pong);
        }
        else if (frame.opcode == PONG) {
            // Ignore
        }
        else if (frame.opcode == CONTINUATION) {
            if (!client.in_fragment)
                throw WebSocketException("Continuation frame without initial fragment");
            client.fragment_buf.insert(client.fragment_buf.end(), frame.data.begin(), frame.data.end());
            if (frame.fin) {
                WebSocketFrame assembled;
                assembled.complete = true;
                assembled.fin = true;
                assembled.opcode = client.fragment_opcode;
                assembled.mask = false;
                assembled.len = client.fragment_buf.size();
                assembled.data = std::move(client.fragment_buf);
                client.fragment_buf.clear();
                client.in_fragment = false;
                messages.push_back(std::move(assembled));
            }
        }
        else {
            // TEXT or BINARY
            if (frame.fin) {
                if (client.in_fragment)
                    throw WebSocketException("New data frame while accumulating fragments");
                messages.push_back(std::move(frame));
            }
            else {
                if (client.in_fragment)
                    throw WebSocketException("New fragment while accumulating fragments");
                client.in_fragment = true;
                client.fragment_opcode = frame.opcode;
                client.fragment_buf = std::move(frame.data);
            }
        }
    }

    return messages;
}

void WebSocketServer::send_frame(ClientConnection& client, const WebSocketFrame& frame) {
    auto wire = frame.serialize();
    client.socket->send(wire.data(), wire.size());
}

void WebSocketServer::remove_client(ClientId id) {
    // Removes the client from the map only. Must be called under mutex_.
    // The caller is responsible for firing on_disconnected_ AFTER releasing
    // the lock to avoid deadlock (callbacks may call send/broadcast).
    auto it = clients_.find(id);
    if (it == clients_.end())
        return;

    // Remove from poller and fd map before destroying the socket
    int cfd = it->second.socket->fd();
    if (cfd >= 0) {
        poller_.remove(cfd);
        fd_to_client_.erase(cfd);
    }

    clients_.erase(it);
    ws_connections_.get().set(static_cast<double>(
        std::count_if(clients_.begin(), clients_.end(),
                      [](const auto& p) { return p.second.handshake_complete; })));
}
