#include <gtest/gtest.h>

#include "MockTcpSocket.h"
#include "net/IServerSocket.h"
#include "net/WebSocketServer.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <queue>

// Mock server socket that returns pre-configured mock client sockets.
class MockServerSocket : public IServerSocket {
  public:
    void bind_and_listen(uint16_t, int) override {
        bound_ = true;
    }
    void set_non_blocking(bool) override {
    }
    void close() override {
        bound_ = false;
    }

    std::unique_ptr<ITcpSocket> accept() override {
        if (pending_clients_.empty())
            return nullptr;
        auto client = std::move(pending_clients_.front());
        pending_clients_.pop();
        return client;
    }

    void add_client(std::unique_ptr<MockTcpSocket> client) {
        pending_clients_.push(std::move(client));
    }

    bool bound_ = false;

  private:
    std::queue<std::unique_ptr<MockTcpSocket>> pending_clients_;
};

// Build a valid WebSocket upgrade request.
static std::vector<uint8_t> make_upgrade_request(const std::string& key = "dGhlIHNhbXBsZSBub25jZQ==",
                                                 const std::string& subprotocol = "") {
    std::string req = "GET / HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Version: 13\r\n"
                      "Sec-WebSocket-Key: " +
                      key + "\r\n";
    if (!subprotocol.empty())
        req += "Sec-WebSocket-Protocol: " + subprotocol + "\r\n";
    req += "\r\n";
    return {req.begin(), req.end()};
}

// Build a masked WebSocket close frame (code 1000).
static std::vector<uint8_t> make_masked_close_frame() {
    std::vector<uint8_t> frame;
    frame.push_back(0x88); // FIN + CLOSE
    frame.push_back(0x82); // masked, 2 bytes payload
    // Mask key: all zeros
    frame.push_back(0);
    frame.push_back(0);
    frame.push_back(0);
    frame.push_back(0);
    // Close code 1000 in network byte order (unmasked since mask is zero)
    frame.push_back(0x03);
    frame.push_back(0xE8);
    return frame;
}

// Build a masked WebSocket text frame.
static std::vector<uint8_t> make_masked_text_frame(const std::string& payload) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);                                     // FIN + TEXT
    uint8_t len = static_cast<uint8_t>(payload.size()) | 0x80; // masked
    frame.push_back(len);
    // Mask key: all zeros (simplest — XOR is identity)
    frame.push_back(0);
    frame.push_back(0);
    frame.push_back(0);
    frame.push_back(0);
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

class WebSocketServerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto listener = std::make_unique<MockServerSocket>();
        mock_listener_ = listener.get();
        server_ = std::make_unique<WebSocketServer>(std::move(listener));
    }

    // Add a client that sends an upgrade request on first recv.
    MockTcpSocket* add_connecting_client(const std::string& subprotocol = "") {
        auto client = std::make_unique<MockTcpSocket>();
        auto* ptr = client.get();
        client->feed(make_upgrade_request("dGhlIHNhbXBsZSBub25jZQ==", subprotocol));
        mock_listener_->add_client(std::move(client));
        return ptr;
    }

    MockServerSocket* mock_listener_ = nullptr;
    std::unique_ptr<WebSocketServer> server_;
};

TEST_F(WebSocketServerTest, StartBindsListener) {
    server_->start(8081);
    EXPECT_TRUE(mock_listener_->bound_);
}

TEST_F(WebSocketServerTest, AcceptAndHandshake) {
    server_->start(8081);
    auto* client = add_connecting_client();

    std::vector<uint64_t> connected_ids;
    server_->on_client_connected([&](uint64_t id) { connected_ids.push_back(id); });

    server_->poll();

    ASSERT_EQ(connected_ids.size(), 1u);
    EXPECT_EQ(server_->client_count(), 1u);

    // Verify the 101 response was sent
    auto& sent = client->sent();
    std::string response(sent.begin(), sent.end());
    EXPECT_NE(response.find("101 Switching Protocols"), std::string::npos);
    EXPECT_NE(response.find("Sec-WebSocket-Accept"), std::string::npos);
}

TEST_F(WebSocketServerTest, SubprotocolNegotiation) {
    server_->set_supported_subprotocols({"aonx", "ao2"});
    server_->start(8081);
    add_connecting_client("ao2, aonx");

    uint64_t cid = 0;
    server_->on_client_connected([&](uint64_t id) { cid = id; });
    server_->poll();

    ASSERT_NE(cid, 0u);
    // Server iterates client's preference order, selects first match
    EXPECT_EQ(server_->get_client_subprotocol(cid), "ao2");
}

TEST_F(WebSocketServerTest, SubprotocolFallbackToAo2) {
    server_->set_supported_subprotocols({"aonx", "ao2"});
    server_->start(8081);
    add_connecting_client("ao2");

    uint64_t cid = 0;
    server_->on_client_connected([&](uint64_t id) { cid = id; });
    server_->poll();

    EXPECT_EQ(server_->get_client_subprotocol(cid), "ao2");
}

TEST_F(WebSocketServerTest, ReadClientFrame) {
    server_->start(8081);
    auto* client = add_connecting_client();

    uint64_t cid = 0;
    server_->on_client_connected([&](uint64_t id) { cid = id; });
    server_->poll(); // handshake

    // Feed a masked text frame
    client->feed(make_masked_text_frame("hello"));
    auto frames = server_->poll();

    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].client_id, cid);
    std::string payload(frames[0].frame.data.begin(), frames[0].frame.data.end());
    EXPECT_EQ(payload, "hello");
}

