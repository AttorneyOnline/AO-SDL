#include "net/ProxyProtocol.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> to_bytes(const std::string& s) {
    return {s.begin(), s.end()};
}

} // namespace

// -- PROXY protocol v1 --------------------------------------------------------

TEST(ProxyProtocolV1, TCP4) {
    auto data = to_bytes("PROXY TCP4 192.168.1.1 10.0.0.1 56324 443\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.client_addr, "192.168.1.1");
    EXPECT_EQ(result.client_port, 56324);
    EXPECT_EQ(result.server_addr, "10.0.0.1");
    EXPECT_EQ(result.server_port, 443);
    EXPECT_EQ(result.header_length, data.size());
}

TEST(ProxyProtocolV1, TCP6) {
    auto data = to_bytes("PROXY TCP6 2001:db8::1 ::1 56324 8080\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.client_addr, "2001:db8::1");
    EXPECT_EQ(result.client_port, 56324);
    EXPECT_EQ(result.server_port, 8080);
    EXPECT_EQ(result.header_length, data.size());
}

TEST(ProxyProtocolV1, Unknown) {
    auto data = to_bytes("PROXY UNKNOWN\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.client_addr.empty()); // No address info for UNKNOWN
    EXPECT_EQ(result.header_length, data.size());
}

TEST(ProxyProtocolV1, FollowedByHTTP) {
    auto data = to_bytes("PROXY TCP4 1.2.3.4 5.6.7.8 1234 80\r\nGET / HTTP/1.1\r\n\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.client_addr, "1.2.3.4");
    // header_length should only cover the PROXY line
    EXPECT_EQ(result.header_length, strlen("PROXY TCP4 1.2.3.4 5.6.7.8 1234 80\r\n"));
}

TEST(ProxyProtocolV1, Truncated) {
    auto data = to_bytes("PROXY TCP4 192.168.1.1");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.need_more_data); // No \r\n yet, could be incomplete
}

TEST(ProxyProtocolV1, InvalidProtocol) {
    auto data = to_bytes("PROXY UDP4 1.2.3.4 5.6.7.8 1234 80\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_FALSE(result.valid);
}

// -- PROXY protocol v2 --------------------------------------------------------

TEST(ProxyProtocolV2, IPv4) {
    // Build a v2 header: signature(12) + ver_cmd(1) + fam(1) + len(2) + addrs(12)
    std::vector<uint8_t> data = {
        0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A, // signature
        0x21,                                                                   // version 2, command PROXY
        0x11,                                                                   // AF_INET, STREAM
        0x00, 0x0C,                                                             // addr length = 12
    };
    // src: 192.168.1.100
    data.push_back(192);
    data.push_back(168);
    data.push_back(1);
    data.push_back(100);
    // dst: 10.0.0.1
    data.push_back(10);
    data.push_back(0);
    data.push_back(0);
    data.push_back(1);
    // src port: 12345 (0x3039)
    data.push_back(0x30);
    data.push_back(0x39);
    // dst port: 443 (0x01BB)
    data.push_back(0x01);
    data.push_back(0xBB);

    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.client_addr, "192.168.1.100");
    EXPECT_EQ(result.client_port, 12345);
    EXPECT_EQ(result.server_addr, "10.0.0.1");
    EXPECT_EQ(result.server_port, 443);
    EXPECT_EQ(result.header_length, 28u); // 16 + 12
}

TEST(ProxyProtocolV2, LocalCommand) {
    // LOCAL command (health check) — no address data
    std::vector<uint8_t> data = {
        0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,
        0x20,       // version 2, command LOCAL
        0x00,       // AF_UNSPEC
        0x00, 0x00, // addr length = 0
    };

    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.client_addr.empty());
    EXPECT_EQ(result.header_length, 16u);
}

TEST(ProxyProtocolV2, TruncatedHeader) {
    std::vector<uint8_t> data = {
        0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A, 0x21, 0x11,
    };
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.need_more_data);
}

// -- Not a PROXY protocol header -----------------------------------------------

TEST(ProxyProtocol, RegularHTTP) {
    auto data = to_bytes("GET / HTTP/1.1\r\n");
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.need_more_data);
}

TEST(ProxyProtocol, EmptyBuffer) {
    auto result = net::parse_proxy_protocol(nullptr, 0);
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.need_more_data);
}

TEST(ProxyProtocol, SingleByte) {
    uint8_t byte = 'G'; // Start of "GET" — not PROXY
    auto result = net::parse_proxy_protocol(&byte, 1);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.need_more_data); // 'G' doesn't match "PROXY" or v2 sig
}

TEST(ProxyProtocol, PartialPROXYPrefix) {
    auto data = to_bytes("PROX"); // Could be start of "PROXY "
    auto result = net::parse_proxy_protocol(data.data(), data.size());
    EXPECT_FALSE(result.valid);
    EXPECT_TRUE(result.need_more_data); // Need more to decide
}
