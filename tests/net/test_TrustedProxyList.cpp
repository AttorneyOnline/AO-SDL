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

TEST(ExtractForwardedIP, MultipleIPs) {
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
