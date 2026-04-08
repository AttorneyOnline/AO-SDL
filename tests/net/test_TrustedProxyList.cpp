#include "net/TrustedProxyList.h"

#include <gtest/gtest.h>

// -- TrustedProxyList ---------------------------------------------------------

TEST(TrustedProxyList, EmptyListTrustsNothing) {
    net::TrustedProxyList list;
    EXPECT_FALSE(list.is_trusted("127.0.0.1"));
    EXPECT_TRUE(list.empty());
}

TEST(TrustedProxyList, ExactIPv4Match) {
    net::TrustedProxyList list;
    list.configure({"127.0.0.1", "10.0.0.1"});
    EXPECT_TRUE(list.is_trusted("127.0.0.1"));
    EXPECT_TRUE(list.is_trusted("10.0.0.1"));
    EXPECT_FALSE(list.is_trusted("10.0.0.2"));
    EXPECT_FALSE(list.is_trusted("192.168.1.1"));
}

TEST(TrustedProxyList, CIDRv4Match) {
    net::TrustedProxyList list;
    list.configure({"10.0.0.0/8"});
    EXPECT_TRUE(list.is_trusted("10.0.0.1"));
    EXPECT_TRUE(list.is_trusted("10.255.255.255"));
    EXPECT_FALSE(list.is_trusted("11.0.0.1"));
    EXPECT_FALSE(list.is_trusted("192.168.1.1"));
}

TEST(TrustedProxyList, CIDRv4Slash16) {
    net::TrustedProxyList list;
    list.configure({"172.16.0.0/12"});
    EXPECT_TRUE(list.is_trusted("172.16.0.1"));
    EXPECT_TRUE(list.is_trusted("172.31.255.255"));
    EXPECT_FALSE(list.is_trusted("172.32.0.1"));
}

TEST(TrustedProxyList, ExactIPv6Match) {
    net::TrustedProxyList list;
    list.configure({"::1"});
    EXPECT_TRUE(list.is_trusted("::1"));
    EXPECT_FALSE(list.is_trusted("::2"));
    EXPECT_EQ(list.size(), 1u);
}

