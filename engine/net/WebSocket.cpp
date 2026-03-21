#include "net/WebSocket.h"

// Platform network byte-order functions (htonl/htons/ntohs) must come before
// any header that uses them (e.g. sha1.h).
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "net/KissnetTcpSocket.h"
#include "utils/Base64.h"
#include "utils/Version.h"

#include <sha1.h>

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <ctime>
#include <format>
#include <functional>


HTTPResponse::HTTPResponse(StatusLine status_line, HTTPHeaders headers) : status_line(status_line), headers(headers) {
}

HTTPResponse::StatusLine HTTPResponse::get_status() const {
    return status_line;
}

std::string HTTPResponse::get_header(std::string header) const {
    std::string headerval = "";

    auto it = headers.find(header);
    if (it != headers.end()) {
        headerval = it->second;
    }

    return headerval;
}

WebSocket::WebSocket(const std::string& host, uint16_t port)
    : WebSocket(host, port, std::make_unique<KissnetTcpSocket>(host, port)) {
}

WebSocket::WebSocket(const std::string& host, uint16_t port, std::unique_ptr<ITcpSocket> socket)
    : socket(std::move(socket)), ready(false), connecting(false) {
    set_header("Host", std::format("{}:{}", host, port));
    set_header("Upgrade", "websocket");
    set_header("Connection", "Upgrade");
    set_header("Sec-WebSocket-Version", "13");
    set_header("User-Agent", std::string("AO-SDL/") + ao_sdl_version());
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
        std::string key = header.first;
        std::string val = header.second;

        std::string header_str = std::format("{}: {}\r\n", key, val);
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
    catch (const WebSocketException& ex) {
        throw ex;
    }

    if (!handshake_good) {
        throw WebSocketException("Unspecified error occurred while validating handshake response");
    }
    else {
        ready = true;
        connecting = false;
        socket->set_non_blocking(true);
    }
}

std::vector<WebSocket::WebSocketFrame> WebSocket::read() {
    std::vector<WebSocketFrame> messages;

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
            pl_len = WebSocket::net_to_host_64(pl_len_netorder);
            bytes_needed += pl_len;
        }

        if (bytes.size() < bytes_needed) {
            extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
            return messages;
        }

        size_t data_offset = bytes_needed - pl_len;

        WebSocketFrame frame;
        frame.complete = true;

        frame.fin = (bytes[0] & 0x80) != 0;
        frame.rsv = (bytes[0] & 0x70) >> 4;
        frame.opcode = (Opcode)(bytes[0] & 0x0F);

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
        // Control frames may appear between fragments (RFC 6455 §5.5).
        if (frame.opcode == CLOSE) {
            // Echo the close frame back if we haven't already sent one
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
            // Deliver the close frame so callers can see the disconnect
            messages.push_back(frame);
            return messages;
        }
        else if (frame.opcode == PING) {
            WebSocketFrame pong;
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
                // Reassemble the complete message
                WebSocketFrame assembled;
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
            // TEXT or BINARY
            if (frame.fin) {
                // Unfragmented message — deliver directly
                if (in_fragment_) {
                    throw WebSocketException("Received new data frame while still accumulating fragments");
                }
                messages.push_back(frame);
            } else {
                // First fragment of a fragmented message
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
    WebSocketFrame frame;

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
    WebSocketFrame frame;
    frame.fin = true;
    frame.rsv = 0;
    frame.opcode = CLOSE;
    frame.mask = true;

    // Close payload: 2-byte status code + optional reason string
    uint16_t net_code = htons(code);
    frame.data.resize(2);
    std::memcpy(frame.data.data(), &net_code, 2);
    if (!reason.empty()) {
        size_t reason_len = std::min(reason.size(), (size_t)123); // max 125 - 2 for code
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

std::vector<std::string> WebSocket::get_lines(std::span<uint8_t> input) {
    std::vector<std::string> lines;

    static const uint8_t DELIMITER[] = {'\r', '\n'};
    const size_t DELIM_SIZE = sizeof(DELIMITER);

    size_t start = 0;
    while (start < input.size()) {
        auto it = std::search(input.begin() + static_cast<std::ptrdiff_t>(start), input.end(), std::begin(DELIMITER),
                              std::end(DELIMITER));

        if (it == input.end()) {
            auto lastchunk = input.subspan(start);
            extra_data.clear();
            extra_data.insert(extra_data.end(), lastchunk.begin(), lastchunk.end());
            break;
        }
        else {
            size_t foundPos = static_cast<size_t>(std::distance(input.begin(), it));
            size_t chunkSize = (foundPos - start);

            auto chunk = input.subspan(start, chunkSize);
            std::string chunkstr(chunk.begin(), chunk.end());
            lines.push_back(chunkstr);

            start = foundPos + DELIM_SIZE;
        }
    }

    return lines;
}

inline std::string WebSocket::trim(const std::string& str) {
    if (str.empty())
        return str;

    std::size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    std::size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

inline std::string WebSocket::collapse_lws(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    bool in_whitespace = false;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_whitespace = true;
        }
        else {
            if (in_whitespace) {
                result.push_back(' ');
                in_whitespace = false;
            }
            result.push_back(c);
        }
    }

    return result;
}

std::pair<std::string, std::string> WebSocket::parse_http_header(const std::string& header) {
    auto colonPos = header.find(':');
    if (colonPos == std::string::npos) {
        return std::make_pair(trim(header), std::string{});
    }

    std::string fieldName = trim(header.substr(0, colonPos));

    std::string fieldValue;
    if (colonPos + 1 < header.size()) {
        fieldValue = header.substr(colonPos + 1);
    }
    fieldValue = trim(fieldValue);
    fieldValue = collapse_lws(fieldValue);

    return std::make_pair(fieldName, fieldValue);
}

HTTPResponse::StatusLine WebSocket::parse_status_line(const std::string& line) {
    std::size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos) {
        throw WebSocketException("Invalid status line: missing space after HTTP version.");
    }

    std::string httpVersion = line.substr(0, firstSpace);

    std::size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        throw WebSocketException("Invalid status line: missing space after status code.");
    }

    std::string statusCodeStr = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));
    std::string reasonPhrase = line.substr(secondSpace + 1);

    if (httpVersion.size() < 5 || httpVersion.compare(0, 5, "HTTP/") != 0) {
        throw WebSocketException("Invalid HTTP version: must start with 'HTTP/'.");
    }

    int statusCode;
    try {
        statusCode = std::stoi(statusCodeStr);
    }
    catch (...) {
        throw WebSocketException("Invalid status code: not an integer.");
    }

    if (statusCode < 100 || statusCode > 599) {
        throw WebSocketException("Invalid status code: out of HTTP standard range 100-599.");
    }

    if (reasonPhrase.empty()) {
        throw WebSocketException("Empty reason phrase.");
    }

    return HTTPResponse::StatusLine{httpVersion, statusCode, reasonPhrase};
}

