#include "WebSocket.h"

#include "utils/Base64.h"
#include "utils/sha1.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>
#include <functional>

// todo: support closing, ping/pong, and continuation frames

HTTPResponse::HTTPResponse(StatusLine status_line, HTTPHeaders headers) : status_line(status_line), headers(headers) {
}

HTTPResponse::StatusLine HTTPResponse::get_status() const {
    return status_line;
}

std::string HTTPResponse::get_header(std::string header) const {
    std::string headerval = "";

    // todo: potential issue, headers are case sensitive!!
    try {
        headerval = headers.at(header);
    }
    catch (std::out_of_range) {
        ; // just return empty string
    }

    return headerval;
}

WebSocket::WebSocket(const std::string& host, uint16_t port)
    : tcp_sock(kissnet::endpoint(host, port)), ready(false), connecting(false) {
    set_header("Host", std::format("{}:{}", host, port));
    set_header("Upgrade", "websocket");
    set_header("Connection", "Upgrade");
    set_header("Sec-WebSocket-Version", "13");
    // todo: better versioning
    // this header should probably be set not in the constructor
    set_header("User-Agent", "tsurushiage/indev");
    // todo: more of a note than anything, but may be of interest in the future-
    // Sec-WebSocket-Protocol could provide a graceful transition mechanism if we want to make servers that speak
    // multiple protocols for AO. Other mechanisms like Authorization could provide clean ways of doing client auth for
    // various reasons, like moderator logins or centralized accounts if we move to that model
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

    // Keep things synchronous until we finish the handshake
    tcp_sock.set_non_blocking(false);
    auto stat = tcp_sock.connect();

    std::vector<uint8_t> handshake_buf;

    const std::string http_get = std::format("GET /{} HTTP/1.1\r\n", endpoint);
    handshake_buf.insert(handshake_buf.end(), http_get.begin(), http_get.end());

    for (auto header : http_headers) {
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
        throw;
    }

    if (!handshake_good) {
        throw WebSocketException("Unspecified error occurred while validating handshake response");
    }
    else {
        ready = true;
        connecting = false;
        tcp_sock.set_non_blocking(true);
    }
}

