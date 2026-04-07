#include <gtest/gtest.h>

#include "platform/Socket.h"

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#endif

using namespace platform;

// -- Construction and validity ------------------------------------------------

TEST(PlatformSocket, DefaultConstructedIsInvalid) {
    Socket s;
    EXPECT_FALSE(s.valid());
    EXPECT_EQ(s.fd(), -1);
    EXPECT_FALSE(s.is_ssl());
}

TEST(PlatformSocket, MoveConstructTransfersOwnership) {
    auto s1 = tcp_create();
    ASSERT_TRUE(s1.valid());
    int fd = s1.fd();

    Socket s2 = std::move(s1);
    EXPECT_FALSE(s1.valid());
    EXPECT_TRUE(s2.valid());
    EXPECT_EQ(s2.fd(), fd);
}

TEST(PlatformSocket, MoveAssignTransfersOwnership) {
    auto s1 = tcp_create();
    ASSERT_TRUE(s1.valid());
    int fd = s1.fd();

    Socket s2;
    s2 = std::move(s1);
    EXPECT_FALSE(s1.valid());
    EXPECT_TRUE(s2.valid());
    EXPECT_EQ(s2.fd(), fd);
}

TEST(PlatformSocket, CloseInvalidatesSocket) {
    auto s = tcp_create();
    ASSERT_TRUE(s.valid());
    s.close();
    EXPECT_FALSE(s.valid());
    EXPECT_EQ(s.fd(), -1);
}

TEST(PlatformSocket, DoubleCloseIsSafe) {
    auto s = tcp_create();
    s.close();
    s.close(); // should not crash
    EXPECT_FALSE(s.valid());
}

// -- TCP listen + accept + connect (loopback) ---------------------------------

TEST(PlatformSocket, ListenAcceptConnect) {
    auto listener = tcp_listen("127.0.0.1", 0); // port 0 = OS-assigned
    ASSERT_TRUE(listener.valid());

    // Get the assigned port
    uint16_t port = listener.local_port();
    ASSERT_GT(port, 0);

    listener.set_non_blocking(true);

    // Connect from a client thread
    std::thread client_thread([port] {
        auto client = tcp_connect("127.0.0.1", port);
        EXPECT_TRUE(client.valid());
        const char* msg = "hello";
        client.send(msg, strlen(msg));
    });

    // Accept with a brief busy-wait (non-blocking listener)
    Socket accepted;
    for (int i = 0; i < 100 && !accepted.valid(); ++i) {
        std::string remote_addr;
        uint16_t remote_port = 0;
        accepted = tcp_accept(listener, remote_addr, remote_port);
        if (!accepted.valid())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_TRUE(accepted.valid());

    // Read the message
    char buf[64] = {};
    // Brief wait for data
    accepted.set_non_blocking(false);
    ssize_t n = accepted.recv(buf, sizeof(buf));
    EXPECT_EQ(n, 5);
    EXPECT_STREQ(buf, "hello");

    client_thread.join();
}

TEST(PlatformSocket, AcceptReturnsInvalidWhenNoPending) {
    auto listener = tcp_listen("127.0.0.1", 0);
    listener.set_non_blocking(true);

    std::string addr;
    uint16_t port = 0;
    auto result = tcp_accept(listener, addr, port);
    EXPECT_FALSE(result.valid());
}

// -- Send / recv roundtrip ----------------------------------------------------

TEST(PlatformSocket, SendRecvRoundtrip) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread sender([port] {
        auto s = tcp_connect("127.0.0.1", port);
        std::string data(4096, 'X'); // larger than typical kernel buffer splits
        s.send(data.data(), data.size());
        s.shutdown();
    });

    std::string remote;
    uint16_t rport = 0;
    listener.set_non_blocking(false);
    auto accepted = tcp_accept(listener, remote, rport);
    ASSERT_TRUE(accepted.valid());

    std::string received;
    char buf[1024];
    while (true) {
        ssize_t n = accepted.recv(buf, sizeof(buf));
        if (n <= 0)
            break;
        received.append(buf, static_cast<size_t>(n));
    }

    EXPECT_EQ(received.size(), 4096u);
    EXPECT_EQ(received, std::string(4096, 'X'));

    sender.join();
}

