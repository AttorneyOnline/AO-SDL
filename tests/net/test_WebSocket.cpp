#include "net/WebSocket.h"
#include "MockTcpSocket.h"
#include "utils/Base64.h"

#include <sha1.h>

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static WebSocket::WebSocketFrame make_frame(bool fin, uint8_t rsv,
                                            WebSocket::Opcode opcode,
                                            bool mask, uint32_t mask_key,
                                            uint8_t len_code, uint64_t len,
                                            std::vector<uint8_t> data) {
    WebSocket::WebSocketFrame f;
    f.complete  = true;
    f.fin       = fin;
    f.rsv       = rsv;
    f.opcode    = opcode;
    f.mask      = mask;
    f.mask_key  = mask_key;
    f.len_code  = len_code;
    f.len       = len;
    f.data      = std::move(data);
    return f;
}

// Compute the Sec-WebSocket-Accept value for a given base64 key string,
// mirroring the logic in WebSocket::validate_handshake().
static std::string compute_ws_accept(const std::string& ws_key) {
    static const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    SHA1 sha;
    sha.update(ws_key + magic);
    std::string hash_bytes = sha.final();
    return Base64::encode(std::vector<uint8_t>(hash_bytes.begin(), hash_bytes.end()));
}

// Extract the value of a header from a raw HTTP request string.
static std::string extract_header(const std::string& request, const std::string& name) {
    std::string needle = name + ": ";
    auto pos = request.find(needle);
    if (pos == std::string::npos) return {};
    auto start = pos + needle.size();
    auto end   = request.find("\r\n", start);
    return request.substr(start, end - start);
}

// Build a WebSocket with an injected mock socket.
static std::pair<WebSocket, MockTcpSocket*> make_ws() {
    auto* raw = new MockTcpSocket();
    WebSocket ws("localhost", 0, std::unique_ptr<ITcpSocket>(raw));
    return {std::move(ws), raw};
}

// ---------------------------------------------------------------------------
// WebSocketFrame::serialize — header byte 0 (FIN | RSV | opcode)
// ---------------------------------------------------------------------------

TEST(WebSocketFrameSerialize, FirstByteFinAndTextOpcode) {
    auto f = make_frame(true, 0, WebSocket::TEXT, false, 0, 1, 1, {0x41});
    auto bytes = f.serialize();
    ASSERT_GE(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], 0x81u);  // FIN=1, opcode=TEXT(0x01)
}

TEST(WebSocketFrameSerialize, FinFalseGivesZeroHighBit) {
    auto f = make_frame(false, 0, WebSocket::CONTINUATION, false, 0, 0, 0, {});
    auto bytes = f.serialize();
    EXPECT_EQ(bytes[0] & 0x80u, 0x00u);
}

TEST(WebSocketFrameSerialize, RsvBitsEncoded) {
    auto f = make_frame(false, 0x05, WebSocket::TEXT, false, 0, 1, 1, {0xAA});
    auto bytes = f.serialize();
    EXPECT_EQ((bytes[0] >> 4) & 0x07u, 0x05u);
}

TEST(WebSocketFrameSerialize, PingOpcodeEncoded) {
    auto f = make_frame(true, 0, WebSocket::PING, false, 0, 0, 0, {});
    EXPECT_EQ(f.serialize()[0] & 0x0Fu, 0x09u);
}

TEST(WebSocketFrameSerialize, BinaryOpcodeEncoded) {
    auto f = make_frame(true, 0, WebSocket::BINARY, false, 0, 2, 2, {0x01, 0x02});
    EXPECT_EQ(f.serialize()[0] & 0x0Fu, 0x02u);
}

TEST(WebSocketFrameSerialize, CloseOpcodeEncoded) {
    auto f = make_frame(true, 0, WebSocket::CLOSE, false, 0, 0, 0, {});
    EXPECT_EQ(f.serialize()[0] & 0x0Fu, 0x08u);
}

// ---------------------------------------------------------------------------
// WebSocketFrame::serialize — byte 1 and payload length
// ---------------------------------------------------------------------------

