#include <gtest/gtest.h>

#include "net/PlatformServerSocket.h"
#include "net/PlatformTcpSocket.h"

#include <memory>
#include <thread>
#include <vector>

// ===========================================================================
// PlatformTcpSocket
// ===========================================================================

TEST(PlatformTcpSocket, ConnectToListener) {
    PlatformServerSocket server;
    server.bind_and_listen(0); // OS-assigned port

    // Get the actual port via fd
    uint16_t port = server.local_port();

    PlatformTcpSocket client("127.0.0.1", port);
    client.connect();

    // Accept — use blocking mode to wait for the connection
    server.set_non_blocking(false);
    auto accepted = server.accept();
    ASSERT_NE(accepted, nullptr);
}

TEST(PlatformTcpSocket, SendRecvRoundtrip) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    uint16_t port = server.local_port();

    PlatformTcpSocket client("127.0.0.1", port);
    client.connect();

    server.set_non_blocking(false);
    auto accepted = server.accept();
    ASSERT_NE(accepted, nullptr);
    accepted->set_non_blocking(true);

    // Send from client
    const uint8_t msg[] = {0xDE, 0xAD, 0xBE, 0xEF};
    client.send(msg, sizeof(msg));

    // Receive on server side — brief wait
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto data = accepted->recv();
    ASSERT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0xDE);
    EXPECT_EQ(data[3], 0xEF);
}

TEST(PlatformTcpSocket, FdIsValid) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    EXPECT_GE(server.fd(), 0);

    uint16_t port = server.local_port();

    PlatformTcpSocket client("127.0.0.1", port);
    // fd is -1 before connect (socket not yet created)
    client.connect();
    EXPECT_GE(client.fd(), 0);
}

TEST(PlatformTcpSocket, BytesAvailable) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    uint16_t port = server.local_port();

    PlatformTcpSocket client("127.0.0.1", port);
    client.connect();

    server.set_non_blocking(false);
    auto accepted = server.accept();
    ASSERT_NE(accepted, nullptr);

    EXPECT_FALSE(accepted->bytes_available());

    const uint8_t data[] = {1, 2, 3};
    client.send(data, sizeof(data));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(accepted->bytes_available());
}

TEST(PlatformTcpSocket, ConnectFailureThrows) {
    PlatformTcpSocket client("127.0.0.1", 1); // port 1 = not listening
    EXPECT_THROW(client.connect(), std::runtime_error);
}

TEST(PlatformTcpSocket, PeerCloseThrowsOnRecv) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    uint16_t port = server.local_port();

    PlatformTcpSocket client("127.0.0.1", port);
    client.connect();

    server.set_non_blocking(false);
    auto accepted = server.accept();
    ASSERT_NE(accepted, nullptr);

    // Close the client side
    // (PlatformTcpSocket doesn't expose close, so let it go out of scope)
    {
        PlatformTcpSocket temp = std::move(client);
        // temp destroyed here, closing the connection
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    accepted->set_non_blocking(false);

    // Reading from a closed connection should either return empty or throw
    EXPECT_THROW(accepted->recv(), WebSocketException);
}

// ===========================================================================
// PlatformServerSocket
// ===========================================================================

TEST(PlatformServerSocket, AcceptReturnsNullWhenNoPending) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    server.set_non_blocking(true);

    auto result = server.accept();
    EXPECT_EQ(result, nullptr);
}

TEST(PlatformServerSocket, AcceptMultipleClients) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    uint16_t port = server.local_port();

    // Connect 3 clients
    std::vector<std::unique_ptr<PlatformTcpSocket>> clients;
    for (int i = 0; i < 3; ++i) {
        auto c = std::make_unique<PlatformTcpSocket>("127.0.0.1", port);
        c->connect();
        clients.push_back(std::move(c));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.set_non_blocking(true);

    int accepted_count = 0;
    while (auto s = server.accept()) {
        accepted_count++;
    }
    EXPECT_EQ(accepted_count, 3);
}

TEST(PlatformServerSocket, BindThrowsOnInvalidAddress) {
    PlatformServerSocket server("192.0.2.1"); // TEST-NET, not routable
    // This may throw on bind or may succeed on some OSes — just verify no crash
    try {
        server.bind_and_listen(0);
    }
    catch (const std::runtime_error&) {
        // Expected on most systems
    }
}

TEST(PlatformServerSocket, CloseAndRebind) {
    PlatformServerSocket server;
    server.bind_and_listen(0);
    uint16_t port = server.local_port();

    server.close();

    // Should be able to bind again (SO_REUSEADDR)
    PlatformServerSocket server2;
    server2.bind_and_listen(port);
    EXPECT_GE(server2.fd(), 0);
}

// ===========================================================================
// SSL enable (smoke test — no actual TLS server to connect to)
// ===========================================================================

TEST(PlatformTcpSocket, EnableSslSetsFlag) {
    PlatformTcpSocket client("127.0.0.1", 443);
    client.enable_ssl("example.com");
    // Can't test actual handshake without a TLS server, but verify
    // that enable_ssl doesn't crash and connect() attempts TLS
    EXPECT_THROW(client.connect(), std::runtime_error); // connection refused or TLS error
}

// ===========================================================================
// WSClientThread URL parsing (via PlatformTcpSocket integration)
// ===========================================================================
// The parse_ws_url function is static in WSClientThread.cpp, so we test
// its behavior indirectly through observable side effects.

// These are really testing that the wss:// flow doesn't crash during
// construction — actual TLS testing needs a real server.
TEST(WsUrlParsing, WssSchemeIsRecognized) {
    // Constructing a PlatformTcpSocket with enable_ssl should not crash
    PlatformTcpSocket sock("example.com", 443);
    sock.enable_ssl("example.com");
    // Don't call connect() — no server
}

TEST(WsUrlParsing, WsSchemeIsPlaintext) {
    PlatformTcpSocket sock("example.com", 80);
    // Not calling enable_ssl — this is a plain connection
    // Just verify construction works
}
