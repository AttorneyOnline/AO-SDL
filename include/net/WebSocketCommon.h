/**
 * @file WebSocketCommon.h
 * @brief HTTP types and parsing utilities shared by WebSocket client and server.
 */
#pragma once

#include <algorithm>
#include <map>
#include <span>
#include <string>
#include <vector>

/**
 * @brief Case-insensitive comparator for HTTP header map keys.
 */
struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); });
    }
};

/** @brief Map of HTTP headers with case-insensitive key comparison. */
using HTTPHeaders = std::map<std::string, std::string, CaseInsensitiveCompare>;

/**
 * @brief Parsed HTTP response (status line + headers).
 */
class HTTPResponse {
  public:
    struct StatusLine {
        std::string http_version;
        int status_code;
        std::string status_reason;
    };

    HTTPResponse() = default;
    HTTPResponse(StatusLine status_line, HTTPHeaders headers);

    std::string get_header(std::string header) const;
    StatusLine get_status() const;

  private:
    StatusLine status_line;
    HTTPHeaders headers;
};

/**
 * @brief Parsed HTTP request line (for server-side handshake).
 */
struct HTTPRequestLine {
    std::string method;
    std::string uri;
    std::string http_version;
};

namespace ws {

/** @brief Trim leading and trailing whitespace. */
std::string trim(const std::string& str);

/** @brief Collapse linear whitespace (multiple spaces/tabs → single space). */
std::string collapse_lws(const std::string& str);

/** @brief Parse a single HTTP header line into a key-value pair. */
std::pair<std::string, std::string> parse_http_header(const std::string& header);

/** @brief Parse an HTTP status line (e.g. "HTTP/1.1 101 Switching Protocols"). */
HTTPResponse::StatusLine parse_status_line(const std::string& line);

/** @brief Parse an HTTP request line (e.g. "GET /ws HTTP/1.1"). */
HTTPRequestLine parse_request_line(const std::string& line);

/** @brief Case-insensitive string equality. */
bool case_insensitive_equal(const std::string& a, const std::string& b);

/**
 * @brief Split raw bytes on CRLF boundaries into lines.
 * @param input Raw bytes to parse.
 * @param[in,out] remainder Leftover bytes that don't end with CRLF.
 *                          Incoming remainder is prepended to input.
 * @return Vector of parsed lines (without CRLF).
 */
std::vector<std::string> get_lines(std::span<uint8_t> input, std::vector<uint8_t>& remainder);

/**
 * @brief Compute the Sec-WebSocket-Accept value for a given client key.
 *
 * Concatenates the Base64-encoded client key with the RFC 6455 magic GUID,
 * SHA-1 hashes the result, and Base64-encodes the hash.
 *
 * @param client_key_b64 The Base64-encoded Sec-WebSocket-Key from the client.
 * @return The Sec-WebSocket-Accept value to include in the server response.
 */
std::string compute_accept_key(const std::string& client_key_b64);

} // namespace ws