std::vector<WebSocket::WebSocketFrame> WebSocket::read() {
    std::vector<WebSocketFrame> messages;
    std::vector<uint8_t> bytes = read_raw();

    do {
        if (bytes.size() == 0) {
            // Nothing available for us on the socket, so we can just return an empty list
            return messages;
        }

        // We can guarantee that the data we read contains the start of a valid WebSocket frame.
        // If we start processing the data and there is some left over, that's the start of another frame.
        // We put that data into extra_data and read_raw will keep it at the start of the buffer it returns.
        // However, we cannot guarantee we have enough data to figure out the length. If we can't compute
        // the length of the next frame, stick the data we have into extra_data and return.
        uint64_t bytes_needed = 2;
        if (bytes.size() < bytes_needed) {
            extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
            return messages;
        }

        uint64_t pl_len = bytes[1] & 0x7F; // Payload len field per RFC
        bool masked = (bytes[1] & 0x80) != 0;

        if (masked) {
            throw WebSocketException("Client received a masked frame");
            return messages;
        }

        if (pl_len <= 125) {
            // Payload length is 0-125, so we can calculate how many more bytes we need.
            // Don't need to account for mask data as we are a client and should have already failed if it is there.
            bytes_needed += pl_len;
        }
        else if (pl_len == 126) {
            bytes_needed += 2;

            if (bytes.size() < bytes_needed) {
                extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
                return messages;
            }

            // Strict aliasing is for cowards
            pl_len = ntohs(*(uint16_t*)(&bytes[2]));
            bytes_needed += pl_len;
        }
        else if (pl_len == 127) {
            bytes_needed += 8;

            if (bytes.size() < bytes_needed) {
                extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
                return messages;
            }

            // Strict aliasing is still for cowards
            pl_len = ntohll(*(uint64_t*)(&bytes[2]));
            bytes_needed += pl_len;
        }

        if (bytes.size() < bytes_needed) {
            extra_data.insert(extra_data.end(), bytes.begin(), bytes.end());
            return messages;
        }

        size_t data_offset = bytes_needed - pl_len;

        // At this point, we know we have enough data to decode a whole frame.
        WebSocketFrame frame;
        frame.complete = true;

        frame.fin = (bytes[0] & 0x80) != 0;
        frame.rsv = (bytes[0] & 0x70) >> 4;
        frame.opcode = (Opcode)(bytes[0] & 0x0F);

        frame.mask = masked;
        frame.len_code = bytes[1] & 0x7F;
        frame.len = pl_len;

        frame.mask_key = 0x00000000;

        frame.data.insert(frame.data.end(), bytes.begin() + data_offset, bytes.begin() + bytes_needed);
        if (frame.data.size() != pl_len) {
            throw WebSocketException("Message length was invalid!");
        }

        extra_data.clear();
        extra_data.insert(extra_data.end(), bytes.begin() + bytes_needed, bytes.end());

        messages.push_back(frame);
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
        frame.len_code = frame.len;
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

void WebSocket::generate_mask() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::generate(sec_ws_key.begin(), sec_ws_key.end(), []() { return std::rand() % 256; });
}

void WebSocket::write_raw(std::span<const uint8_t> data_bytes) {
    tcp_sock.send(reinterpret_cast<const std::byte*>(data_bytes.data()), data_bytes.size());
}

std::vector<uint8_t> WebSocket::read_raw() {
    std::vector<uint8_t> out_buf;
    out_buf.insert(out_buf.end(), extra_data.begin(), extra_data.end());
    extra_data.clear();

    do {
        kissnet::buffer<RECV_BUF_SIZE> buf;
        const auto [recv_size, status_code] = tcp_sock.recv(buf);

        if (!status_code.valid) {
            throw WebSocketException("TCP socket read invalid");
        }

        if (recv_size != 0) {
            std::transform(buf.begin(), buf.begin() + recv_size, std::back_inserter(out_buf),
                           [](std::byte b) { return static_cast<uint8_t>(b); });
        }
    } while (tcp_sock.bytes_available());

    return out_buf;
}

std::vector<std::string> WebSocket::get_lines(std::span<uint8_t> input) {
    std::vector<std::string> lines;

    // Our delimiter (as bytes)
    static const uint8_t DELIMITER[] = {'\r', '\n'};
    const size_t DELIM_SIZE = sizeof(DELIMITER);

    size_t start = 0;
    while (start < input.size()) {
        // Find the next occurrence of "\r\n" starting from input.begin() + start
        auto it = std::search(input.begin() + static_cast<std::ptrdiff_t>(start), input.end(), std::begin(DELIMITER),
                              std::end(DELIMITER));

        if (it == input.end()) {
            // No more delimiters found, so the rest of the data is a final chunk.
            auto lastchunk = input.subspan(start);
            extra_data.clear(); // Should already be cleared by read_raw(), but do it again to make Extra Sure
            extra_data.insert(extra_data.end(), lastchunk.begin(), lastchunk.end());
            break;
        }
        else {
            // Found the delimiter.
            // Calculate the position and size of this chunk so that it includes the delimiter.
            size_t foundPos = static_cast<size_t>(std::distance(input.begin(), it));
            size_t chunkSize = (foundPos - start);

            auto chunk = input.subspan(start, chunkSize);
            std::string chunkstr(chunk.begin(), chunk.end());
            lines.push_back(chunkstr);

            // Advance start just beyond the found delimiter.
            start = foundPos + DELIM_SIZE;
        }
    }

    return lines;
}

// Helper function to trim leading/trailing whitespace.
inline std::string WebSocket::trim(const std::string& str) {
    if (str.empty())
        return str;

    // find first non-whitespace
    std::size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    // find last non-whitespace
    std::size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

// Helper function to collapse internal linear whitespace (LWS) to a single space.
// According to RFC 2616, LWS inside field-content can be replaced with a single SP.
inline std::string WebSocket::collapse_lws(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    bool in_whitespace = false;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            // If we encounter whitespace, remember it and only add a single space if
            // subsequent characters are non-whitespace.
            in_whitespace = true;
        }
        else {
            if (in_whitespace) {
                // If we were just in whitespace, insert a single space before the next token.
                result.push_back(' ');
                in_whitespace = false;
            }
            result.push_back(c);
        }
    }

    return result;
}