TEST(WebSocketFrameSerialize, SmallUnmaskedLengthInByte1) {
    auto f = make_frame(true, 0, WebSocket::TEXT, false, 0, 5, 5,
                        std::vector<uint8_t>(5, 0x00));
    auto bytes = f.serialize();
    EXPECT_EQ(bytes[1] & 0x80u, 0x00u);  // MASK bit clear
    EXPECT_EQ(bytes[1] & 0x7Fu, 5u);
}

TEST(WebSocketFrameSerialize, ZeroLengthFrame) {
    auto f = make_frame(true, 0, WebSocket::TEXT, false, 0, 0, 0, {});
    auto bytes = f.serialize();
    EXPECT_EQ(bytes.size(), 2u);
    EXPECT_EQ(bytes[1] & 0x7Fu, 0u);
}

TEST(WebSocketFrameSerialize, MaxSmallLength125) {
    std::vector<uint8_t> data(125, 0xAB);
    auto f = make_frame(true, 0, WebSocket::BINARY, false, 0, 125, 125, data);
    auto bytes = f.serialize();
    EXPECT_EQ(bytes[1] & 0x7Fu, 125u);
    EXPECT_EQ(bytes.size(), 2u + 125u);
}

TEST(WebSocketFrameSerialize, MediumLengthLen126Field) {
    constexpr uint64_t PAYLOAD = 300;
    auto f = make_frame(true, 0, WebSocket::BINARY, false, 0, 126, PAYLOAD,
                        std::vector<uint8_t>(PAYLOAD, 0x00));
    auto bytes = f.serialize();
    EXPECT_EQ(bytes[1] & 0x7Fu, 126u);
    EXPECT_EQ(bytes[2], 0x01u);  // big-endian 300 = 0x012C
    EXPECT_EQ(bytes[3], 0x2Cu);
    EXPECT_EQ(bytes.size(), 2u + 2u + PAYLOAD);
}

TEST(WebSocketFrameSerialize, LargeLengthLen127Field) {
    constexpr uint64_t PAYLOAD = 70000;  // 0x00000000_00011170
    auto f = make_frame(true, 0, WebSocket::BINARY, false, 0, 127, PAYLOAD,
                        std::vector<uint8_t>(PAYLOAD, 0x00));
    auto bytes = f.serialize();
    EXPECT_EQ(bytes[1] & 0x7Fu, 127u);
    EXPECT_EQ(bytes[7], 0x01u);
    EXPECT_EQ(bytes[8], 0x11u);
    EXPECT_EQ(bytes[9], 0x70u);
    EXPECT_EQ(bytes.size(), 2u + 8u + PAYLOAD);
}

TEST(WebSocketFrameSerialize, UnmaskedPayloadPassthrough) {
    std::vector<uint8_t> data = {0x41, 0x4F, 0x32};
    auto f = make_frame(true, 0, WebSocket::TEXT, false, 0, 3, 3, data);
    auto bytes = f.serialize();
    ASSERT_EQ(bytes.size(), 5u);
    EXPECT_EQ(bytes[2], 0x41u);
    EXPECT_EQ(bytes[3], 0x4Fu);
    EXPECT_EQ(bytes[4], 0x32u);
}

// ---------------------------------------------------------------------------
// WebSocketFrame::serialize — masking
// ---------------------------------------------------------------------------

TEST(WebSocketFrameSerialize, MaskBitSetInByte1) {
    auto f = make_frame(true, 0, WebSocket::TEXT, true, 0x01020304, 2, 2, {'A', 'B'});
    EXPECT_NE(f.serialize()[1] & 0x80u, 0u);
}

TEST(WebSocketFrameSerialize, MaskKeyWrittenInBigEndian) {
    auto f = make_frame(true, 0, WebSocket::TEXT, true, 0xDEADBEEFu, 0, 0, {});
    auto bytes = f.serialize();
    ASSERT_GE(bytes.size(), 6u);
    EXPECT_EQ(bytes[2], 0xDEu);
    EXPECT_EQ(bytes[3], 0xADu);
    EXPECT_EQ(bytes[4], 0xBEu);
    EXPECT_EQ(bytes[5], 0xEFu);
}

