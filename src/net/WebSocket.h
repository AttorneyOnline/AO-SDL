#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <kissnet.hpp>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> HTTPHeaders;

class WebSocketException : public std::runtime_error {
  public:
    explicit WebSocketException(const std::string& message) : std::runtime_error(message){};
};

class HTTPResponse {
  public:
    struct StatusLine {
        std::string http_version;
        int status_code;
        std::string status_reason;
    };

    HTTPResponse(StatusLine status_line, HTTPHeaders headers);
    std::string get_header(std::string header) const;
    StatusLine get_status() const;

  private:
    StatusLine status_line;
    HTTPHeaders headers;
};

class WebSocket {
  public:
    enum Opcode { CONTIUNUATION = 0x00, TEXT = 0x01, BINARY = 0x02, CLOSE = 0x08, PING = 0x09, PONG = 0x0A };

    struct WebSocketFrame {
        bool complete;
        std::vector<uint8_t> bytes;

        bool fin;
        uint8_t rsv;
        Opcode opcode;

        bool mask;
        uint8_t len_code;
        uint64_t len;

        uint32_t mask_key;

        std::vector<uint8_t> data;

        std::vector<uint8_t> serialize();
    };

    /**
     * @brief Construct a new WebSocket client for the specified host/port.
     */
    WebSocket(const std::string& host, uint16_t port);

    /**
     * @brief Add or overwrite an HTTP header to be sent during the handshake.
     */
    void set_header(const std::string& header, const std::string& value);

    /**
     * @brief Connect to the server with the default endpoint ("/").
     */
    void connect();

    /**
     * @brief Connect to the server using a custom endpoint (e.g., "/chat").
     */
    void connect(const std::string& endpoint);

    /**
     * @brief Read any available WebSocket frames
     * @return A WebSocketFrame containing the parsed message data (if any).
     */
    std::vector<WebSocketFrame> read();

    /**
     * @brief Write a WebSocket frame
     * @param data_bytes The data to send (unmasked).
     */
    void write(std::span<const uint8_t> data_bytes);

  private:
    /* ==================== 1) Raw I/O ==================== */

    /**
     * @brief Read raw bytes directly from the underlying TCP socket.
     * @return A vector of bytes that have been read.
     * @throws WebSocketException if the socket read is invalid.
     */
    std::vector<uint8_t> read_raw();

    /**
     * @brief Write raw bytes directly to the underlying TCP socket (no WebSocket framing).
     * @param data_bytes Bytes to be written to the socket.
     */
    void write_raw(std::span<const uint8_t> data_bytes);

    /* ==================== 2) Handshake Methods ==================== */

    /**
     * @brief Generate a random 16-byte mask key for the Sec-WebSocket-Key header.
     */
    void generate_mask();

    /**
     * @brief Read and parse the serverÅfs HTTP handshake response.
     * @return An HTTPResponse containing status line and headers.
     */
    HTTPResponse read_handshake();

    /**
     * @brief Validate the HTTP handshake response against WebSocket rules.
     * @param response The HTTPResponse to validate.
     * @return true if validation passes; throws WebSocketException otherwise.
     */
    bool validate_handshake(const HTTPResponse& response);

    /* ==================== 3) Header / Line Parsing Helpers ==================== */

    /**
     * @brief Split raw data into lines by "\r\n".
     * @param input The span of bytes to parse.
     * @return A list of lines (strings).
     */
    std::vector<std::string> get_lines(std::span<uint8_t> input);

    /**
     * @brief Trim leading and trailing whitespace from a string.
     */
    std::string trim(const std::string& str);

    /**
     * @brief Collapse internal whitespace to a single space (per RFC 2616 LWS rules).
     */
    std::string collapse_lws(const std::string& str);

    /**
     * @brief Parse a single "Header: Value" line into a (fieldName, fieldValue) pair.
     */
    std::pair<std::string, std::string> parse_http_header(const std::string& header);

    /**
     * @brief Parse the first line of the HTTP response (Status-Line).
     */
    HTTPResponse::StatusLine parse_status_line(const std::string& line);

    /* ==================== 4) Utility & Internal Data ==================== */

    /**
     * @brief Case-insensitive string compare for header fields, etc.
     */
    bool case_insensitive_compare(const std::string& a, const std::string& b);

    /**
     * @brief Buffer size for raw reads from the TCP socket.
     */
    static constexpr size_t RECV_BUF_SIZE = 8192;

    // Underlying TCP socket
    kissnet::tcp_socket tcp_sock;

    // HTTP headers to send during the handshake
    HTTPHeaders http_headers;

    // 16-byte random mask key used for Sec-WebSocket-Key
    std::array<uint8_t, 16> sec_ws_key;

    // Stores leftover bytes (e.g., partial lines) between read calls
    std::vector<uint8_t> extra_data;

    // Connection state flags
    bool ready;
    bool connecting;
};

#endif