HTTPResponse WebSocket::read_handshake() {
    std::vector<std::string> response_lines;

    while (std::find(response_lines.begin(), response_lines.end(), "") == response_lines.end()) {
        std::vector<uint8_t> response_buf = read_raw();
        auto new_lines = get_lines(response_buf);
        response_lines.insert(response_lines.end(), new_lines.begin(), new_lines.end());
    }

    HTTPResponse::StatusLine status_line;
    HTTPHeaders headers;

    std::string status_line_str = response_lines[0];
    response_lines.erase(response_lines.begin());
    status_line = parse_status_line(status_line_str);

    for (const auto& hline : response_lines) {
        auto headerkv = parse_http_header(hline);
        headers.emplace(headerkv);
    }

    HTTPResponse response(status_line, headers);
    return response;
}

bool WebSocket::case_insensitive_equal(const std::string& a, const std::string& b) {
    std::string a_lower, b_lower;
    a_lower.resize(a.size());
    b_lower.resize(b.size());

    std::function strlower = [](unsigned char c) { return std::tolower(c); };

    std::transform(a.begin(), a.end(), a_lower.begin(), strlower);
    std::transform(b.begin(), b.end(), b_lower.begin(), strlower);

    return a_lower == b_lower;
}

bool WebSocket::validate_handshake(const HTTPResponse& response) {
    bool response_code_ok = false;
    bool upgrade_ok = false;
    bool connection_ok = false;
    bool sec_ws_accept_ok = false;
    bool extensions_ok = false;
    bool protocol_ok = false;

    static constexpr int HTTP_STATUS_SWITCHING_PROTOCOLS = 101;

    if (response.get_status().status_code == HTTP_STATUS_SWITCHING_PROTOCOLS) {
        response_code_ok = true;
    }
    else {
        throw WebSocketException(std::format("Response status code was not 101, got {} ({})",
                                             response.get_status().status_code, response.get_status().status_reason));
    }

    if (case_insensitive_equal(response.get_header("Upgrade"), "websocket")) {
        upgrade_ok = true;
    }
    else {
        throw WebSocketException("Upgrade header not present or malformed");
    }

    if (case_insensitive_equal(response.get_header("Connection"), "Upgrade")) {
        connection_ok = true;
    }
    else {
        throw WebSocketException("Connection header not present or malformed");
    }

    std::string ws_accept_recvd = response.get_header("Sec-WebSocket-Accept");
    if (ws_accept_recvd == "") {
        throw WebSocketException("Sec-WebSocket-Accept not present");
    }

    static const std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string magical_maskstr(std::format("{}{}", Base64::encode(sec_ws_key), magic_string));

    SHA1 sha;
    sha.update(magical_maskstr);
    const std::string hashstr = sha.final();
    const std::vector<uint8_t> hash(hashstr.begin(), hashstr.end());

    std::string ws_accept_calculated = Base64::encode(hash);

    if (ws_accept_calculated == ws_accept_recvd) {
        sec_ws_accept_ok = true;
    }
    else {
        throw WebSocketException(std::format("Sec-WebSocket-Accept from response was invalid (expected {}, got {})",
                                             ws_accept_calculated, ws_accept_recvd));
    }

    if (response.get_header("Sec-WebSocket-Extensions") == "") {
        extensions_ok = true;
    }
    else {
        throw WebSocketException(std::format("Server requested WebSocket Extensions: {} (none are supported)",
                                             response.get_header("Sec-WebSocket-Extensions")));
    }

    if (response.get_header("Sec-WebSocket-Protocol") == "") {
        protocol_ok = true;
    }
    else {
        throw WebSocketException(std::format("Server requested WebSocket Subprotocols: {} (none are supported)",
                                             response.get_header("Sec-WebSocket-Protocol")));
    }

    return response_code_ok && upgrade_ok && connection_ok && sec_ws_accept_ok && extensions_ok && protocol_ok;
}