TEST(WebSocketFrameSerialize, PayloadXoredWithMaskKey) {
    // mask_key=0xDEADBEEF: idx0 mask byte = 0xDE, idx1 = 0xAD
    auto f = make_frame(true, 0, WebSocket::TEXT, true, 0xDEADBEEFu, 2, 2, {0x41, 0x42});
    auto bytes = f.serialize();
    ASSERT_EQ(bytes.size(), 8u);
    EXPECT_EQ(bytes[6], uint8_t(0x41 ^ 0xDE));
    EXPECT_EQ(bytes[7], uint8_t(0x42 ^ 0xAD));
}

TEST(WebSocketFrameSerialize, MaskWrapsEveryFourBytes) {
    auto f = make_frame(true, 0, WebSocket::TEXT, true, 0x01020304u, 5, 5,
                        {0x00, 0x00, 0x00, 0x00, 0x00});
    auto bytes = f.serialize();
    ASSERT_EQ(bytes.size(), 2u + 4u + 5u);
    EXPECT_EQ(bytes[6],  0x01u);
    EXPECT_EQ(bytes[7],  0x02u);
    EXPECT_EQ(bytes[8],  0x03u);
    EXPECT_EQ(bytes[9],  0x04u);
    EXPECT_EQ(bytes[10], 0x01u);  // wraps back to byte 0 of mask
}

// ---------------------------------------------------------------------------
// HTTPResponse
// ---------------------------------------------------------------------------

TEST(HTTPResponse, GetStatusCodeAndReason) {
    HTTPResponse resp({"HTTP/1.1", 101, "Switching Protocols"}, {});
    EXPECT_EQ(resp.get_status().status_code, 101);
    EXPECT_EQ(resp.get_status().status_reason, "Switching Protocols");
    EXPECT_EQ(resp.get_status().http_version, "HTTP/1.1");
}

TEST(HTTPResponse, MissingHeaderReturnsEmpty) {
    HTTPResponse resp({"HTTP/1.1", 101, "Switching Protocols"}, {});
    EXPECT_EQ(resp.get_header("X-Nonexistent"), "");
}

TEST(HTTPResponse, HeaderLookupIsCaseInsensitive) {
    HTTPHeaders h;
    h["sec-websocket-accept"] = "value";
    HTTPResponse resp({"HTTP/1.1", 101, "Switching Protocols"}, h);
    EXPECT_EQ(resp.get_header("Sec-WebSocket-Accept"), "value");
    EXPECT_EQ(resp.get_header("SEC-WEBSOCKET-ACCEPT"), "value");
}

// ---------------------------------------------------------------------------
// WebSocket state
// ---------------------------------------------------------------------------

TEST(WebSocket, IsConnectedFalseBeforeConnect) {
    WebSocket ws("127.0.0.1", 27016);
    EXPECT_FALSE(ws.is_connected());
}

TEST(WebSocket, IsConnectedFalseWithMockBeforeConnect) {
    auto [ws, _] = make_ws();
    EXPECT_FALSE(ws.is_connected());
}

// ---------------------------------------------------------------------------
// WebSocket::read — frame parsing via mock
// ---------------------------------------------------------------------------

TEST(WebSocketRead, ReturnsEmptyWhenNoData) {
    auto [ws, mock] = make_ws();
    // No data fed — mock returns empty vector, read() returns empty list.
    EXPECT_TRUE(ws.read().empty());
}

TEST(WebSocketRead, ParsesSmallTextFrame) {
    auto [ws, mock] = make_ws();
    mock->feed({0x81, 0x02, 'h', 'i'});  // FIN+TEXT, len=2

    auto frames = ws.read();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, WebSocket::TEXT);
    EXPECT_TRUE(frames[0].fin);
    EXPECT_EQ(frames[0].len, 2u);
    std::string got(frames[0].data.begin(), frames[0].data.end());
    EXPECT_EQ(got, "hi");
}

