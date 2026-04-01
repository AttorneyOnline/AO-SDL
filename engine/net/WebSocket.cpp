#include "net/WebSocket.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "net/PlatformTcpSocket.h"
#include "utils/Base64.h"
#include "utils/Version.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>

WebSocket::WebSocket(const std::string& host, uint16_t port)
    : WebSocket(host, port, std::make_unique<PlatformTcpSocket>(host, port)) {
}

WebSocket::WebSocket(const std::string& host, uint16_t port, std::unique_ptr<ITcpSocket> socket)
    : socket(std::move(socket)), ready(false), connecting(false),
      http_headers({{"Host", std::format("{}:{}", host, port)},
                    {"Upgrade", "websocket"},
                    {"Connection", "Upgrade"},
                    {"Sec-WebSocket-Version", "13"},
                    {"User-Agent", std::string("AO-SDL/") + ao_sdl_version()}}) {
}

void WebSocket::set_header(const std::string& header, const std::string& value) {
    http_headers[header] = value;
}

void WebSocket::connect() {
    connect("");
}

void WebSocket::connect(const std::string& endpoint) {
    if (ready || connecting) {
        throw WebSocketException("WebSocket connection is either already established or in progress");
    }

    connecting = true;
    generate_mask();
    set_header("Sec-WebSocket-Key", Base64::encode(sec_ws_key));

    socket->set_non_blocking(false);
    socket->connect();

    std::vector<uint8_t> handshake_buf;

    const std::string http_get = std::format("GET /{} HTTP/1.1\r\n", endpoint);
    handshake_buf.insert(handshake_buf.end(), http_get.begin(), http_get.end());

    for (const auto& header : http_headers) {
        std::string header_str = std::format("{}: {}\r\n", header.first, header.second);
        handshake_buf.insert(handshake_buf.end(), header_str.begin(), header_str.end());
    }

    const std::string carriage_return = "\r\n";
    handshake_buf.insert(handshake_buf.end(), carriage_return.begin(), carriage_return.end());
    write_raw(handshake_buf);

    HTTPResponse handshake_response = read_handshake();
    bool handshake_good = false;
    try {
        handshake_good = validate_handshake(handshake_response);
    }
    catch (const WebSocketException&) {
        connecting = false;
        throw;
    }

    if (!handshake_good) {
        connecting = false;
        throw WebSocketException("Unspecified error occurred while validating handshake response");
    }

    ready = true;
    connecting = false;
    socket->set_non_blocking(true);
}