std::pair<std::string, std::string> WebSocket::parse_http_header(const std::string& header) {
    // According to RFC 2616 4.2:
    //    message-header = field-name ":" [ field-value ]

    // 1. Locate the first colon, which separates field-name from field-value.
    auto colonPos = header.find(':');
    if (colonPos == std::string::npos) {
        // No colon found -> Malformed header? Return entire line as field-name or handle differently.
        // Here, we'll treat the entire `header` as a field-name with an empty value:
        return std::make_pair(trim(header), std::string{});
    }

    // 2. Extract the field-name (to the left of the colon).
    std::string fieldName = trim(header.substr(0, colonPos));

    // 3. Extract the field-value (to the right of the colon).
    //    (the substring starts at colonPos+1 to skip the colon)
    std::string fieldValue;
    if (colonPos + 1 < header.size()) {
        fieldValue = header.substr(colonPos + 1);
    }
    fieldValue = trim(fieldValue);

    // 4. Optionally collapse internal whitespace in the field-value to a single space (RFC 2616 LWS rules).
    fieldValue = collapse_lws(fieldValue);

    return std::make_pair(fieldName, fieldValue);
}

HTTPResponse::StatusLine WebSocket::parse_status_line(const std::string& line) {
    // 1. Find first space (between HTTP-Version and Status-Code).
    std::size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos) {
        throw WebSocketException("Invalid status line: missing space after HTTP version.");
    }

    // Extract the HTTP-Version substring.
    std::string httpVersion = line.substr(0, firstSpace);

    std::size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos) {
        throw WebSocketException("Invalid status line: missing space after status code.");
    }

    // Extract the Status-Code substring.
    std::string statusCodeStr = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));

    // 3. Extract the Reason-Phrase (which may contain additional spaces).
    std::string reasonPhrase = line.substr(secondSpace + 1);

    // --- Validate the HTTP-Version. ---
    // Per RFC 2616, typical format is "HTTP/x.x" but we'll do a minimal check here.
    if (httpVersion.size() < 5 || httpVersion.compare(0, 5, "HTTP/") != 0) {
        throw WebSocketException("Invalid HTTP version: must start with 'HTTP/'.");
    }

    // --- Validate the Status-Code. ---
    // Must be numeric, typically 3 digits (e.g., 200). We do a basic integer parse and range check.
    int statusCode;
    try {
        statusCode = std::stoi(statusCodeStr);
    }
    catch (...) {
        throw WebSocketException("Invalid status code: not an integer.");
    }

    // Check typical range: 100-599 (most common HTTP status codes).
    if (statusCode < 100 || statusCode > 599) {
        throw WebSocketException("Invalid status code: out of HTTP standard range 100-599.");
    }

    // Reason-Phrase can be almost anything, but we'll just ensure it's not empty.
    // (RFC 7230 states it's optional, but older RFCs suggest it's typically present.)
    if (reasonPhrase.empty()) {
        throw WebSocketException("Empty reason phrase.");
    }

    // Return the successfully parsed status line.
    return HTTPResponse::StatusLine{httpVersion, statusCode, reasonPhrase};
}

HTTPResponse WebSocket::read_handshake() {
    std::vector<std::string> response_lines;

    // Keep reading in data until we got an empty line (\r\n\r\n in the binary stream)
    while (std::find(response_lines.begin(), response_lines.end(), "") == response_lines.end()) {
        std::vector<uint8_t> response_buf = read_raw();
        auto new_lines = get_lines(response_buf);
        response_lines.insert(response_lines.end(), new_lines.begin(), new_lines.end());
    }

    HTTPResponse::StatusLine status_line;
    HTTPHeaders headers;

    // First, get the response Status-Line
    std::string status_line_str = response_lines[0];
    response_lines.erase(response_lines.begin());
    status_line = parse_status_line(status_line_str);

    // Now, parse the response headers
    for (auto hline : response_lines) {
        auto headerkv = parse_http_header(hline);
        headers.emplace(headerkv);
    }

    HTTPResponse response(status_line, headers);
    return response;
}