TEST(WebSocketRead, ParsesBinaryFrame) {
    auto [ws, mock] = make_ws();
    mock->feed({0x82, 0x03, 0x01, 0x02, 0x03});  // FIN+BINARY, len=3

    auto frames = ws.read();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].opcode, WebSocket::BINARY);
    EXPECT_EQ(frames[0].data, (std::vector<uint8_t>{0x01, 0x02, 0x03}));
}

TEST(WebSocketRead, ParsesExtended16BitLength) {
    // FIN+TEXT, len_code=0x7E (126), 2-byte len = 0x0003 (big-endian), then 3 bytes
    std::vector<uint8_t> frame = {0x81, 0x7E, 0x00, 0x03, 'a', 'b', 'c'};
    auto [ws, mock] = make_ws();
    mock->feed(frame);

    auto frames = ws.read();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].len, 3u);
    std::string got(frames[0].data.begin(), frames[0].data.end());
    EXPECT_EQ(got, "abc");
}

TEST(WebSocketRead, ParsesTwoFramesFromOneChunk) {
    auto [ws, mock] = make_ws();
    // Two complete frames in a single recv chunk.
    mock->feed({0x81, 0x01, 'A',   // frame 1: "A"
                0x81, 0x01, 'B'}); // frame 2: "B"

    auto frames = ws.read();
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[0].data[0], uint8_t('A'));
    EXPECT_EQ(frames[1].data[0], uint8_t('B'));
}

TEST(WebSocketRead, BuffersIncompleteFrameForNextRead) {
    auto [ws, mock] = make_ws();
    // First read: only the header, no payload yet.
    mock->feed({0x81, 0x02});

    EXPECT_TRUE(ws.read().empty());

    // Second read: payload arrives.
    mock->feed({'h', 'i'});
    auto frames = ws.read();
    ASSERT_EQ(frames.size(), 1u);
    std::string got(frames[0].data.begin(), frames[0].data.end());
    EXPECT_EQ(got, "hi");
}

TEST(WebSocketRead, ThrowsOnServerMaskedFrame) {
    auto [ws, mock] = make_ws();
    // Byte 1 = 0x82 → MASK bit set, len=2
    mock->feed({0x81, 0x82, 0xDE, 0xAD, 0xBE, 0xEF, 'h', 'i'});
    EXPECT_THROW(ws.read(), WebSocketException);
}

TEST(WebSocketRead, PingFrameAutoRepliesWithPong) {
    auto [ws, mock] = make_ws();
    mock->feed({0x89, 0x00});  // FIN+PING, empty payload

    auto frames = ws.read();
    // PING is consumed internally; no user-visible frame.
    EXPECT_TRUE(frames.empty());
    // A PONG should have been sent.
    const auto& sent = mock->sent();
    ASSERT_GE(sent.size(), 2u);
    EXPECT_EQ(sent[0] & 0x0Fu, 0x0Au);  // opcode = PONG
    EXPECT_NE(sent[1] & 0x80u, 0u);     // MASK bit set (client always masks)
}

// ---------------------------------------------------------------------------
// WebSocket::write — frame encoding via mock
// ---------------------------------------------------------------------------