std::vector<::WebSocketFrame> WebSocket::read() {
    if (!ready)
        throw WebSocketException("Cannot read from a closed WebSocket");

    std::vector<::WebSocketFrame> messages;

    do {
        std::vector<uint8_t> bytes = read_raw();

        if (bytes.size() == 0) {
            return messages;
        }

        uint64_t bytes_needed = 2;
        if (bytes.size() < bytes_needed) {
            extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
            return messages;
        }

        uint64_t pl_len = bytes[1] & 0x7F;
        bool masked = (bytes[1] & 0x80) != 0;

        if (masked) {
            throw WebSocketException("Client received a masked frame");
            return messages;
        }

        if (pl_len <= 125) {
            bytes_needed += pl_len;
        }
        else if (pl_len == 126) {
            bytes_needed += sizeof(uint16_t);

            if (bytes.size() < bytes_needed) {
                extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
                return messages;
            }

            uint16_t pl_len_netorder;
            std::memcpy(&pl_len_netorder, &bytes[2], sizeof(uint16_t));
            pl_len = ntohs(pl_len_netorder);
            bytes_needed += pl_len;
        }
        else if (pl_len == 127) {
            bytes_needed += sizeof(uint64_t);

            if (bytes.size() < bytes_needed) {
                extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
                return messages;
            }

            uint64_t pl_len_netorder;
            std::memcpy(&pl_len_netorder, &bytes[2], sizeof(uint64_t));
            pl_len = net_to_host_64(pl_len_netorder);
            bytes_needed += pl_len;
        }

        if (bytes.size() < bytes_needed) {
            extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
            return messages;
        }

        size_t data_offset = bytes_needed - pl_len;

        ::WebSocketFrame frame;
        frame.complete = true;

        frame.fin = (bytes[0] & 0x80) != 0;
        frame.rsv = (bytes[0] & 0x70) >> 4;
        frame.opcode = (::Opcode)(bytes[0] & 0x0F);

        frame.mask = masked;
        frame.len_code = bytes[1] & 0x7F;
        frame.len = pl_len;

        frame.mask_key = 0x00000000;
        if (masked) {
            throw WebSocketException("Client received masked frame from server");
        }

        frame.data.insert(frame.data.end(), bytes.begin() + data_offset, bytes.begin() + bytes_needed);
        if (frame.data.size() != pl_len) {
            throw WebSocketException("Message length was invalid!");
        }

        extra_data.clear();
        extra_data.insert(extra_data.end(), bytes.begin() + bytes_needed, bytes.end());

        // --- Control frames (CLOSE, PING, PONG) ---
        if (frame.opcode == CLOSE) {
            if (ready) {
                uint16_t code = 1000;
                if (frame.data.size() >= 2) {
                    uint16_t net_code;
                    std::memcpy(&net_code, frame.data.data(), 2);
                    code = ntohs(net_code);
                }
                send_close(code, "");
                ready = false;
            }
            throw WebSocketException("Connection closed by server");
        }
        else if (frame.opcode == PING) {
            ::WebSocketFrame pong;
            pong.fin = true;
            pong.rsv = 0;
            pong.opcode = PONG;
            pong.mask = true;
            pong.len = frame.len;
            pong.len_code = frame.len <= 125 ? (uint8_t)frame.len : 126;

            std::vector<uint8_t> randbuf(4);
            std::generate(randbuf.begin(), randbuf.end(), []() { return std::rand() % 256; });
            pong.mask_key = *reinterpret_cast<const uint32_t*>(randbuf.data());

            pong.data = frame.data;
            write_raw(pong.serialize());
        }
        else if (frame.opcode == PONG) {
            // Unsolicited pong — ignore per RFC 6455 §5.5.3
        }
        // --- Data frames (TEXT, BINARY, CONTINUATION) ---
        else if (frame.opcode == CONTINUATION) {
            if (!in_fragment_) {
                throw WebSocketException("Received continuation frame without an initial fragment");
            }
            fragment_buf_.insert(fragment_buf_.end(), frame.data.begin(), frame.data.end());
            if (frame.fin) {
                ::WebSocketFrame assembled;
                assembled.complete = true;
                assembled.fin = true;
                assembled.rsv = 0;
                assembled.opcode = fragment_opcode_;
                assembled.mask = false;
                assembled.len = fragment_buf_.size();
                assembled.len_code = 0;
                assembled.mask_key = 0;
                assembled.data = std::move(fragment_buf_);
                fragment_buf_.clear();
                in_fragment_ = false;
                messages.push_back(std::move(assembled));
            }
        }
        else {
            if (frame.fin) {
                if (in_fragment_) {
                    throw WebSocketException("Received new data frame while still accumulating fragments");
                }
                messages.push_back(frame);
            }
            else {
                if (in_fragment_) {
                    throw WebSocketException("Received new fragmented data frame while still accumulating fragments");
                }
                in_fragment_ = true;
                fragment_opcode_ = frame.opcode;
                fragment_buf_ = std::move(frame.data);
            }
        }
    } while (extra_data.size() != 0);

    return messages;
}

void WebSocket::write(std::span<const uint8_t> data_bytes) {
    if (!ready)
        throw WebSocketException("Cannot write to a closed WebSocket");

    ::WebSocketFrame frame;

    frame.fin = true;
    frame.rsv = 0x00;
    frame.opcode = TEXT;

    frame.mask = true;
    frame.len = data_bytes.size();

    if (frame.len <= 125) {
        frame.len_code = (uint8_t)(frame.len & 0xFF);
    }
    else if (frame.len >= 126 && frame.len <= UINT16_MAX) {
        frame.len_code = 126;
    }
    else {
        frame.len_code = 127;
    }

    std::vector<uint8_t> randbuf(4);
    std::generate(randbuf.begin(), randbuf.end(), []() { return std::rand() % 256; });
    frame.mask_key = *reinterpret_cast<const uint32_t*>(randbuf.data());

    frame.data.insert(frame.data.end(), data_bytes.begin(), data_bytes.end());

    std::vector<uint8_t> wireframe = frame.serialize();
    write_raw(wireframe);
}

bool WebSocket::is_connected() {
    return ready || connecting;
}

void WebSocket::close() {
    close(1000);
}