// -- Socket options -----------------------------------------------------------

TEST(PlatformSocket, SetOptionsDoNotCrash) {
    auto s = tcp_create();
    ASSERT_TRUE(s.valid());
    s.set_non_blocking(true);
    s.set_non_blocking(false);
    s.set_reuse_addr(true);
    s.set_tcp_nodelay(true);
}

TEST(PlatformSocket, BytesAvailableOnFreshSocket) {
    auto s = tcp_create();
    EXPECT_FALSE(s.bytes_available());
}

// -- Connect failure ----------------------------------------------------------

TEST(PlatformSocket, ConnectToClosedPortThrows) {
    // Port 1 is almost certainly not listening
    EXPECT_THROW(tcp_connect("127.0.0.1", 1), std::runtime_error);
}

TEST(PlatformSocket, ConnectToUnresolvableHostThrows) {
    EXPECT_THROW(tcp_connect("this.host.does.not.exist.invalid", 80), std::runtime_error);
}

// -- Bidirectional simultaneous I/O -------------------------------------------

TEST(PlatformSocket, BidirectionalIO) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();

    std::thread peer([port] {
        auto s = tcp_connect("127.0.0.1", port);
        s.send("ping", 4);
        char buf[5] = {};
        ssize_t n = s.recv(buf, 4);
        EXPECT_EQ(n, 4);
        EXPECT_EQ(std::string(buf, 4), "pong");
    });

    listener.set_non_blocking(false);
    std::string remote;
    uint16_t rport = 0;
    auto accepted = tcp_accept(listener, remote, rport);

    char buf[5] = {};
    ssize_t n = accepted.recv(buf, 4);
    EXPECT_EQ(n, 4);
    EXPECT_EQ(std::string(buf, 4), "ping");
    accepted.send("pong", 4);

    peer.join();
}

// -- Connection timeout -------------------------------------------------------

TEST(PlatformSocket, TcpConnectWithTimeoutSucceeds) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();
    listener.set_non_blocking(false);

    std::thread server([&] {
        std::string addr;
        uint16_t rp;
        auto conn = tcp_accept(listener, addr, rp);
        // just accept and let it close
    });

    auto sock = tcp_connect("127.0.0.1", port, 5000); // 5s timeout — plenty for loopback
    EXPECT_TRUE(sock.valid());

    server.join();
}

TEST(PlatformSocket, TcpConnectWithTimeoutTimesOut) {
    // Connect to a non-listening port with a very short timeout.
    // Port 1 is almost certainly not listening on loopback.
    auto start = std::chrono::steady_clock::now();
    EXPECT_THROW(tcp_connect("127.0.0.1", 1, 100), std::runtime_error);
    auto elapsed = std::chrono::steady_clock::now() - start;
    // Should complete within 2 seconds (the 100ms timeout + overhead)
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 2);
}

// -- Recv timeout -------------------------------------------------------------

TEST(PlatformSocket, SetRecvTimeoutCausesReadToTimeout) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();
    listener.set_non_blocking(false);

    std::thread server([&] {
        std::string addr;
        uint16_t rp;
        auto conn = tcp_accept(listener, addr, rp);
        // Accept but never send anything — let the client timeout
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });

    auto sock = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(sock.valid());
    sock.set_recv_timeout(200); // 200ms timeout

    char buf[64];
    auto start = std::chrono::steady_clock::now();
    ssize_t n = sock.recv(buf, sizeof(buf));
    auto elapsed = std::chrono::steady_clock::now() - start;

    // recv should return -1 or 0 (timeout/error), not block for 2 seconds
    EXPECT_LE(n, 0);
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 2);

    server.join();
}

