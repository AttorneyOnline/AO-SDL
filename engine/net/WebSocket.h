/**
 * @file WebSocket.h
 * @brief RFC 6455 WebSocket client implementation.
 *
 * Provides a WebSocket client that performs the opening handshake over an
 * ITcpSocket and then supports reading/writing framed WebSocket messages.
 */
#pragma once

#include "net/ITcpSocket.h"

#include <array>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

// Define byte swap macro based on compiler
#if defined(__GNUC__) || defined(__clang__)
#define BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#include <cstdlib>
#define BSWAP64(x) _byteswap_uint64(x)
#else
static inline uint64_t BSWAP64(uint64_t x) {
    return ((x & 0x00000000000000FFULL) << 56) | ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x0000000000FF0000ULL) << 24) | ((x & 0x00000000FF000000ULL) << 8) |
           ((x & 0x000000FF00000000ULL) >> 8) | ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x00FF000000000000ULL) >> 40) | ((x & 0xFF00000000000000ULL) >> 56);
}
#endif

/**
 * @brief Case-insensitive comparator for HTTP header map keys.
 */
struct CaseInsensitiveCompare {
    /**
     * @brief Compare two strings lexicographically, ignoring case.
     * @param a First string.
     * @param b Second string.
     * @return True if @p a is less than @p b (case-insensitive).
     */
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); });
    }
};

/** @brief Map of HTTP headers with case-insensitive key comparison. */
typedef std::map<std::string, std::string, CaseInsensitiveCompare> HTTPHeaders;

/**
 * @brief Parsed HTTP response (status line + headers).
 *
 * Used internally during the WebSocket opening handshake to validate the
 * server's HTTP 101 Switching Protocols response.
 */
class HTTPResponse {
  public:
    /** @brief Parsed HTTP status line components. */
    struct StatusLine {
        std::string http_version; /**< HTTP version string (e.g. "HTTP/1.1"). */
        int status_code;          /**< Numeric status code (e.g. 101). */
        std::string status_reason; /**< Reason phrase (e.g. "Switching Protocols"). */
    };

    /**
     * @brief Construct an HTTPResponse from a parsed status line and headers.
     * @param status_line The parsed status line.
     * @param headers The parsed HTTP headers.
     */
    HTTPResponse(StatusLine status_line, HTTPHeaders headers);

    /**
     * @brief Retrieve a header value by name (case-insensitive lookup).
     * @param header The header name.
     * @return The header value, or an empty string if not present.
     */
    std::string get_header(std::string header) const;

    /**
     * @brief Get the parsed status line.
     * @return The StatusLine struct.
     */
    StatusLine get_status() const;

  private:
    StatusLine status_line;
    HTTPHeaders headers;
};

/**
 * @brief RFC 6455 WebSocket client.
 *
 * Performs the opening handshake over an ITcpSocket, then supports sending
 * and receiving framed WebSocket messages. The socket is owned via
 * unique_ptr and may be injected for testing.
 *
 * @note connect() is a blocking call -- it performs DNS resolution, TCP
 *       connection, and the HTTP upgrade handshake synchronously.
 */
class WebSocket {
  public:
    /** @brief WebSocket frame opcodes per RFC 6455. */
    enum Opcode { CONTINUATION = 0x00, TEXT = 0x01, BINARY = 0x02, CLOSE = 0x08, PING = 0x09, PONG = 0x0A };

    /**
     * @brief A single WebSocket frame (possibly partial during parsing).
     */
    struct WebSocketFrame {
        bool complete;             /**< True if the frame has been fully received. */
        std::vector<uint8_t> bytes; /**< Raw bytes of the frame as received. */

        bool fin;       /**< FIN bit: true if this is the final fragment. */
        uint8_t rsv;    /**< RSV bits (reserved, should be 0). */
        Opcode opcode;  /**< Frame opcode. */

        bool mask;          /**< True if the payload is masked. */
        uint8_t len_code;   /**< Raw length code from the frame header. */
        uint64_t len;       /**< Actual payload length after decoding. */

        uint32_t mask_key;  /**< Masking key (only valid if mask is true). */

        std::vector<uint8_t> data; /**< Unmasked payload data. */

        /**
         * @brief Serialize this frame into a byte buffer suitable for sending.
         * @return The serialized frame bytes.
         */
        std::vector<uint8_t> serialize();
    };

    /**
     * @brief Construct a WebSocket client using the default kissnet TCP socket.
     * @param host The remote hostname or IP address.
     * @param port The remote TCP port number.
     */
    WebSocket(const std::string& host, uint16_t port);

    /**
     * @brief Construct a WebSocket client with an injected socket (for testing).
     * @param host The remote hostname (used in the HTTP Host header).
     * @param port The remote TCP port number.
     * @param socket An ITcpSocket implementation to use for transport.
     */
    WebSocket(const std::string& host, uint16_t port, std::unique_ptr<ITcpSocket> socket);

    /** @brief Move constructor. */
    WebSocket(WebSocket&&) = default;
    /** @brief Move assignment operator. */
    WebSocket& operator=(WebSocket&&) = default;

    /**
     * @brief Set a custom HTTP header to include in the opening handshake.
     * @param header The header name.
     * @param value The header value.
     */
    void set_header(const std::string& header, const std::string& value);

    /**
     * @brief Perform the WebSocket opening handshake to the root endpoint "/".
     *
     * This is a blocking call. It connects the underlying TCP socket, sends
     * the HTTP upgrade request, and validates the server response.
     *
     * @throws WebSocketException on connection or handshake failure.
     */
    void connect();

    /**
     * @brief Perform the WebSocket opening handshake to a specific endpoint.
     * @param endpoint The URI path to connect to (e.g. "/ws").
     *
     * This is a blocking call.
     *
     * @throws WebSocketException on connection or handshake failure.
     */
    void connect(const std::string& endpoint);

    /**
     * @brief Read any complete WebSocket frames available on the socket.
     * @return A vector of fully received WebSocketFrame objects. May be empty
     *         if no complete frames are available.
     */
    std::vector<WebSocketFrame> read();

    /**
     * @brief Send a binary WebSocket message.
     * @param data_bytes The payload bytes to send.
     */
    void write(std::span<const uint8_t> data_bytes);

    /**
     * @brief Check whether the WebSocket connection is established and ready.
     * @return True if the handshake completed successfully and the connection
     *         has not been closed.
     */
    bool is_connected();

  private:
    std::vector<uint8_t> read_raw();
    void write_raw(std::span<const uint8_t> data_bytes);

    void generate_mask();
    HTTPResponse read_handshake();
    bool validate_handshake(const HTTPResponse& response);

    std::vector<std::string> get_lines(std::span<uint8_t> input);
    std::string trim(const std::string& str);
    std::string collapse_lws(const std::string& str);
    std::pair<std::string, std::string> parse_http_header(const std::string& header);
    HTTPResponse::StatusLine parse_status_line(const std::string& line);

    bool case_insensitive_equal(const std::string& a, const std::string& b);

    static inline uint64_t ntohll(uint64_t net_value);
    static inline uint64_t htonll(uint64_t host_value);

    std::unique_ptr<ITcpSocket> socket; /**< Underlying TCP transport. */

    HTTPHeaders http_headers;           /**< Custom headers for the handshake. */
    std::array<uint8_t, 16> sec_ws_key; /**< Random key for Sec-WebSocket-Key. */
    std::vector<uint8_t> extra_data;    /**< Leftover bytes from handshake read. */

    bool ready;      /**< True after a successful handshake. */
    bool connecting; /**< True while the handshake is in progress. */
};
