#include "moderation/UrlExtractor.h"

#include <gtest/gtest.h>

namespace {

using moderation::UrlExtractor;
using moderation::UrlLayerConfig;

UrlLayerConfig basic_config() {
    UrlLayerConfig cfg;
    cfg.enabled = true;
    cfg.blocked_score = 1.0;
    cfg.unknown_url_score = 0.0;
    return cfg;
}

} // namespace

TEST(UrlExtractorTest, PlainMessageNoUrls) {
    UrlExtractor ex;
    ex.configure(basic_config());
    auto r = ex.extract("hello there friend");
    EXPECT_TRUE(r.urls.empty());
    EXPECT_EQ(r.score, 0.0);
}

TEST(UrlExtractorTest, ExtractsHttpsUrl) {
    UrlExtractor ex;
    ex.configure(basic_config());
    auto r = ex.extract("check out https://example.com/page for details");
    ASSERT_EQ(r.urls.size(), 1u);
    EXPECT_NE(r.urls[0].find("example.com"), std::string::npos);
}

TEST(UrlExtractorTest, ExtractsBareDomain) {
    UrlExtractor ex;
    ex.configure(basic_config());
    auto r = ex.extract("visit example.com today");
    ASSERT_EQ(r.urls.size(), 1u);
    EXPECT_EQ(r.urls[0], "example.com");
}

TEST(UrlExtractorTest, BlocklistMatchScoresBlocked) {
    UrlLayerConfig cfg = basic_config();
    cfg.blocklist = {"free-robux", "bit.ly"};
    UrlExtractor ex;
    ex.configure(cfg);

    auto r = ex.extract("click here: bit.ly/scam-link");
    EXPECT_EQ(r.score, 1.0);
    EXPECT_EQ(r.blocked.size(), 1u);
}

TEST(UrlExtractorTest, AllowlistOverridesBlocklist) {
    UrlLayerConfig cfg = basic_config();
    cfg.blocklist = {".com"}; // ridiculously broad
    cfg.allowlist = {"aceattorneyonline.com"};
    UrlExtractor ex;
    ex.configure(cfg);

    auto r = ex.extract("see aceattorneyonline.com/forum");
    EXPECT_TRUE(r.urls.empty()); // allowed URLs aren't even extracted
    EXPECT_EQ(r.score, 0.0);
}

TEST(UrlExtractorTest, UnknownUrlScoreAppliedWhenConfigured) {
    UrlLayerConfig cfg = basic_config();
    cfg.unknown_url_score = 0.3;
    UrlExtractor ex;
    ex.configure(cfg);

    auto r = ex.extract("see random-domain.com for more");
    EXPECT_EQ(r.urls.size(), 1u);
    EXPECT_DOUBLE_EQ(r.score, 0.3);
}

TEST(UrlExtractorTest, DisabledLayerScoresZero) {
    UrlLayerConfig cfg;
    cfg.enabled = false;
    cfg.blocklist = {"bad.com"};
    UrlExtractor ex;
    ex.configure(cfg);

    auto r = ex.extract("visit bad.com right now");
    EXPECT_TRUE(r.urls.empty());
    EXPECT_EQ(r.score, 0.0);
}

TEST(UrlExtractorTest, TrailingPunctuationStripped) {
    UrlExtractor ex;
    ex.configure(basic_config());
    // "check example.com." — the trailing "." should not break the match.
    auto r = ex.extract("check example.com.");
    ASSERT_EQ(r.urls.size(), 1u);
    // Either "example.com" (if we strip) or "example.com." (if we don't).
    // Our implementation strips trailing punctuation.
    EXPECT_EQ(r.urls[0], "example.com");
}

TEST(UrlExtractorTest, CaseInsensitiveBlocklist) {
    UrlLayerConfig cfg = basic_config();
    cfg.blocklist = {"scam"};
    UrlExtractor ex;
    ex.configure(cfg);
    auto r = ex.extract("hit SCAM.tld for free");
    EXPECT_EQ(r.score, 1.0);
}