TEST(PlatformSocket, SetSendTimeoutDoesNotCrash) {
    auto s = tcp_create();
    ASSERT_TRUE(s.valid());
    s.set_send_timeout(1000); // just verify it doesn't crash
    s.set_recv_timeout(1000);
}

// -- Non-blocking recv semantics ----------------------------------------------

// -- Local port -----------------------------------------------------------------

TEST(PlatformSocket, LocalPortReturnsAssignedPort) {
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();
    EXPECT_GT(port, 0u);
}

TEST(PlatformSocket, LocalPortOnUnboundSocketReturnsZero) {
    auto s = tcp_create();
    EXPECT_EQ(s.local_port(), 0u);
}

// -- Shutdown without prior connect ---------------------------------------------

TEST(PlatformSocket, ShutdownOnUnconnectedSocketDoesNotCrash) {
    auto s = tcp_create();
    s.shutdown(); // should not crash
    s.close();
}

// -- SSL methods on non-SSL socket -----------------------------------------------

TEST(PlatformSocket, NegotiatedProtocolEmptyOnPlainSocket) {
    auto s = tcp_create();
    EXPECT_TRUE(s.negotiated_protocol().empty());
}

TEST(PlatformSocket, IsSSLReturnsFalseOnPlainSocket) {
    auto s = tcp_create();
    EXPECT_FALSE(s.is_ssl());
}

// -- Non-blocking recv semantics ----------------------------------------------

TEST(PlatformSocket, NonBlockingRecvReturnsNegativeNotZero) {
    // Verify that recv on a non-blocking socket with no data returns -1
    // (EAGAIN), not 0 (which means graceful close). This distinction is
    // critical for TLS I/O loops that need to tell "no data yet" from
    // "connection closed".
    auto listener = tcp_listen("127.0.0.1", 0);
    uint16_t port = listener.local_port();
    listener.set_non_blocking(false);

    std::thread server([&] {
        std::string addr;
        uint16_t rp;
        auto conn = tcp_accept(listener, addr, rp);
        // Hold connection open but send nothing
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });

    auto sock = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(sock.valid());
    sock.set_non_blocking(true);

    char buf[64];
    ssize_t n = sock.recv(buf, sizeof(buf));

    // Non-blocking with no data: must return -1 (not 0)
    EXPECT_EQ(n, -1) << "non-blocking recv with no data should return -1, not " << n;

    server.join();
}

// -- IPv6 dual-stack listening ------------------------------------------------

#ifndef _WIN32
TEST(PlatformSocket, TcpListenDualStack) {
    // tcp_listen with "0.0.0.0" or empty address should attempt IPv6
    // dual-stack (accepts both IPv4 and IPv6), falling back to IPv4.
    auto listener = tcp_listen("0.0.0.0", 0);
    ASSERT_TRUE(listener.valid());
    uint16_t port = listener.local_port();
    EXPECT_GT(port, 0);

    // Verify we can connect via IPv4 loopback regardless of the
    // underlying socket family (dual-stack or pure IPv4 fallback).
    listener.set_non_blocking(true);
    auto client = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(client.valid());

    // Accept the connection
    listener.set_non_blocking(false);
    std::string remote;
    uint16_t rp = 0;
    auto conn = tcp_accept(listener, remote, rp);
    EXPECT_TRUE(conn.valid());
}

TEST(PlatformSocket, TcpListenExplicitIPv4Address) {
    // Explicit IPv4 address should always work regardless of IPv6 support
    auto listener = tcp_listen("127.0.0.1", 0);
    ASSERT_TRUE(listener.valid());
    uint16_t port = listener.local_port();
    EXPECT_GT(port, 0);

    auto client = tcp_connect("127.0.0.1", port);
    ASSERT_TRUE(client.valid());
}
#endif