TEST(WebSocketWrite, SendsTextFrameWithCorrectFirstByte) {
    auto [ws, mock] = make_ws();
    std::string payload = "hello";
    ws.write(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    const auto& sent = mock->sent();
    ASSERT_GE(sent.size(), 2u);
    EXPECT_EQ(sent[0], 0x81u);  // FIN + TEXT
}

TEST(WebSocketWrite, SentFrameHasMaskBitSet) {
    auto [ws, mock] = make_ws();
    std::string payload = "hi";
    ws.write(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    EXPECT_NE(mock->sent()[1] & 0x80u, 0u);
}

TEST(WebSocketWrite, SentFramePayloadLengthIsCorrect) {
    auto [ws, mock] = make_ws();
    std::string payload = "hello";  // 5 bytes
    ws.write(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    EXPECT_EQ(mock->sent()[1] & 0x7Fu, 5u);
}

TEST(WebSocketWrite, MaskedPayloadDecodesBack) {
    // The write() method masks the payload with a random key.
    // We can recover the original by XOR-ing with the key bytes from the wire frame.
    auto [ws, mock] = make_ws();
    std::string payload = "AO2";
    ws.write(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    const auto& sent = mock->sent();
    // Layout: [0]=hdr, [1]=mask+len, [2..5]=mask key, [6..8]=masked payload
    ASSERT_EQ(sent.size(), 2u + 4u + 3u);

    uint8_t k0 = sent[2], k1 = sent[3], k2 = sent[4];
    EXPECT_EQ(char(sent[6] ^ k0), 'A');
    EXPECT_EQ(char(sent[7] ^ k1), 'O');
    EXPECT_EQ(char(sent[8] ^ k2), '2');
}

// ---------------------------------------------------------------------------
// WebSocket::connect — handshake validation via mock
// ---------------------------------------------------------------------------

// Helper: build a 101 Switching Protocols response using the Sec-WebSocket-Key
// extracted from the captured HTTP upgrade request.
static std::string make_101_response(const std::vector<uint8_t>& request_bytes) {
    std::string request(request_bytes.begin(), request_bytes.end());
    std::string ws_key = extract_header(request, "Sec-WebSocket-Key");
    std::string accept = compute_ws_accept(ws_key);
    return "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
}

TEST(WebSocketConnect, ValidHandshakeCompletesWithoutException) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;  // feed only once
        auto resp = make_101_response(sent);
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_NO_THROW(ws.connect());
    EXPECT_TRUE(ws.is_connected());
}

TEST(WebSocketConnect, IsConnectedTrueAfterValidHandshake) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;
        auto resp = make_101_response(sent);
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    ws.connect();
    EXPECT_TRUE(ws.is_connected());
}

TEST(WebSocketConnect, ThrowsOnNon101StatusCode) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>&) {
        if (mock->bytes_available()) return;
        std::string resp = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_THROW(ws.connect(), WebSocketException);
}

TEST(WebSocketConnect, ThrowsOnMissingUpgradeHeader) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;
        std::string request(sent.begin(), sent.end());
        std::string accept = compute_ws_accept(extract_header(request, "Sec-WebSocket-Key"));
        std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_THROW(ws.connect(), WebSocketException);
}

TEST(WebSocketConnect, ThrowsOnMissingConnectionHeader) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;
        std::string request(sent.begin(), sent.end());
        std::string accept = compute_ws_accept(extract_header(request, "Sec-WebSocket-Key"));
        std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_THROW(ws.connect(), WebSocketException);
}

TEST(WebSocketConnect, ThrowsOnWrongSecWebSocketAccept) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>&) {
        if (mock->bytes_available()) return;
        std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: nottherighthash==\r\n\r\n";
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_THROW(ws.connect(), WebSocketException);
}

TEST(WebSocketConnect, ThrowsOnUnwantedExtensionsHeader) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;
        std::string request(sent.begin(), sent.end());
        std::string accept = compute_ws_accept(extract_header(request, "Sec-WebSocket-Key"));
        std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
                           "Upgrade: websocket\r\n"
                           "Connection: Upgrade\r\n"
                           "Sec-WebSocket-Accept: " + accept + "\r\n"
                           "Sec-WebSocket-Extensions: permessage-deflate\r\n\r\n";
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    EXPECT_THROW(ws.connect(), WebSocketException);
}

TEST(WebSocketConnect, ThrowsIfAlreadyConnected) {
    auto [ws, mock] = make_ws();
    mock->on_send = [mock](const std::vector<uint8_t>& sent) {
        if (mock->bytes_available()) return;
        auto resp = make_101_response(sent);
        mock->feed(std::vector<uint8_t>(resp.begin(), resp.end()));
    };
    ws.connect();
    EXPECT_THROW(ws.connect(), WebSocketException);
}
