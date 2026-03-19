#pragma once

#include "ITcpSocket.h"

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

struct CaseInsensitiveCompare {
    bool operator()(const std::string& a, const std::string& b) const {
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char ac, unsigned char bc) { return std::tolower(ac) < std::tolower(bc); });
    }
};

typedef std::map<std::string, std::string, CaseInsensitiveCompare> HTTPHeaders;

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
    enum Opcode { CONTINUATION = 0x00, TEXT = 0x01, BINARY = 0x02, CLOSE = 0x08, PING = 0x09, PONG = 0x0A };

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
     * @brief Construct a WebSocket client using the default kissnet TCP socket.
     */
    WebSocket(const std::string& host, uint16_t port);

    /**
     * @brief Construct a WebSocket client with an injected socket (for testing).
     */
    WebSocket(const std::string& host, uint16_t port, std::unique_ptr<ITcpSocket> socket);

    WebSocket(WebSocket&&) = default;
    WebSocket& operator=(WebSocket&&) = default;

    void set_header(const std::string& header, const std::string& value);

    void connect();
    void connect(const std::string& endpoint);

    std::vector<WebSocketFrame> read();
    void write(std::span<const uint8_t> data_bytes);

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

    std::unique_ptr<ITcpSocket> socket;

    HTTPHeaders http_headers;
    std::array<uint8_t, 16> sec_ws_key;
    std::vector<uint8_t> extra_data;

    bool ready;
    bool connecting;
};
