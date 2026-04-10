#include "moderation/UnicodeClassifier.h"

#include <gtest/gtest.h>

namespace {

using moderation::UnicodeClassifier;
using moderation::UnicodeLayerConfig;

UnicodeLayerConfig enabled_config() {
    UnicodeLayerConfig cfg;
    cfg.enabled = true;
    cfg.combining_mark_threshold = 0.3;
    cfg.exotic_script_threshold = 0.3;
    cfg.format_char_threshold = 0.1;
    cfg.max_score = 1.0;
    return cfg;
}

class UnicodeClassifierTest : public ::testing::Test {
  protected:
    void SetUp() override {
        classifier_.configure(enabled_config());
    }

    UnicodeClassifier classifier_;
};

} // namespace

TEST_F(UnicodeClassifierTest, PlainAsciiScoresZero) {
    auto r = classifier_.classify("hello world");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.combining_marks, 0);
    EXPECT_EQ(r.exotic_script, 0);
}

TEST_F(UnicodeClassifierTest, DisabledLayerReturnsZero) {
    UnicodeClassifier disabled;
    UnicodeLayerConfig cfg;
    cfg.enabled = false;
    disabled.configure(cfg);
    // Even with a zalgo blast, a disabled layer returns a zero score.
    EXPECT_EQ(disabled.score("ḧ̸̢̛̳͈́̂e̶̜̗̾͌l̷̜̄l̷̜̪̋͝o̴̧̿"), 0.0);
}

TEST_F(UnicodeClassifierTest, ZalgoTextTriggersCombining) {
    // "hello" with a thick stack of combining marks on each base letter.
    // Constructed byte-by-byte to avoid literal rendering surprises.
    std::string zalgo = "h";
    // Append a bunch of combining diacritical marks (U+0300..U+036F)
    // to the 'h' so the ratio crosses the threshold.
    for (int i = 0; i < 10; ++i) {
        zalgo += "\xCC\x80"; // U+0300 combining grave accent
    }
    zalgo += "i";
    auto r = classifier_.classify(zalgo);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.combining_marks, 0);
    EXPECT_NE(r.reason.find("zalgo"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, CuneiformTriggersExotic) {
    // U+12000 CUNEIFORM SIGN A — encoded as UTF-8 F0 92 80 80.
    std::string msg;
    for (int i = 0; i < 10; ++i)
        msg += "\xF0\x92\x80\x80";
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.exotic_script, 0);
    EXPECT_NE(r.reason.find("exotic_script"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, FormatCharacterAbuseTriggered) {
    // A string loaded with ZWJ (U+200D), LRO (U+202D), RLO (U+202E).
    std::string msg = "hi";
    for (int i = 0; i < 5; ++i) {
        msg += "\xE2\x80\x8D"; // ZWJ
        msg += "\xE2\x80\xAE"; // RLO
    }
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.format_chars, 0);
}

TEST_F(UnicodeClassifierTest, InvalidUtf8ScoresHigh) {
    // A lone continuation byte is invalid UTF-8.
    std::string msg = "\xC2"; // Unexpected end of 2-byte sequence.
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, EmojiIsCommonNotExotic) {
    // 🎉 U+1F389 PARTY POPPER — classified as COMMON in our scheme,
    // so it should NOT trigger the exotic-script signal.
    std::string msg = "congrats \xF0\x9F\x8E\x89 great job";
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, ScriptMixingInShortMessageTriggered) {
    // "hеllo" — the 'е' is Cyrillic U+0435 (2 bytes CD B5 in UTF-8).
    // Mix Latin(ASCII) + Cyrillic + Greek in a short message — three
    // distinct non-common scripts. This is the homoglyph-attack shape
    // and should trigger the script_mix signal.
    std::string msg = "h\xD0\xB5ll\xCE\xBF"; // h + cyrillic e + ll + greek omicron
    auto r = classifier_.classify(msg);
    // ASCII (h, l, l), Cyrillic (е), Greek (ο) → 3 scripts.
    EXPECT_EQ(r.script_count, 3);
    EXPECT_GT(r.score, 0.0);
    EXPECT_NE(r.reason.find("script_mix"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, EmptyMessageScoresZero) {
    auto r = classifier_.classify("");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.total_codepoints, 0);
}
