#include <gtest/gtest.h>

#include "net/WebSocketCommon.h"

TEST(WebSocketCommonTest, TrimWhitespace) {
    EXPECT_EQ(ws::trim("  hello  "), "hello");
    EXPECT_EQ(ws::trim("hello"), "hello");
    EXPECT_EQ(ws::trim("   "), "");
    EXPECT_EQ(ws::trim(""), "");
}

TEST(WebSocketCommonTest, CollapseLws) {
    EXPECT_EQ(ws::collapse_lws("a  b   c"), "a b c");
    EXPECT_EQ(ws::collapse_lws("  leading"), " leading");
    EXPECT_EQ(ws::collapse_lws("no\textra"), "no extra");
}

TEST(WebSocketCommonTest, ParseHttpHeader) {
    auto [key, val] = ws::parse_http_header("Content-Type: text/html");
    EXPECT_EQ(key, "Content-Type");
    EXPECT_EQ(val, "text/html");
}

TEST(WebSocketCommonTest, ParseHttpHeaderNoValue) {
    auto [key, val] = ws::parse_http_header("EmptyHeader");
    EXPECT_EQ(key, "EmptyHeader");
    EXPECT_EQ(val, "");
}

TEST(WebSocketCommonTest, ParseStatusLine) {
    auto sl = ws::parse_status_line("HTTP/1.1 101 Switching Protocols");
    EXPECT_EQ(sl.http_version, "HTTP/1.1");
    EXPECT_EQ(sl.status_code, 101);
    EXPECT_EQ(sl.status_reason, "Switching Protocols");
}

TEST(WebSocketCommonTest, ParseStatusLineInvalid) {
    EXPECT_THROW(ws::parse_status_line("GARBAGE"), std::runtime_error);
    EXPECT_THROW(ws::parse_status_line("HTTP/1.1 abc OK"), std::runtime_error);
}

TEST(WebSocketCommonTest, ParseRequestLine) {
    auto rl = ws::parse_request_line("GET /ws HTTP/1.1");
    EXPECT_EQ(rl.method, "GET");
    EXPECT_EQ(rl.uri, "/ws");
    EXPECT_EQ(rl.http_version, "HTTP/1.1");
}

TEST(WebSocketCommonTest, CaseInsensitiveEqual) {
    EXPECT_TRUE(ws::case_insensitive_equal("WebSocket", "websocket"));
    EXPECT_TRUE(ws::case_insensitive_equal("UPGRADE", "upgrade"));
    EXPECT_FALSE(ws::case_insensitive_equal("abc", "abcd"));
}

TEST(WebSocketCommonTest, GetLinesBasic) {
    std::string raw = "line1\r\nline2\r\n";
    std::vector<uint8_t> input(raw.begin(), raw.end());
    std::vector<uint8_t> remainder;
    auto lines = ws::get_lines(input, remainder);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_TRUE(remainder.empty());
}

TEST(WebSocketCommonTest, GetLinesPartial) {
    std::string raw = "complete\r\npartial";
    std::vector<uint8_t> input(raw.begin(), raw.end());
    std::vector<uint8_t> remainder;
    auto lines = ws::get_lines(input, remainder);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "complete");
    std::string rem(remainder.begin(), remainder.end());
    EXPECT_EQ(rem, "partial");
}

TEST(WebSocketCommonTest, GetLinesContinuation) {
    // First call leaves remainder
    std::string raw1 = "par";
    std::vector<uint8_t> input1(raw1.begin(), raw1.end());
    std::vector<uint8_t> remainder;
    auto lines1 = ws::get_lines(input1, remainder);
    EXPECT_TRUE(lines1.empty());

    // Second call completes the line
    std::string raw2 = "tial\r\n";
    std::vector<uint8_t> input2(raw2.begin(), raw2.end());
    auto lines2 = ws::get_lines(input2, remainder);
    ASSERT_EQ(lines2.size(), 1u);
    EXPECT_EQ(lines2[0], "partial");
}

TEST(WebSocketCommonTest, ComputeAcceptKey) {
    // RFC 6455 example: key "dGhlIHNhbXBsZSBub25jZQ==" → "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    auto result = ws::compute_accept_key("dGhlIHNhbXBsZSBub25jZQ==");
    EXPECT_EQ(result, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST(HTTPResponseTest, GetHeaderCaseInsensitive) {
    HTTPHeaders headers;
    headers["Content-Type"] = "text/html";
    headers["Connection"] = "Upgrade";
    HTTPResponse resp(HTTPResponse::StatusLine{"HTTP/1.1", 200, "OK"}, headers);

    EXPECT_EQ(resp.get_header("content-type"), "text/html");
    EXPECT_EQ(resp.get_header("CONNECTION"), "Upgrade");
    EXPECT_EQ(resp.get_header("Missing"), "");
}