bool WebSocket::case_insensitive_compare(const std::string& a, const std::string& b) {
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

    /*
       1.  If the status code received from the server is not 101, the
       client handles the response per HTTP [RFC2616] procedures.  In
       particular, the client might perform authentication if it
       receives a 401 status code; the server might redirect the client
       using a 3xx status code (but clients are not required to follow
       them), etc.  Otherwise, proceed as follows.
    */
    if (response.get_status().status_code == HTTP_STATUS_SWITCHING_PROTOCOLS) {
        response_code_ok = true;
    }
    else {
        // Technically non-conformant, but implementing any other HTTP status code is far, far, far out of scope.
        throw WebSocketException(std::format("Response status code was not 101, got {} ({})",
                                             response.get_status().status_code, response.get_status().status_reason));
    }

    /*
       2.  If the response lacks an |Upgrade| header field or the |Upgrade|
       header field contains a value that is not an ASCII case-
       insensitive match for the value "websocket", the client MUST
       _Fail the WebSocket Connection_.
    */
    if (case_insensitive_compare(response.get_header("Upgrade"), "websocket")) {
        upgrade_ok = true;
    }
    else {
        throw WebSocketException("Upgrade header not present or malformed");
    }

    /*
       3.  If the response lacks a |Connection| header field or the
       |Connection| header field doesn't contain a token that is an
       ASCII case-insensitive match for the value "Upgrade", the client
       MUST _Fail the WebSocket Connection_.
    */
    if (case_insensitive_compare(response.get_header("Connection"), "Upgrade")) {
        connection_ok = true;
    }
    else {
        throw WebSocketException("Connection header not present or malformed");
    }

    /*
       4.  If the response lacks a |Sec-WebSocket-Accept| header field or
       the |Sec-WebSocket-Accept| contains a value other than the
       base64-encoded SHA-1 of the concatenation of the |Sec-WebSocket-
       Key| (as a string, not base64-decoded) with the string "258EAFA5-
       E914-47DA-95CA-C5AB0DC85B11" but ignoring any leading and
       trailing whitespace, the client MUST _Fail the WebSocket
       Connection_.
    */
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

    /*
       5.  If the response includes a |Sec-WebSocket-Extensions| header
       field and this header field indicates the use of an extension
       that was not present in the client's handshake (the server has
       indicated an extension not requested by the client), the client
       MUST _Fail the WebSocket Connection_.  (The parsing of this
       header field to determine which extensions are requested is
       discussed in Section 9.1.)
     */

    if (response.get_header("Sec-WebSocket-Extensions") == "") {
        extensions_ok = true;
    }
    else {
        throw WebSocketException(std::format("Server requested WebSocket Extensions: {} (none are supported)",
                                             response.get_header("Sec-WebSocket-Extensions")));
    }

    /*
       6.  If the response includes a |Sec-WebSocket-Protocol| header field
       and this header field indicates the use of a subprotocol that was
       not present in the client's handshake (the server has indicated a
       subprotocol not requested by the client), the client MUST _Fail
       the WebSocket Connection_.
     */

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
        uint64_t net_order = htonll(len);
        lenbuf.resize(8);
        std::memcpy(lenbuf.data(), &net_order, sizeof(net_order));
    }
    out_buf.insert(out_buf.end(), lenbuf.begin(), lenbuf.end());

    std::vector<uint8_t> maskbuf(4);
    uint32_t mask_net = htonl(mask_key);
    std::memcpy(maskbuf.data(), &mask_net, sizeof(mask_net));
    out_buf.insert(out_buf.end(), maskbuf.begin(), maskbuf.end());

    out_buf.reserve(out_buf.size() + data.size());
    std::transform(data.begin(), data.end(), std::back_inserter(out_buf),
                   [mask_key = this->mask_key, index = size_t(0)](uint8_t c) mutable {
                       int shift = 24 - ((index % 4) * 8);
                       uint8_t mask_byte = static_cast<uint8_t>((mask_key >> shift) & 0xFF);

                       ++index;
                       return static_cast<uint8_t>(c ^ mask_byte);
                   });

    return out_buf;
}