TEST(TrustedProxyList, CIDRv6Match) {
    net::TrustedProxyList list;
    list.configure({"fd00::/8"});
    EXPECT_TRUE(list.is_trusted("fd00::1"));
    EXPECT_TRUE(list.is_trusted("fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
    EXPECT_FALSE(list.is_trusted("fe00::1"));
}

TEST(TrustedProxyList, MixedIPv4AndIPv6) {
    net::TrustedProxyList list;
    list.configure({"127.0.0.1", "::1", "10.0.0.0/8"});
    EXPECT_TRUE(list.is_trusted("127.0.0.1"));
    EXPECT_TRUE(list.is_trusted("::1"));
    EXPECT_TRUE(list.is_trusted("10.1.2.3"));
    EXPECT_FALSE(list.is_trusted("192.168.1.1"));
    EXPECT_FALSE(list.is_trusted("::2"));
    EXPECT_EQ(list.size(), 3u);
}

TEST(TrustedProxyList, InvalidEntriesSkipped) {
    net::TrustedProxyList list;
    list.configure({"not-an-ip", "127.0.0.1", "999.999.999.999/8", "::1"});
    // Only valid entries should be configured
    EXPECT_TRUE(list.is_trusted("127.0.0.1"));
    EXPECT_TRUE(list.is_trusted("::1"));
    EXPECT_EQ(list.size(), 2u);
}

TEST(TrustedProxyList, IPv4MappedIPv6MatchesIPv4Rules) {
    net::TrustedProxyList list;
    list.configure({"172.16.0.0/12"});
    // Dual-stack sockets report IPv4 peers as ::ffff:a.b.c.d
    EXPECT_TRUE(list.is_trusted("::ffff:172.18.0.6"));
    EXPECT_TRUE(list.is_trusted("::ffff:172.31.255.255"));
    EXPECT_FALSE(list.is_trusted("::ffff:192.168.1.1"));
    // Plain IPv4 should still work
    EXPECT_TRUE(list.is_trusted("172.18.0.6"));
}

TEST(TrustedProxyList, EmptyStringIgnored) {
    net::TrustedProxyList list;
    list.configure({""});
    EXPECT_TRUE(list.empty());
}

TEST(TrustedProxyList, EmptyIPNotTrusted) {
    net::TrustedProxyList list;
    list.configure({"127.0.0.1"});
    EXPECT_FALSE(list.is_trusted(""));
}

// -- extract_forwarded_ip -----------------------------------------------------

TEST(ExtractForwardedIP, SingleIP) {
    EXPECT_EQ(net::extract_forwarded_ip("1.2.3.4"), "1.2.3.4");
}

TEST(ExtractForwardedIP, MultipleIPsNoTrustList) {
    // Without a trusted list, returns leftmost
    EXPECT_EQ(net::extract_forwarded_ip("1.2.3.4, 5.6.7.8, 9.10.11.12"), "1.2.3.4");
}

TEST(ExtractForwardedIP, WhitespaceTrimmed) {
    EXPECT_EQ(net::extract_forwarded_ip("  1.2.3.4  "), "1.2.3.4");
    EXPECT_EQ(net::extract_forwarded_ip("  1.2.3.4 , 5.6.7.8"), "1.2.3.4");
}

TEST(ExtractForwardedIP, IPv6Address) {
    EXPECT_EQ(net::extract_forwarded_ip("2001:db8::1"), "2001:db8::1");
    EXPECT_EQ(net::extract_forwarded_ip("2001:db8::1, 10.0.0.1"), "2001:db8::1");
}

TEST(ExtractForwardedIP, EmptyReturnsEmpty) {
    EXPECT_EQ(net::extract_forwarded_ip(""), "");
    EXPECT_EQ(net::extract_forwarded_ip("   "), "");
}

TEST(ExtractForwardedIP, InvalidIPRejected) {
    // Non-IP values should be rejected via inet_pton validation
    EXPECT_EQ(net::extract_forwarded_ip("><script>alert(1)</script>"), "");
    EXPECT_EQ(net::extract_forwarded_ip("not-an-ip"), "");
}

// -- Rightmost untrusted extraction -------------------------------------------

TEST(ExtractForwardedIP, RightmostUntrustedWithTrustList) {
    // Simulate: client 1.2.3.4 → proxy 10.0.0.1 → our proxy 10.0.0.2
    // XFF: "1.2.3.4, 10.0.0.1" (proxy appended the real client IP)
    // With spoofed header: "evil.ip, 1.2.3.4, 10.0.0.1"
    net::TrustedProxyList trusted;
    trusted.configure({"10.0.0.0/8"});

    // Normal case: rightmost untrusted is 1.2.3.4
    EXPECT_EQ(net::extract_forwarded_ip("1.2.3.4, 10.0.0.1", &trusted), "1.2.3.4");

    // Spoofed: attacker prepends "5.5.5.5" — rightmost untrusted is still the real client
    EXPECT_EQ(net::extract_forwarded_ip("5.5.5.5, 1.2.3.4, 10.0.0.1", &trusted), "1.2.3.4");
}

TEST(ExtractForwardedIP, RightmostUntrustedSingleIP) {
    net::TrustedProxyList trusted;
    trusted.configure({"10.0.0.0/8"});

    // Single untrusted IP — return it
    EXPECT_EQ(net::extract_forwarded_ip("1.2.3.4", &trusted), "1.2.3.4");

    // Single trusted IP — all trusted, falls back to leftmost
    EXPECT_EQ(net::extract_forwarded_ip("10.0.0.1", &trusted), "10.0.0.1");
}

TEST(ExtractForwardedIP, RightmostUntrustedAllTrusted) {
    net::TrustedProxyList trusted;
    trusted.configure({"10.0.0.0/8", "172.16.0.0/12"});

    // All IPs are trusted — falls back to leftmost
    EXPECT_EQ(net::extract_forwarded_ip("10.0.0.1, 172.16.0.1", &trusted), "10.0.0.1");
}

TEST(ExtractForwardedIP, RightmostSkipsInvalidEntries) {
    net::TrustedProxyList trusted;
    trusted.configure({"10.0.0.0/8"});

    // Invalid entries in the chain should be skipped
    EXPECT_EQ(net::extract_forwarded_ip("1.2.3.4, garbage, 10.0.0.1", &trusted), "1.2.3.4");
}