TEST_F(WebSocketServerTest, BroadcastSendsToAllClients) {
    server_->start(8081);
    auto* c1 = add_connecting_client();
    auto* c2 = add_connecting_client();

    server_->on_client_connected([](uint64_t) {});
    server_->poll(); // handshake both

    ASSERT_EQ(server_->client_count(), 2u);

    c1->clear_sent();
    c2->clear_sent();

    std::string msg = "broadcast!";
    server_->broadcast({reinterpret_cast<const uint8_t*>(msg.data()), msg.size()});

    // Both clients should have received a frame
    EXPECT_GT(c1->sent().size(), 0u);
    EXPECT_GT(c2->sent().size(), 0u);
}

TEST_F(WebSocketServerTest, SendToSpecificClient) {
    server_->start(8081);
    auto* c1 = add_connecting_client();
    auto* c2 = add_connecting_client();

    uint64_t id1 = 0, id2 = 0;
    int n = 0;
    server_->on_client_connected([&](uint64_t id) {
        if (n++ == 0)
            id1 = id;
        else
            id2 = id;
    });
    server_->poll();

    c1->clear_sent();
    c2->clear_sent();

    std::string msg = "just for you";
    server_->send(id1, {reinterpret_cast<const uint8_t*>(msg.data()), msg.size()});

    EXPECT_GT(c1->sent().size(), 0u);
    EXPECT_EQ(c2->sent().size(), 0u);
}

TEST_F(WebSocketServerTest, StopClosesAllClients) {
    server_->start(8081);
    add_connecting_client();
    server_->on_client_connected([](uint64_t) {});
    server_->poll();

    EXPECT_EQ(server_->client_count(), 1u);
    server_->stop();
    EXPECT_EQ(server_->client_count(), 0u);
}

TEST_F(WebSocketServerTest, DisconnectCallbackFiresOnClientClose) {
    server_->start(8081);
    auto* client = add_connecting_client();

    uint64_t connected_id = 0;
    server_->on_client_connected([&](uint64_t id) { connected_id = id; });

    uint64_t disconnected_id = 0;
    server_->on_client_disconnected([&](uint64_t id) { disconnected_id = id; });

    server_->poll(); // handshake
    ASSERT_NE(connected_id, 0u);

    // Client sends a close frame
    client->feed(make_masked_close_frame());
    server_->poll(); // processes close

    EXPECT_EQ(disconnected_id, connected_id);
    EXPECT_EQ(server_->client_count(), 0u);
}

TEST_F(WebSocketServerTest, DisconnectCallbackFiresOnCloseClient) {
    server_->start(8081);
    add_connecting_client();

    uint64_t connected_id = 0;
    server_->on_client_connected([&](uint64_t id) { connected_id = id; });

    uint64_t disconnected_id = 0;
    server_->on_client_disconnected([&](uint64_t id) { disconnected_id = id; });

    server_->poll();
    ASSERT_NE(connected_id, 0u);

    server_->close_client(connected_id);

    EXPECT_EQ(disconnected_id, connected_id);
    EXPECT_EQ(server_->client_count(), 0u);
}

TEST_F(WebSocketServerTest, DisconnectCallbackFiresOnStop) {
    server_->start(8081);
    add_connecting_client();
    add_connecting_client();

    std::vector<uint64_t> connected_ids;
    server_->on_client_connected([&](uint64_t id) { connected_ids.push_back(id); });

    std::vector<uint64_t> disconnected_ids;
    server_->on_client_disconnected([&](uint64_t id) { disconnected_ids.push_back(id); });

    server_->poll();
    ASSERT_EQ(connected_ids.size(), 2u);

    server_->stop();

    // Both clients should get disconnect callbacks
    ASSERT_EQ(disconnected_ids.size(), 2u);
    std::sort(connected_ids.begin(), connected_ids.end());
    std::sort(disconnected_ids.begin(), disconnected_ids.end());
    EXPECT_EQ(disconnected_ids, connected_ids);
}

// The entire point of firing callbacks outside the lock: it must be
// safe to call send() from within on_disconnected_ without deadlocking.
TEST_F(WebSocketServerTest, SendFromDisconnectCallbackDoesNotDeadlock) {
    server_->start(8081);
    auto* c1 = add_connecting_client();
    auto* c2 = add_connecting_client();

    uint64_t id1 = 0, id2 = 0;
    int n = 0;
    server_->on_client_connected([&](uint64_t id) {
        if (n++ == 0)
            id1 = id;
        else
            id2 = id;
    });

    server_->on_client_disconnected([&](uint64_t id) {
        // When client 1 disconnects, try to send to client 2.
        // This would deadlock if the callback fired under the mutex.
        if (id == id1) {
            std::string msg = "goodbye";
            server_->send(id2, {reinterpret_cast<const uint8_t*>(msg.data()), msg.size()});
        }
    });

    server_->poll(); // handshake both
    ASSERT_NE(id1, 0u);
    ASSERT_NE(id2, 0u);
    c2->clear_sent();

    // Client 1 sends close frame
    c1->feed(make_masked_close_frame());
    server_->poll(); // processes close → fires callback → sends to c2

    // If we get here, no deadlock. Verify c2 received the message.
    EXPECT_GT(c2->sent().size(), 0u);
}