void WebSocket::close(uint16_t code, const std::string& reason) {
    if (!ready)
        return;
    send_close(code, reason);
    ready = false;
}

void WebSocket::send_close(uint16_t code, const std::string& reason) {
    ::WebSocketFrame frame;
    frame.fin = true;
    frame.rsv = 0;
    frame.opcode = CLOSE;
    frame.mask = true;

    uint16_t net_code = htons(code);
    frame.data.resize(2);
    std::memcpy(frame.data.data(), &net_code, 2);
    if (!reason.empty()) {
        size_t reason_len = std::min(reason.size(), (size_t)123);
        frame.data.insert(frame.data.end(), reason.begin(), reason.begin() + reason_len);
    }

    frame.len = frame.data.size();
    frame.len_code = (uint8_t)frame.len;

    std::vector<uint8_t> randbuf(4);
    std::generate(randbuf.begin(), randbuf.end(), []() { return std::rand() % 256; });
    frame.mask_key = *reinterpret_cast<const uint32_t*>(randbuf.data());

    write_raw(frame.serialize());
}

void WebSocket::generate_mask() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::generate(sec_ws_key.begin(), sec_ws_key.end(), []() { return std::rand() % 256; });
}

void WebSocket::write_raw(std::span<const uint8_t> data_bytes) {
    socket->send(data_bytes.data(), data_bytes.size());
}

std::vector<uint8_t> WebSocket::read_raw() {
    std::vector<uint8_t> out_buf;
    out_buf.insert(out_buf.end(), extra_data.begin(), extra_data.end());
    extra_data.clear();

    do {
        std::vector<uint8_t> chunk = socket->recv();
        out_buf.insert(out_buf.end(), chunk.begin(), chunk.end());
    } while (socket->bytes_available());

    return out_buf;
}

HTTPResponse WebSocket::read_handshake() {
    std::vector<std::string> response_lines;

    while (std::find(response_lines.begin(), response_lines.end(), "") == response_lines.end()) {
        std::vector<uint8_t> response_buf = read_raw();
        auto new_lines = ws::get_lines(response_buf, extra_data);
        response_lines.insert(response_lines.end(), new_lines.begin(), new_lines.end());
    }

    HTTPResponse::StatusLine status_line;
    HTTPHeaders headers;

    std::string status_line_str = response_lines[0];
    response_lines.erase(response_lines.begin());
    status_line = ws::parse_status_line(status_line_str);

    for (const auto& hline : response_lines) {
        auto headerkv = ws::parse_http_header(hline);
        headers.emplace(headerkv);
    }

    HTTPResponse response(status_line, headers);
    return response;
}

bool WebSocket::validate_handshake(const HTTPResponse& response) {
    static constexpr int HTTP_STATUS_SWITCHING_PROTOCOLS = 101;

    if (response.get_status().status_code != HTTP_STATUS_SWITCHING_PROTOCOLS) {
        throw WebSocketException(std::format("Response status code was not 101, got {} ({})",
                                             response.get_status().status_code, response.get_status().status_reason));
    }

    if (!ws::case_insensitive_equal(response.get_header("Upgrade"), "websocket")) {
        throw WebSocketException("Upgrade header not present or malformed");
    }

    if (!ws::case_insensitive_equal(response.get_header("Connection"), "Upgrade")) {
        throw WebSocketException("Connection header not present or malformed");
    }

    std::string ws_accept_recvd = response.get_header("Sec-WebSocket-Accept");
    if (ws_accept_recvd.empty()) {
        throw WebSocketException("Sec-WebSocket-Accept not present");
    }

    std::string ws_accept_calculated = ws::compute_accept_key(Base64::encode(sec_ws_key));
    if (ws_accept_calculated != ws_accept_recvd) {
        throw WebSocketException(std::format("Sec-WebSocket-Accept from response was invalid (expected {}, got {})",
                                             ws_accept_calculated, ws_accept_recvd));
    }

    if (!response.get_header("Sec-WebSocket-Extensions").empty()) {
        throw WebSocketException(std::format("Server requested WebSocket Extensions: {} (none are supported)",
                                             response.get_header("Sec-WebSocket-Extensions")));
    }

    if (!response.get_header("Sec-WebSocket-Protocol").empty()) {
        throw WebSocketException(std::format("Server requested WebSocket Subprotocols: {} (none are supported)",
                                             response.get_header("Sec-WebSocket-Protocol")));
    }

    return true;
}
