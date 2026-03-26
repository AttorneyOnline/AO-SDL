/**
 * @file WebSocket.h
 * @brief RFC 6455 WebSocket client implementation.
 *
 * Provides a WebSocket client that performs the opening handshake over an
 * ITcpSocket and then supports reading/writing framed WebSocket messages.
 */
#pragma once

#include "net/ITcpSocket.h"
#include "net/WebSocketCommon.h"
#include "net/WebSocketFrame.h"

#include <array>
#include <memory>
#include <span>
#include <string>
#include <vector>

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
    // Backward-compatible aliases for code that uses WebSocket::TEXT, etc.
    using Opcode = ::Opcode;
    using WebSocketFrame = ::WebSocketFrame;

    static constexpr ::Opcode CONTINUATION = ::CONTINUATION;
    static constexpr ::Opcode TEXT = ::TEXT;
    static constexpr ::Opcode BINARY = ::BINARY;
    static constexpr ::Opcode CLOSE = ::CLOSE;
    static constexpr ::Opcode PING = ::PING;
    static constexpr ::Opcode PONG = ::PONG;

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
     * @throws WebSocketException on connection or handshake failure.
     */
    void connect();

    /**
     * @brief Perform the WebSocket opening handshake to a specific endpoint.
     * @param endpoint The URI path to connect to (e.g. "/ws").
     * @throws WebSocketException on connection or handshake failure.
     */
    void connect(const std::string& endpoint);

    /**
     * @brief Read any complete WebSocket frames available on the socket.
     * @return A vector of fully received WebSocketFrame objects. May be empty
     *         if no complete frames are available.
     */
    std::vector<::WebSocketFrame> read();

    /**
     * @brief Send a binary WebSocket message.
     * @param data_bytes The payload bytes to send.
     */
    void write(std::span<const uint8_t> data_bytes);

    /**
     * @brief Initiate a graceful close with status code 1000 (Normal Closure).
     */
    void close();

    /**
     * @brief Initiate a graceful close with a specific status code and reason.
     * @param code   The close status code (RFC 6455 §7.4).
     * @param reason Optional human-readable reason (UTF-8, max 123 bytes).
     */
    void close(uint16_t code, const std::string& reason = "");

    /**
     * @brief Check whether the WebSocket connection is established and ready.
     * @return True if the handshake completed successfully and the connection
     *         has not been closed.
     */
    bool is_connected();

  private:
    void send_close(uint16_t code, const std::string& reason);
    std::vector<uint8_t> read_raw();
    void write_raw(std::span<const uint8_t> data_bytes);

    void generate_mask();
    HTTPResponse read_handshake();
    bool validate_handshake(const HTTPResponse& response);

    std::unique_ptr<ITcpSocket> socket; /**< Underlying TCP transport. */

    HTTPHeaders http_headers;           /**< Custom headers for the handshake. */
    std::array<uint8_t, 16> sec_ws_key; /**< Random key for Sec-WebSocket-Key. */
    std::vector<uint8_t> extra_data;    /**< Leftover bytes from handshake read. */

    bool ready;      /**< True after a successful handshake. */
    bool connecting; /**< True while the handshake is in progress. */

    // Continuation frame accumulation (RFC 6455 §5.4)
    std::vector<uint8_t> fragment_buf_; /**< Accumulated payload across fragments. */
    ::Opcode fragment_opcode_ = TEXT;   /**< Opcode from the first fragment. */
    bool in_fragment_ = false;          /**< True while accumulating fragments. */
};
