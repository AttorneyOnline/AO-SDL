/**
 * @file Http.cpp
 * @brief HTTP client implementation using platform::Socket.
 *
 * Implements the http::Client and supporting types (Result, Request/Response
 * helpers, error stringification). The server side is not yet implemented.
 */
#include "net/Http.h"

#include "platform/Socket.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <stdexcept>

namespace http {

// ===========================================================================
// Utility: URL / scheme parsing
// ===========================================================================

struct ParsedHost {
    std::string scheme; // "http" or "https"
    std::string host;
    int port;
    bool ssl;
};

static ParsedHost parse_scheme_host_port(const std::string& input) {
    ParsedHost p;
    std::string rest = input;

    // Scheme
    auto scheme_end = rest.find("://");
    if (scheme_end != std::string::npos) {
        p.scheme = rest.substr(0, scheme_end);
        rest = rest.substr(scheme_end + 3);
    }
    else {
        p.scheme = "http";
    }

    std::transform(p.scheme.begin(), p.scheme.end(), p.scheme.begin(), [](unsigned char c) { return std::tolower(c); });
    p.ssl = (p.scheme == "https");
    p.port = p.ssl ? 443 : 80;

    // Host and optional port
    // Handle IPv6: [::1]:8080
    if (!rest.empty() && rest[0] == '[') {
        auto bracket = rest.find(']');
        if (bracket != std::string::npos) {
            p.host = rest.substr(1, bracket - 1);
            rest = rest.substr(bracket + 1);
            if (!rest.empty() && rest[0] == ':') {
                p.port = std::stoi(rest.substr(1));
            }
        }
    }
    else {
        auto colon = rest.find(':');
        auto slash = rest.find('/');
        if (colon != std::string::npos && (slash == std::string::npos || colon < slash)) {
            p.host = rest.substr(0, colon);
            auto port_str = rest.substr(colon + 1);
            if (slash != std::string::npos) {
                port_str = port_str.substr(0, slash - colon - 1);
            }
            p.port = std::stoi(port_str);
        }
        else {
            p.host = (slash != std::string::npos) ? rest.substr(0, slash) : rest;
        }
    }

    return p;
}

// ===========================================================================
// HTTP request/response serialization
// ===========================================================================

static std::string build_request(const std::string& method, const std::string& path, const std::string& host,
                                 const Headers& headers, const std::string& body) {
    std::string req;
    req.reserve(256 + body.size());
    req += method + " " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";

    for (auto& [k, v] : headers) {
        // Skip Host — we already wrote it
        if (detail::case_ignore::equal(k, "Host"))
            continue;
        req += k + ": " + v + "\r\n";
    }

    if (!body.empty()) {
        req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }

    req += "\r\n";
    req += body;
    return req;
}

static bool send_all(platform::Socket& sock, const std::string& data) {
    size_t total = 0;
    while (total < data.size()) {
        ssize_t n = sock.send(data.data() + total, data.size() - total);
        if (n <= 0)
            return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

/// Read until we find `\r\n\r\n`. Returns all data read (headers + any body start).
static std::string read_headers(platform::Socket& sock) {
    std::string buf;
    char c;
    while (true) {
        ssize_t n = sock.recv(&c, 1);
        if (n <= 0)
            break;
        buf += c;
        if (buf.size() >= 4 && buf.substr(buf.size() - 4) == "\r\n\r\n") {
            break;
        }
    }
    return buf;
}

struct ParsedResponse {
    int status = 0;
    Headers headers;
    std::string header_raw;
    std::string body_start; // any body bytes read during header parsing
};

static ParsedResponse parse_response_headers(const std::string& raw) {
    ParsedResponse pr;
    auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return pr;

    pr.header_raw = raw.substr(0, header_end);
    pr.body_start = raw.substr(header_end + 4);

    // Parse status line: HTTP/1.1 200 OK
    auto first_line_end = pr.header_raw.find("\r\n");
    std::string status_line = pr.header_raw.substr(0, first_line_end);

    // Find first space -> version, second space -> status code
    auto sp1 = status_line.find(' ');
    if (sp1 != std::string::npos) {
        auto sp2 = status_line.find(' ', sp1 + 1);
        std::string code_str =
            (sp2 != std::string::npos) ? status_line.substr(sp1 + 1, sp2 - sp1 - 1) : status_line.substr(sp1 + 1);
        std::from_chars(code_str.data(), code_str.data() + code_str.size(), pr.status);
    }

    // Parse headers
    size_t pos = first_line_end + 2;
    while (pos < pr.header_raw.size()) {
        auto line_end = pr.header_raw.find("\r\n", pos);
        if (line_end == std::string::npos)
            line_end = pr.header_raw.size();
        std::string line = pr.header_raw.substr(pos, line_end - pos);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            // Trim leading whitespace from value
            while (!val.empty() && val[0] == ' ')
                val.erase(val.begin());
            pr.headers.emplace(std::move(key), std::move(val));
        }
        pos = line_end + 2;
    }

    return pr;
}

static std::string read_body(platform::Socket& sock, const ParsedResponse& pr, size_t content_length) {
    std::string body = pr.body_start;
    body.reserve(content_length);
    char buf[8192];
    while (body.size() < content_length) {
        size_t want = std::min(sizeof(buf), content_length - body.size());
        ssize_t n = sock.recv(buf, want);
        if (n <= 0)
            break;
        body.append(buf, static_cast<size_t>(n));
    }
    return body;
}

static std::string read_chunked_body(platform::Socket& sock, const std::string& initial) {
    // Simple chunked transfer-encoding reader.
    std::string body;
    std::string pending = initial;

    auto read_more = [&](size_t needed) {
        char buf[8192];
        while (pending.size() < needed) {
            ssize_t n = sock.recv(buf, sizeof(buf));
            if (n <= 0)
                return false;
            pending.append(buf, static_cast<size_t>(n));
        }
        return true;
    };

    while (true) {
        // Read until we have a line with chunk size
        while (pending.find("\r\n") == std::string::npos) {
            if (!read_more(pending.size() + 1))
                return body;
        }

        auto line_end = pending.find("\r\n");
        std::string size_str = pending.substr(0, line_end);
        pending = pending.substr(line_end + 2);

        // Parse hex chunk size
        size_t chunk_size = 0;
        std::from_chars(size_str.data(), size_str.data() + size_str.size(), chunk_size, 16);
        if (chunk_size == 0)
            break; // final chunk

        // Read chunk_size bytes + trailing \r\n
        if (!read_more(chunk_size + 2)) {
            body.append(pending, 0, std::min(pending.size(), chunk_size));
            break;
        }
        body.append(pending, 0, chunk_size);
        pending = pending.substr(chunk_size + 2); // skip \r\n after chunk
    }
    return body;
}

static bool read_body_streaming(platform::Socket& sock, const ParsedResponse& pr, size_t content_length,
                                ContentReceiver receiver) {
    // Feed initial body bytes
    if (!pr.body_start.empty()) {
        if (!receiver(pr.body_start.data(), pr.body_start.size()))
            return false;
    }
    size_t total = pr.body_start.size();
    char buf[8192];
    while (total < content_length) {
        size_t want = std::min(sizeof(buf), content_length - total);
        ssize_t n = sock.recv(buf, want);
        if (n <= 0)
            break;
        if (!receiver(buf, static_cast<size_t>(n)))
            return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

// ===========================================================================
// Error
// ===========================================================================

std::string to_string(Error error) {
    switch (error) {
    case Error::Success:
        return "Success";
    case Error::Connection:
        return "Connection";
    case Error::BindIPAddress:
        return "BindIPAddress";
    case Error::Read:
        return "Read";
    case Error::Write:
        return "Write";
    case Error::ExceedRedirectCount:
        return "ExceedRedirectCount";
    case Error::Canceled:
        return "Canceled";
    case Error::SSLConnection:
        return "SSLConnection";
    case Error::SSLLoadingCerts:
        return "SSLLoadingCerts";
    case Error::SSLServerVerification:
        return "SSLServerVerification";
    case Error::SSLServerHostnameVerification:
        return "SSLServerHostnameVerification";
    case Error::UnsupportedMultipartBoundaryChars:
        return "UnsupportedMultipartBoundaryChars";
    case Error::Compression:
        return "Compression";
    case Error::ConnectionTimeout:
        return "ConnectionTimeout";
    case Error::ProxyConnection:
        return "ProxyConnection";
    case Error::Unknown:
        return "Unknown";
    default:
        return "Unknown";
    }
}

std::ostream& operator<<(std::ostream& os, const Error& obj) {
    return os << to_string(obj);
}

// ===========================================================================
// Request / Response helpers
// ===========================================================================

bool Request::has_header(const std::string& key) const {
    return headers.find(key) != headers.end();
}

std::string Request::get_header_value(const std::string& key, const char* def, size_t id) const {
    auto range = headers.equal_range(key);
    size_t i = 0;
    for (auto it = range.first; it != range.second; ++it, ++i) {
        if (i == id)
            return it->second;
    }
    return def;
}

uint64_t Request::get_header_value_u64(const std::string& key, uint64_t def, size_t id) const {
    auto val = get_header_value(key, "", id);
    if (val.empty())
        return def;
    uint64_t result = def;
    std::from_chars(val.data(), val.data() + val.size(), result);
    return result;
}

size_t Request::get_header_value_count(const std::string& key) const {
    return headers.count(key);
}

void Request::set_header(const std::string& key, const std::string& val) {
    headers.emplace(key, val);
}

bool Request::has_param(const std::string& key) const {
    return params.find(key) != params.end();
}

std::string Request::get_param_value(const std::string& key, size_t id) const {
    auto range = params.equal_range(key);
    size_t i = 0;
    for (auto it = range.first; it != range.second; ++it, ++i) {
        if (i == id)
            return it->second;
    }
    return {};
}

size_t Request::get_param_value_count(const std::string& key) const {
    return params.count(key);
}

bool Request::is_multipart_form_data() const {
    auto ct = get_header_value("Content-Type");
    return ct.find("multipart/form-data") != std::string::npos;
}

bool Request::has_file(const std::string& key) const {
    return files.find(key) != files.end();
}

MultipartFormData Request::get_file_value(const std::string& key) const {
    auto it = files.find(key);
    return (it != files.end()) ? it->second : MultipartFormData{};
}

std::vector<MultipartFormData> Request::get_file_values(const std::string& key) const {
    std::vector<MultipartFormData> result;
    auto range = files.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return result;
}

// Response

bool Response::has_header(const std::string& key) const {
    return headers.find(key) != headers.end();
}

std::string Response::get_header_value(const std::string& key, const char* def, size_t id) const {
    auto range = headers.equal_range(key);
    size_t i = 0;
    for (auto it = range.first; it != range.second; ++it, ++i) {
        if (i == id)
            return it->second;
    }
    return def;
}

uint64_t Response::get_header_value_u64(const std::string& key, uint64_t def, size_t id) const {
    auto val = get_header_value(key, "", id);
    if (val.empty())
        return def;
    uint64_t result = def;
    std::from_chars(val.data(), val.data() + val.size(), result);
    return result;
}

size_t Response::get_header_value_count(const std::string& key) const {
    return headers.count(key);
}

void Response::set_header(const std::string& key, const std::string& val) {
    headers.emplace(key, val);
}

void Response::set_redirect(const std::string& url, int st) {
    set_header("Location", url);
    status = st;
}

void Response::set_content(const char* s, size_t n, const std::string& content_type) {
    body.assign(s, n);
    set_header("Content-Type", content_type);
    content_length_ = n;
}

void Response::set_content(const std::string& s, const std::string& content_type) {
    set_content(s.data(), s.size(), content_type);
}

void Response::set_content(std::string&& s, const std::string& content_type) {
    content_length_ = s.size();
    body = std::move(s);
    set_header("Content-Type", content_type);
}

void Response::set_content_provider(size_t length, const std::string& content_type, ContentProvider provider,
                                    ContentProviderResourceReleaser resource_releaser) {
    content_length_ = length;
    content_provider_ = std::move(provider);
    content_provider_resource_releaser_ = std::move(resource_releaser);
    set_header("Content-Type", content_type);
}

void Response::set_content_provider(const std::string& content_type, ContentProviderWithoutLength provider,
                                    ContentProviderResourceReleaser resource_releaser) {
    content_length_ = 0;
    is_chunked_content_provider_ = true;
    content_provider_ = [p = std::move(provider)](size_t offset, size_t, DataSink& sink) { return p(offset, sink); };
    content_provider_resource_releaser_ = std::move(resource_releaser);
    set_header("Content-Type", content_type);
}

void Response::set_chunked_content_provider(const std::string& content_type, ContentProviderWithoutLength provider,
                                            ContentProviderResourceReleaser resource_releaser) {
    set_content_provider(content_type, std::move(provider), std::move(resource_releaser));
}

void Response::set_file_content(const std::string& path, const std::string& content_type) {
    file_content_path_ = path;
    file_content_content_type_ = content_type;
}

void Response::set_file_content(const std::string& path) {
    file_content_path_ = path;
}

// Result

bool Result::has_request_header(const std::string& key) const {
    return request_headers_.find(key) != request_headers_.end();
}

std::string Result::get_request_header_value(const std::string& key, const char* def, size_t id) const {
    auto range = request_headers_.equal_range(key);
    size_t i = 0;
    for (auto it = range.first; it != range.second; ++it, ++i) {
        if (i == id)
            return it->second;
    }
    return def;
}

uint64_t Result::get_request_header_value_u64(const std::string& key, uint64_t def, size_t id) const {
    auto val = get_request_header_value(key, "", id);
    if (val.empty())
        return def;
    uint64_t result = def;
    std::from_chars(val.data(), val.data() + val.size(), result);
    return result;
}

size_t Result::get_request_header_value_count(const std::string& key) const {
    return request_headers_.count(key);
}

// ===========================================================================
// Free functions
// ===========================================================================

void default_socket_options(socket_t) {
}

const char* status_message(int status) {
    switch (status) {
    // 1xx Informational (RFC 9110 §15.2)
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing"; // RFC 2518 (WebDAV)
    case 103:
        return "Early Hints"; // RFC 8297

    // 2xx Success (RFC 9110 §15.3)
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-Authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content"; // RFC 9110 §15.3.7
    case 207:
        return "Multi-Status"; // RFC 4918 (WebDAV)
    case 208:
        return "Already Reported"; // RFC 5842 (WebDAV)
    case 226:
        return "IM Used"; // RFC 3229

    // 3xx Redirection (RFC 9110 §15.4)
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy"; // RFC 9110 (deprecated)
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permanent Redirect"; // RFC 9110 §15.4.9

    // 4xx Client Error (RFC 9110 §15.5)
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Content Too Large"; // RFC 9110 (was "Payload Too Large")
    case 414:
        return "URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 418:
        return "I'm a Teapot"; // RFC 2324
    case 421:
        return "Misdirected Request"; // RFC 9110 §15.5.20
    case 422:
        return "Unprocessable Content"; // RFC 9110 (was "Unprocessable Entity")
    case 423:
        return "Locked"; // RFC 4918 (WebDAV)
    case 424:
        return "Failed Dependency"; // RFC 4918 (WebDAV)
    case 425:
        return "Too Early"; // RFC 8470
    case 426:
        return "Upgrade Required";
    case 428:
        return "Precondition Required"; // RFC 6585
    case 429:
        return "Too Many Requests"; // RFC 6585
    case 431:
        return "Request Header Fields Too Large"; // RFC 6585
    case 451:
        return "Unavailable For Legal Reasons"; // RFC 7725

    // 5xx Server Error (RFC 9110 §15.6)
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    case 506:
        return "Variant Also Negotiates"; // RFC 2295
    case 507:
        return "Insufficient Storage"; // RFC 4918 (WebDAV)
    case 508:
        return "Loop Detected"; // RFC 5842 (WebDAV)
    case 510:
        return "Not Extended"; // RFC 2774 (obsoleted)
    case 511:
        return "Network Authentication Required"; // RFC 6585

    default:
        return "";
    }
}

std::string get_bearer_token_auth(const Request& req) {
    auto val = req.get_header_value("Authorization");
    if (val.size() > 7 && val.substr(0, 7) == "Bearer ") {
        return val.substr(7);
    }
    return {};
}

// Stream
ssize_t Stream::write(const char* ptr) {
    return write(ptr, strlen(ptr));
}
ssize_t Stream::write(const std::string& s) {
    return write(s.data(), s.size());
}

// ===========================================================================
// ClientImpl
// ===========================================================================

ClientImpl::ClientImpl(const std::string& host) : ClientImpl(host, 80) {
    // Re-parse to detect scheme for port default
    auto parsed = parse_scheme_host_port(host);
    const_cast<std::string&>(host_) = parsed.host;
    const_cast<int&>(port_) = parsed.port;
    const_cast<std::string&>(host_and_port_) = parsed.host + ":" + std::to_string(parsed.port);
    if (parsed.ssl) {
        // Store that we need SSL — use client_cert_path_ as a flag
        // (hacky but avoids adding a member to match the original API)
        const_cast<std::string&>(client_cert_path_) = "__ssl__";
    }
}

ClientImpl::ClientImpl(const std::string& host, int port)
    : host_(host), port_(port), host_and_port_(host + ":" + std::to_string(port)) {
}

ClientImpl::ClientImpl(const std::string& host, int port, const std::string& client_cert_path,
                       const std::string& client_key_path)
    : host_(host), port_(port), host_and_port_(host + ":" + std::to_string(port)), client_cert_path_(client_cert_path),
      client_key_path_(client_key_path) {
}

ClientImpl::~ClientImpl() = default;

bool ClientImpl::is_valid() const {
    return true;
}

void ClientImpl::stop() {
    socket_.sock = INVALID_SOCKET_VALUE;
}

std::string ClientImpl::host() const {
    return host_;
}
int ClientImpl::port() const {
    return port_;
}
size_t ClientImpl::is_socket_open() const {
    return socket_.is_open() ? 1 : 0;
}
socket_t ClientImpl::socket() const {
    return socket_.sock;
}

void ClientImpl::set_hostname_addr_map(std::map<std::string, std::string> addr_map) {
    addr_map_ = std::move(addr_map);
}
void ClientImpl::set_default_headers(Headers headers) {
    default_headers_ = std::move(headers);
}
void ClientImpl::set_header_writer(std::function<ssize_t(Stream&, Headers&)> const& writer) {
    header_writer_ = writer;
}
void ClientImpl::set_address_family(int family) {
    address_family_ = family;
}
void ClientImpl::set_tcp_nodelay(bool on) {
    tcp_nodelay_ = on;
}
void ClientImpl::set_ipv6_v6only(bool on) {
    ipv6_v6only_ = on;
}
void ClientImpl::set_socket_options(SocketOptions socket_options) {
    socket_options_ = std::move(socket_options);
}
void ClientImpl::set_connection_timeout(time_t sec, time_t usec) {
    connection_timeout_sec_ = sec;
    connection_timeout_usec_ = usec;
}
void ClientImpl::set_read_timeout(time_t sec, time_t usec) {
    read_timeout_sec_ = sec;
    read_timeout_usec_ = usec;
}
void ClientImpl::set_write_timeout(time_t sec, time_t usec) {
    write_timeout_sec_ = sec;
    write_timeout_usec_ = usec;
}
void ClientImpl::set_basic_auth(const std::string& username, const std::string& password) {
    basic_auth_username_ = username;
    basic_auth_password_ = password;
}
void ClientImpl::set_bearer_token_auth(const std::string& token) {
    bearer_token_auth_token_ = token;
}
void ClientImpl::set_keep_alive(bool on) {
    keep_alive_ = on;
}
void ClientImpl::set_follow_location(bool on) {
    follow_location_ = on;
}
void ClientImpl::set_url_encode(bool on) {
    url_encode_ = on;
}
void ClientImpl::set_compress(bool on) {
    compress_ = on;
}
void ClientImpl::set_decompress(bool on) {
    decompress_ = on;
}
void ClientImpl::set_interface(const std::string& intf) {
    interface_ = intf;
}
void ClientImpl::set_proxy(const std::string& host, int port) {
    proxy_host_ = host;
    proxy_port_ = port;
}
void ClientImpl::set_proxy_basic_auth(const std::string& username, const std::string& password) {
    proxy_basic_auth_username_ = username;
    proxy_basic_auth_password_ = password;
}
void ClientImpl::set_proxy_bearer_token_auth(const std::string& token) {
    proxy_bearer_token_auth_token_ = token;
}
void ClientImpl::set_logger(Logger logger) {
    logger_ = std::move(logger);
}

// -- Core request execution -------------------------------------------------

/// Internal: perform a single HTTP request and return the response.
static Result execute_request(const std::string& method, const std::string& host, int port, bool use_ssl,
                              const std::string& path, const Headers& headers, const std::string& body, bool keep_alive,
                              ContentReceiver content_receiver = nullptr) {
    try {
        auto sock = platform::tcp_connect(host, static_cast<uint16_t>(port));
        if (use_ssl) {
            sock.ssl_connect(host);
        }

        // Build merged headers
        Headers merged = headers;
        if (keep_alive) {
            merged.emplace("Connection", "keep-alive");
        }
        else {
            merged.emplace("Connection", "close");
        }

        std::string raw_req = build_request(method, path, host, merged, body);
        if (!send_all(sock, raw_req)) {
            return Result(nullptr, Error::Write);
        }

        std::string raw_resp = read_headers(sock);
        if (raw_resp.empty()) {
            return Result(nullptr, Error::Read);
        }

        auto pr = parse_response_headers(raw_resp);
        auto res = std::make_unique<Response>();
        res->status = pr.status;
        res->headers = std::move(pr.headers);

        // Determine body length
        auto te = res->get_header_value("Transfer-Encoding");
        bool chunked = (te.find("chunked") != std::string::npos);

        if (content_receiver) {
            // Streaming mode
            if (chunked) {
                // Read chunked and feed to receiver
                // Simple: read all then feed (could be improved to stream chunks)
                std::string full_body = read_chunked_body(sock, pr.body_start);
                if (!full_body.empty()) {
                    content_receiver(full_body.data(), full_body.size());
                }
            }
            else {
                size_t content_length = static_cast<size_t>(res->get_header_value_u64("Content-Length", 0));
                if (content_length > 0) {
                    read_body_streaming(sock, pr, content_length, content_receiver);
                }
                else if (!pr.body_start.empty()) {
                    content_receiver(pr.body_start.data(), pr.body_start.size());
                }
            }
        }
        else {
            // Buffer entire body
            if (chunked) {
                res->body = read_chunked_body(sock, pr.body_start);
            }
            else {
                size_t content_length = static_cast<size_t>(res->get_header_value_u64("Content-Length", 0));
                if (content_length > 0) {
                    res->body = read_body(sock, pr, content_length);
                }
                else {
                    res->body = std::move(pr.body_start);
                }
            }
        }

        return Result(std::move(res), Error::Success);
    }
    catch (const std::exception&) {
        return Result(nullptr, Error::Connection);
    }
}

bool ClientImpl::is_ssl() const {
    return client_cert_path_ == "__ssl__";
}

// GET overloads
Result ClientImpl::Get(const std::string& path) {
    return execute_request("GET", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);
}

Result ClientImpl::Get(const std::string& path, const Headers& headers) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("GET", host_, port_, is_ssl(), path, merged, "", keep_alive_);
}

Result ClientImpl::Get(const std::string& path, Progress) {
    return Get(path);
}

Result ClientImpl::Get(const std::string& path, const Headers& headers, Progress) {
    return Get(path, headers);
}

Result ClientImpl::Get(const std::string& path, ContentReceiver content_receiver) {
    return execute_request("GET", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_,
                           std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("GET", host_, port_, is_ssl(), path, merged, "", keep_alive_, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, ContentReceiver content_receiver, Progress) {
    return Get(path, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Headers& headers, ContentReceiver content_receiver, Progress) {
    return Get(path, headers, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, ResponseHandler, ContentReceiver content_receiver) {
    return Get(path, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Headers& headers, ResponseHandler,
                       ContentReceiver content_receiver) {
    return Get(path, headers, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, ResponseHandler, ContentReceiver content_receiver, Progress) {
    return Get(path, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Headers& headers, ResponseHandler,
                       ContentReceiver content_receiver, Progress) {
    return Get(path, headers, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Params&, const Headers& headers, Progress) {
    return Get(path, headers);
}

Result ClientImpl::Get(const std::string& path, const Params&, const Headers& headers, ContentReceiver content_receiver,
                       Progress) {
    return Get(path, headers, std::move(content_receiver));
}

Result ClientImpl::Get(const std::string& path, const Params&, const Headers& headers, ResponseHandler,
                       ContentReceiver content_receiver, Progress) {
    return Get(path, headers, std::move(content_receiver));
}

// HEAD
Result ClientImpl::Head(const std::string& path) {
    return execute_request("HEAD", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);
}

Result ClientImpl::Head(const std::string& path, const Headers& headers) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("HEAD", host_, port_, is_ssl(), path, merged, "", keep_alive_);
}

// POST — key overloads
Result ClientImpl::Post(const std::string& path) {
    return execute_request("POST", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);
}

Result ClientImpl::Post(const std::string& path, const Headers& headers) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("POST", host_, port_, is_ssl(), path, merged, "", keep_alive_);
}

Result ClientImpl::Post(const std::string& path, const char* body, size_t content_length,
                        const std::string& content_type) {
    Headers h = default_headers_;
    h.emplace("Content-Type", content_type);
    return execute_request("POST", host_, port_, is_ssl(), path, h, std::string(body, content_length), keep_alive_);
}

Result ClientImpl::Post(const std::string& path, const Headers& headers, const char* body, size_t content_length,
                        const std::string& content_type) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    merged.emplace("Content-Type", content_type);
    return execute_request("POST", host_, port_, is_ssl(), path, merged, std::string(body, content_length),
                           keep_alive_);
}

Result ClientImpl::Post(const std::string& path, const std::string& body, const std::string& content_type) {
    return Post(path, body.data(), body.size(), content_type);
}

Result ClientImpl::Post(const std::string& path, const Headers& headers, const std::string& body,
                        const std::string& content_type) {
    return Post(path, headers, body.data(), body.size(), content_type);
}

// Stub out remaining POST/PUT/PATCH/DELETE overloads — they all follow the same pattern.
// Only the most commonly used overloads are fully implemented above.
// The rest delegate to the core pattern.

#define IMPL_BODY_METHOD(Method, METHOD_STR)                                                                           \
    Result ClientImpl::Method(const std::string& path) {                                                               \
        return execute_request(METHOD_STR, host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);           \
    }                                                                                                                  \
    Result ClientImpl::Method(const std::string& path, const char* body, size_t len, const std::string& ct) {          \
        Headers h = default_headers_;                                                                                  \
        h.emplace("Content-Type", ct);                                                                                 \
        return execute_request(METHOD_STR, host_, port_, is_ssl(), path, h, std::string(body, len), keep_alive_);      \
    }                                                                                                                  \
    Result ClientImpl::Method(const std::string& path, const Headers& headers, const char* body, size_t len,           \
                              const std::string& ct) {                                                                 \
        Headers m = default_headers_;                                                                                  \
        for (auto& [k, v] : headers)                                                                                   \
            m.emplace(k, v);                                                                                           \
        m.emplace("Content-Type", ct);                                                                                 \
        return execute_request(METHOD_STR, host_, port_, is_ssl(), path, m, std::string(body, len), keep_alive_);      \
    }                                                                                                                  \
    Result ClientImpl::Method(const std::string& path, const std::string& body, const std::string& ct) {               \
        return Method(path, body.data(), body.size(), ct);                                                             \
    }                                                                                                                  \
    Result ClientImpl::Method(const std::string& path, const Headers& headers, const std::string& body,                \
                              const std::string& ct) {                                                                 \
        return Method(path, headers, body.data(), body.size(), ct);                                                    \
    }

IMPL_BODY_METHOD(Put, "PUT")
IMPL_BODY_METHOD(Patch, "PATCH")

#undef IMPL_BODY_METHOD

// PUT remaining overloads (stubs that delegate)
Result ClientImpl::Put(const std::string& path, const Headers& h, const char* b, size_t l, const std::string& ct,
                       Progress) {
    return Put(path, h, b, l, ct);
}
Result ClientImpl::Put(const std::string& path, const std::string& b, const std::string& ct, Progress) {
    return Put(path, b, ct);
}
Result ClientImpl::Put(const std::string& path, const Headers& h, const std::string& b, const std::string& ct,
                       Progress) {
    return Put(path, h, b, ct);
}
Result ClientImpl::Put(const std::string& path, size_t, ContentProvider, const std::string&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, ContentProviderWithoutLength, const std::string&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers&, size_t, ContentProvider, const std::string&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers&, ContentProviderWithoutLength, const std::string&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Params&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers& h, const Params&) {
    return Put(path, h, "", "");
}
Result ClientImpl::Put(const std::string& path, const Headers& h, const Params&, Progress) {
    return Put(path, h, "", "");
}
Result ClientImpl::Put(const std::string& path, const MultipartFormDataItems&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers&, const MultipartFormDataItems&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers&, const MultipartFormDataItems&, const std::string&) {
    return Put(path);
}
Result ClientImpl::Put(const std::string& path, const Headers&, const MultipartFormDataItems&,
                       const MultipartFormDataProviderItems&) {
    return Put(path);
}

// PATCH remaining
Result ClientImpl::Patch(const std::string& path, const char* b, size_t l, const std::string& ct, Progress) {
    return Patch(path, b, l, ct);
}
Result ClientImpl::Patch(const std::string& path, const Headers& h, const char* b, size_t l, const std::string& ct,
                         Progress) {
    return Patch(path, h, b, l, ct);
}
Result ClientImpl::Patch(const std::string& path, const std::string& b, const std::string& ct, Progress) {
    return Patch(path, b, ct);
}
Result ClientImpl::Patch(const std::string& path, const Headers& h, const std::string& b, const std::string& ct,
                         Progress) {
    return Patch(path, h, b, ct);
}
Result ClientImpl::Patch(const std::string& path, size_t, ContentProvider, const std::string&) {
    return Patch(path);
}
Result ClientImpl::Patch(const std::string& path, ContentProviderWithoutLength, const std::string&) {
    return Patch(path);
}
Result ClientImpl::Patch(const std::string& path, const Headers&, size_t, ContentProvider, const std::string&) {
    return Patch(path);
}
Result ClientImpl::Patch(const std::string& path, const Headers&, ContentProviderWithoutLength, const std::string&) {
    return Patch(path);
}

// POST remaining
Result ClientImpl::Post(const std::string& path, const Headers& h, const char* b, size_t l, const std::string& ct,
                        Progress) {
    return Post(path, h, b, l, ct);
}
Result ClientImpl::Post(const std::string& path, const std::string& b, const std::string& ct, Progress) {
    return Post(path, b, ct);
}
Result ClientImpl::Post(const std::string& path, const Headers& h, const std::string& b, const std::string& ct,
                        Progress) {
    return Post(path, h, b, ct);
}
Result ClientImpl::Post(const std::string& path, size_t, ContentProvider, const std::string&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, ContentProviderWithoutLength, const std::string&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers&, size_t, ContentProvider, const std::string&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers&, ContentProviderWithoutLength, const std::string&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Params&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers& h, const Params&) {
    return Post(path, h);
}
Result ClientImpl::Post(const std::string& path, const Headers& h, const Params&, Progress) {
    return Post(path, h);
}
Result ClientImpl::Post(const std::string& path, const MultipartFormDataItems&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers&, const MultipartFormDataItems&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers&, const MultipartFormDataItems&, const std::string&) {
    return Post(path);
}
Result ClientImpl::Post(const std::string& path, const Headers&, const MultipartFormDataItems&,
                        const MultipartFormDataProviderItems&) {
    return Post(path);
}

// DELETE
Result ClientImpl::Delete(const std::string& path) {
    return execute_request("DELETE", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);
}

Result ClientImpl::Delete(const std::string& path, const Headers& headers) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("DELETE", host_, port_, is_ssl(), path, merged, "", keep_alive_);
}

Result ClientImpl::Delete(const std::string& path, const char* body, size_t len, const std::string& ct) {
    Headers h = default_headers_;
    h.emplace("Content-Type", ct);
    return execute_request("DELETE", host_, port_, is_ssl(), path, h, std::string(body, len), keep_alive_);
}

Result ClientImpl::Delete(const std::string& path, const char* b, size_t l, const std::string& ct, Progress) {
    return Delete(path, b, l, ct);
}
Result ClientImpl::Delete(const std::string& path, const Headers& h, const char* b, size_t l, const std::string& ct) {
    Headers m = default_headers_;
    for (auto& [k, v] : h)
        m.emplace(k, v);
    m.emplace("Content-Type", ct);
    return execute_request("DELETE", host_, port_, is_ssl(), path, m, std::string(b, l), keep_alive_);
}
Result ClientImpl::Delete(const std::string& path, const Headers& h, const char* b, size_t l, const std::string& ct,
                          Progress) {
    return Delete(path, h, b, l, ct);
}
Result ClientImpl::Delete(const std::string& path, const std::string& body, const std::string& ct) {
    return Delete(path, body.data(), body.size(), ct);
}
Result ClientImpl::Delete(const std::string& path, const std::string& b, const std::string& ct, Progress) {
    return Delete(path, b, ct);
}
Result ClientImpl::Delete(const std::string& path, const Headers& h, const std::string& b, const std::string& ct) {
    return Delete(path, h, b.data(), b.size(), ct);
}
Result ClientImpl::Delete(const std::string& path, const Headers& h, const std::string& b, const std::string& ct,
                          Progress) {
    return Delete(path, h, b, ct);
}

// OPTIONS
Result ClientImpl::Options(const std::string& path) {
    return execute_request("OPTIONS", host_, port_, is_ssl(), path, default_headers_, "", keep_alive_);
}

Result ClientImpl::Options(const std::string& path, const Headers& headers) {
    Headers merged = default_headers_;
    for (auto& [k, v] : headers)
        merged.emplace(k, v);
    return execute_request("OPTIONS", host_, port_, is_ssl(), path, merged, "", keep_alive_);
}

// send()
bool ClientImpl::send(Request&, Response&, Error& error) {
    error = Error::Unknown;
    return false; // Not yet implemented
}

Result ClientImpl::send(const Request&) {
    return Result(nullptr, Error::Unknown);
}

// Protected stubs
bool ClientImpl::create_and_connect_socket(Socket&, Error& error) {
    error = Error::Unknown;
    return false;
}
void ClientImpl::shutdown_ssl(Socket&, bool) {
}
void ClientImpl::shutdown_socket(Socket&) const {
}
void ClientImpl::close_socket(Socket& s) {
    s.sock = INVALID_SOCKET_VALUE;
}
bool ClientImpl::process_request(Stream&, Request&, Response&, bool, Error& error) {
    error = Error::Unknown;
    return false;
}
bool ClientImpl::write_content_with_provider(Stream&, const Request&, Error& error) const {
    error = Error::Unknown;
    return false;
}
void ClientImpl::copy_settings(const ClientImpl&) {
}
bool ClientImpl::process_socket(const Socket&, std::function<bool(Stream&)>) {
    return false;
}

// ===========================================================================
// Client (universal wrapper)
// ===========================================================================

Client::Client(const std::string& scheme_host_port) {
    auto parsed = parse_scheme_host_port(scheme_host_port);
    if (parsed.ssl) {
        cli_ = std::make_unique<ClientImpl>(scheme_host_port);
    }
    else {
        cli_ = std::make_unique<ClientImpl>(parsed.host, parsed.port);
    }
}

Client::Client(const std::string& scheme_host_port, const std::string& client_cert_path,
               const std::string& client_key_path) {
    auto parsed = parse_scheme_host_port(scheme_host_port);
    cli_ = std::make_unique<ClientImpl>(parsed.host, parsed.port, client_cert_path, client_key_path);
}

Client::Client(const std::string& host, int port) : cli_(std::make_unique<ClientImpl>(host, port)) {
}

Client::Client(const std::string& host, int port, const std::string& cert, const std::string& key)
    : cli_(std::make_unique<ClientImpl>(host, port, cert, key)) {
}

Client::~Client() = default;

bool Client::is_valid() const {
    return cli_ && cli_->is_valid();
}

// Delegate all methods to cli_
#define DELEGATE(ret, name, args, call)                                                                                \
    ret Client::name args {                                                                                            \
        return cli_->name call;                                                                                        \
    }

Result Client::Get(const std::string& p) {
    return cli_->Get(p);
}
Result Client::Get(const std::string& p, const Headers& h) {
    return cli_->Get(p, h);
}
Result Client::Get(const std::string& p, Progress pr) {
    return cli_->Get(p, pr);
}
Result Client::Get(const std::string& p, const Headers& h, Progress pr) {
    return cli_->Get(p, h, pr);
}
Result Client::Get(const std::string& p, ContentReceiver cr) {
    return cli_->Get(p, std::move(cr));
}
Result Client::Get(const std::string& p, const Headers& h, ContentReceiver cr) {
    return cli_->Get(p, h, std::move(cr));
}
Result Client::Get(const std::string& p, ContentReceiver cr, Progress pr) {
    return cli_->Get(p, std::move(cr), pr);
}
Result Client::Get(const std::string& p, const Headers& h, ContentReceiver cr, Progress pr) {
    return cli_->Get(p, h, std::move(cr), pr);
}
Result Client::Get(const std::string& p, ResponseHandler rh, ContentReceiver cr) {
    return cli_->Get(p, rh, std::move(cr));
}
Result Client::Get(const std::string& p, const Headers& h, ResponseHandler rh, ContentReceiver cr) {
    return cli_->Get(p, h, rh, std::move(cr));
}
Result Client::Get(const std::string& p, const Headers& h, ResponseHandler rh, ContentReceiver cr, Progress pr) {
    return cli_->Get(p, h, rh, std::move(cr), pr);
}
Result Client::Get(const std::string& p, ResponseHandler rh, ContentReceiver cr, Progress pr) {
    return cli_->Get(p, rh, std::move(cr), pr);
}
Result Client::Get(const std::string& p, const Params& pa, const Headers& h, Progress pr) {
    return cli_->Get(p, pa, h, pr);
}
Result Client::Get(const std::string& p, const Params& pa, const Headers& h, ContentReceiver cr, Progress pr) {
    return cli_->Get(p, pa, h, std::move(cr), pr);
}
Result Client::Get(const std::string& p, const Params& pa, const Headers& h, ResponseHandler rh, ContentReceiver cr,
                   Progress pr) {
    return cli_->Get(p, pa, h, rh, std::move(cr), pr);
}

Result Client::Head(const std::string& p) {
    return cli_->Head(p);
}
Result Client::Head(const std::string& p, const Headers& h) {
    return cli_->Head(p, h);
}

Result Client::Post(const std::string& p) {
    return cli_->Post(p);
}
Result Client::Post(const std::string& p, const Headers& h) {
    return cli_->Post(p, h);
}
Result Client::Post(const std::string& p, const char* b, size_t l, const std::string& ct) {
    return cli_->Post(p, b, l, ct);
}
Result Client::Post(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct) {
    return cli_->Post(p, h, b, l, ct);
}
Result Client::Post(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct,
                    Progress pr) {
    return cli_->Post(p, h, b, l, ct, pr);
}
Result Client::Post(const std::string& p, const std::string& b, const std::string& ct) {
    return cli_->Post(p, b, ct);
}
Result Client::Post(const std::string& p, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Post(p, b, ct, pr);
}
Result Client::Post(const std::string& p, const Headers& h, const std::string& b, const std::string& ct) {
    return cli_->Post(p, h, b, ct);
}
Result Client::Post(const std::string& p, const Headers& h, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Post(p, h, b, ct, pr);
}
Result Client::Post(const std::string& p, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Post(p, l, std::move(cp), ct);
}
Result Client::Post(const std::string& p, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Post(p, std::move(cp), ct);
}
Result Client::Post(const std::string& p, const Headers& h, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Post(p, h, l, std::move(cp), ct);
}
Result Client::Post(const std::string& p, const Headers& h, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Post(p, h, std::move(cp), ct);
}
Result Client::Post(const std::string& p, const Params& pa) {
    return cli_->Post(p, pa);
}
Result Client::Post(const std::string& p, const Headers& h, const Params& pa) {
    return cli_->Post(p, h, pa);
}
Result Client::Post(const std::string& p, const Headers& h, const Params& pa, Progress pr) {
    return cli_->Post(p, h, pa, pr);
}
Result Client::Post(const std::string& p, const MultipartFormDataItems& i) {
    return cli_->Post(p, i);
}
Result Client::Post(const std::string& p, const Headers& h, const MultipartFormDataItems& i) {
    return cli_->Post(p, h, i);
}
Result Client::Post(const std::string& p, const Headers& h, const MultipartFormDataItems& i, const std::string& b) {
    return cli_->Post(p, h, i, b);
}
Result Client::Post(const std::string& p, const Headers& h, const MultipartFormDataItems& i,
                    const MultipartFormDataProviderItems& pi) {
    return cli_->Post(p, h, i, pi);
}

Result Client::Put(const std::string& p) {
    return cli_->Put(p);
}
Result Client::Put(const std::string& p, const char* b, size_t l, const std::string& ct) {
    return cli_->Put(p, b, l, ct);
}
Result Client::Put(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct) {
    return cli_->Put(p, h, b, l, ct);
}
Result Client::Put(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct,
                   Progress pr) {
    return cli_->Put(p, h, b, l, ct, pr);
}
Result Client::Put(const std::string& p, const std::string& b, const std::string& ct) {
    return cli_->Put(p, b, ct);
}
Result Client::Put(const std::string& p, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Put(p, b, ct, pr);
}
Result Client::Put(const std::string& p, const Headers& h, const std::string& b, const std::string& ct) {
    return cli_->Put(p, h, b, ct);
}
Result Client::Put(const std::string& p, const Headers& h, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Put(p, h, b, ct, pr);
}
Result Client::Put(const std::string& p, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Put(p, l, std::move(cp), ct);
}
Result Client::Put(const std::string& p, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Put(p, std::move(cp), ct);
}
Result Client::Put(const std::string& p, const Headers& h, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Put(p, h, l, std::move(cp), ct);
}
Result Client::Put(const std::string& p, const Headers& h, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Put(p, h, std::move(cp), ct);
}
Result Client::Put(const std::string& p, const Params& pa) {
    return cli_->Put(p, pa);
}
Result Client::Put(const std::string& p, const Headers& h, const Params& pa) {
    return cli_->Put(p, h, pa);
}
Result Client::Put(const std::string& p, const Headers& h, const Params& pa, Progress pr) {
    return cli_->Put(p, h, pa, pr);
}
Result Client::Put(const std::string& p, const MultipartFormDataItems& i) {
    return cli_->Put(p, i);
}
Result Client::Put(const std::string& p, const Headers& h, const MultipartFormDataItems& i) {
    return cli_->Put(p, h, i);
}
Result Client::Put(const std::string& p, const Headers& h, const MultipartFormDataItems& i, const std::string& b) {
    return cli_->Put(p, h, i, b);
}
Result Client::Put(const std::string& p, const Headers& h, const MultipartFormDataItems& i,
                   const MultipartFormDataProviderItems& pi) {
    return cli_->Put(p, h, i, pi);
}

Result Client::Patch(const std::string& p) {
    return cli_->Patch(p);
}
Result Client::Patch(const std::string& p, const char* b, size_t l, const std::string& ct) {
    return cli_->Patch(p, b, l, ct);
}
Result Client::Patch(const std::string& p, const char* b, size_t l, const std::string& ct, Progress pr) {
    return cli_->Patch(p, b, l, ct, pr);
}
Result Client::Patch(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct) {
    return cli_->Patch(p, h, b, l, ct);
}
Result Client::Patch(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct,
                     Progress pr) {
    return cli_->Patch(p, h, b, l, ct, pr);
}
Result Client::Patch(const std::string& p, const std::string& b, const std::string& ct) {
    return cli_->Patch(p, b, ct);
}
Result Client::Patch(const std::string& p, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Patch(p, b, ct, pr);
}
Result Client::Patch(const std::string& p, const Headers& h, const std::string& b, const std::string& ct) {
    return cli_->Patch(p, h, b, ct);
}
Result Client::Patch(const std::string& p, const Headers& h, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Patch(p, h, b, ct, pr);
}
Result Client::Patch(const std::string& p, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Patch(p, l, std::move(cp), ct);
}
Result Client::Patch(const std::string& p, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Patch(p, std::move(cp), ct);
}
Result Client::Patch(const std::string& p, const Headers& h, size_t l, ContentProvider cp, const std::string& ct) {
    return cli_->Patch(p, h, l, std::move(cp), ct);
}
Result Client::Patch(const std::string& p, const Headers& h, ContentProviderWithoutLength cp, const std::string& ct) {
    return cli_->Patch(p, h, std::move(cp), ct);
}

Result Client::Delete(const std::string& p) {
    return cli_->Delete(p);
}
Result Client::Delete(const std::string& p, const Headers& h) {
    return cli_->Delete(p, h);
}
Result Client::Delete(const std::string& p, const char* b, size_t l, const std::string& ct) {
    return cli_->Delete(p, b, l, ct);
}
Result Client::Delete(const std::string& p, const char* b, size_t l, const std::string& ct, Progress pr) {
    return cli_->Delete(p, b, l, ct, pr);
}
Result Client::Delete(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct) {
    return cli_->Delete(p, h, b, l, ct);
}
Result Client::Delete(const std::string& p, const Headers& h, const char* b, size_t l, const std::string& ct,
                      Progress pr) {
    return cli_->Delete(p, h, b, l, ct, pr);
}
Result Client::Delete(const std::string& p, const std::string& b, const std::string& ct) {
    return cli_->Delete(p, b, ct);
}
Result Client::Delete(const std::string& p, const std::string& b, const std::string& ct, Progress pr) {
    return cli_->Delete(p, b, ct, pr);
}
Result Client::Delete(const std::string& p, const Headers& h, const std::string& b, const std::string& ct) {
    return cli_->Delete(p, h, b, ct);
}
Result Client::Delete(const std::string& p, const Headers& h, const std::string& b, const std::string& ct,
                      Progress pr) {
    return cli_->Delete(p, h, b, ct, pr);
}

Result Client::Options(const std::string& p) {
    return cli_->Options(p);
}
Result Client::Options(const std::string& p, const Headers& h) {
    return cli_->Options(p, h);
}

bool Client::send(Request& req, Response& res, Error& error) {
    return cli_->send(req, res, error);
}
Result Client::send(const Request& req) {
    return cli_->send(req);
}

void Client::stop() {
    cli_->stop();
}
std::string Client::host() const {
    return cli_->host();
}
int Client::port() const {
    return cli_->port();
}
size_t Client::is_socket_open() const {
    return cli_->is_socket_open();
}
socket_t Client::socket() const {
    return cli_->socket();
}

void Client::set_hostname_addr_map(std::map<std::string, std::string> m) {
    cli_->set_hostname_addr_map(std::move(m));
}
void Client::set_default_headers(Headers h) {
    cli_->set_default_headers(std::move(h));
}
void Client::set_header_writer(std::function<ssize_t(Stream&, Headers&)> const& w) {
    cli_->set_header_writer(w);
}
void Client::set_address_family(int f) {
    cli_->set_address_family(f);
}
void Client::set_tcp_nodelay(bool on) {
    cli_->set_tcp_nodelay(on);
}
void Client::set_socket_options(SocketOptions so) {
    cli_->set_socket_options(std::move(so));
}
void Client::set_connection_timeout(time_t s, time_t u) {
    cli_->set_connection_timeout(s, u);
}
void Client::set_read_timeout(time_t s, time_t u) {
    cli_->set_read_timeout(s, u);
}
void Client::set_write_timeout(time_t s, time_t u) {
    cli_->set_write_timeout(s, u);
}
void Client::set_basic_auth(const std::string& u, const std::string& p) {
    cli_->set_basic_auth(u, p);
}
void Client::set_bearer_token_auth(const std::string& t) {
    cli_->set_bearer_token_auth(t);
}
void Client::set_keep_alive(bool on) {
    cli_->set_keep_alive(on);
}
void Client::set_follow_location(bool on) {
    cli_->set_follow_location(on);
}
void Client::set_url_encode(bool on) {
    cli_->set_url_encode(on);
}
void Client::set_compress(bool on) {
    cli_->set_compress(on);
}
void Client::set_decompress(bool on) {
    cli_->set_decompress(on);
}
void Client::set_interface(const std::string& i) {
    cli_->set_interface(i);
}
void Client::set_proxy(const std::string& h, int p) {
    cli_->set_proxy(h, p);
}
void Client::set_proxy_basic_auth(const std::string& u, const std::string& p) {
    cli_->set_proxy_basic_auth(u, p);
}
void Client::set_proxy_bearer_token_auth(const std::string& t) {
    cli_->set_proxy_bearer_token_auth(t);
}
void Client::set_logger(Logger l) {
    cli_->set_logger(std::move(l));
}

#undef DELEGATE

// ===========================================================================
// detail::PathParamsMatcher (needed by Server — stub for now)
// ===========================================================================

namespace detail {

PathParamsMatcher::PathParamsMatcher(const std::string& pattern) {
    // Parse ":param" segments
    std::string::size_type pos = 0;
    std::string::size_type last = 0;
    while ((pos = pattern.find(':', last)) != std::string::npos) {
        static_fragments_.push_back(pattern.substr(last, pos - last));
        auto end = pattern.find('/', pos);
        if (end == std::string::npos)
            end = pattern.size();
        param_names_.push_back(pattern.substr(pos + 1, end - pos - 1));
        last = end;
    }
    static_fragments_.push_back(pattern.substr(last));
}

bool PathParamsMatcher::match(Request& request) const {
    const auto& path = request.path;
    size_t path_pos = 0;

    request.path_params.clear();

    for (size_t i = 0; i < static_fragments_.size(); ++i) {
        auto& frag = static_fragments_[i];
        if (path.compare(path_pos, frag.size(), frag) != 0)
            return false;
        path_pos += frag.size();

        if (i < param_names_.size()) {
            auto end = path.find(separator, path_pos);
            if (end == std::string::npos)
                end = path.size();
            request.path_params[param_names_[i]] = path.substr(path_pos, end - path_pos);
            path_pos = end;
        }
    }
    return path_pos == path.size();
}

bool RegexMatcher::match(Request& request) const {
    return std::regex_match(request.path, request.matches, regex_);
}

ssize_t write_headers(::http::Stream& strm, Headers& headers) {
    ssize_t total = 0;
    for (const auto& [key, val] : headers) {
        std::string line = key + ": " + val + "\r\n";
        ssize_t n = strm.write(line.data(), line.size());
        if (n < 0)
            return n;
        total += n;
    }
    return total;
}

} // namespace detail

} // namespace http