std::vector<uint8_t> WebSocket::WebSocketFrame::serialize() {
    std::vector<uint8_t> out_buf;

    uint8_t fin_rsv_opcode = ((fin ? 0x80 : 0x00) | ((rsv & 0x07) << 4) | (opcode & 0x0F));
    out_buf.push_back(fin_rsv_opcode);

    out_buf.push_back(len_code | (mask ? 0x80 : 0x00));

    std::vector<uint8_t> lenbuf;
    if (len_code == 126) {
        uint16_t net_order = htons((uint16_t)(len & 0xFFFF));
        lenbuf.resize(2);
        std::memcpy(lenbuf.data(), &net_order, sizeof(net_order));
    }
    else if (len_code == 127) {
        uint64_t net_order = WebSocket::host_to_net_64(len);
        lenbuf.resize(8);
        std::memcpy(lenbuf.data(), &net_order, sizeof(net_order));
    }
    out_buf.insert(out_buf.end(), lenbuf.begin(), lenbuf.end());

    if (mask) {
        std::vector<uint8_t> maskbuf(4);
        uint32_t mask_net = htonl(mask_key);
        std::memcpy(maskbuf.data(), &mask_net, sizeof(mask_net));
        out_buf.insert(out_buf.end(), maskbuf.begin(), maskbuf.end());
    }

    out_buf.reserve(out_buf.size() + data.size());

    if (mask) {
        auto key = mask_key;
        size_t idx = 0;
        std::transform(data.begin(), data.end(), std::back_inserter(out_buf), [&key, &idx](uint8_t c) {
            int shift = 24 - ((idx % 4) * 8);
            uint8_t mask_byte = static_cast<uint8_t>((key >> shift) & 0xFF);
            ++idx;
            return static_cast<uint8_t>(c ^ mask_byte);
        });
    }
    else {
        out_buf.insert(out_buf.end(), data.begin(), data.end());
    }

    return out_buf;
}

inline uint64_t WebSocket::net_to_host_64(uint64_t net_value) {
    if constexpr (std::endian::native == std::endian::big) {
        return net_value;
    }
    else if constexpr (std::endian::native == std::endian::little) {
        return BSWAP64(net_value);
    }
    else {
        static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
                      "Unsupported endianess");
    }
}

inline uint64_t WebSocket::host_to_net_64(uint64_t host_value) {
    if constexpr (std::endian::native == std::endian::big) {
        return host_value;
    }
    else if constexpr (std::endian::native == std::endian::little) {
        return BSWAP64(host_value);
    }
    else {
        static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little,
                      "Unsupported endianess");
    }
}
