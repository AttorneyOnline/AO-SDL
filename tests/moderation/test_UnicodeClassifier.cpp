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

// ===========================================================================
// Normal ASCII text — various lengths and content
// ===========================================================================

TEST_F(UnicodeClassifierTest, LongAsciiTextScoresZero) {
    auto r = classifier_.classify("This is a perfectly normal English sentence with no unusual characters. "
                                  "It talks about mundane things like the weather, food, and code reviews.");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.combining_marks, 0);
    EXPECT_EQ(r.format_chars, 0);
    EXPECT_EQ(r.exotic_script, 0);
    EXPECT_EQ(r.private_use, 0);
}

TEST_F(UnicodeClassifierTest, AsciiWithDigitsAndPunctuationScoresZero) {
    auto r = classifier_.classify("Hello! How are you? I'm doing well. 123 test @#$");
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, SingleAsciiCharScoresZero) {
    auto r = classifier_.classify("a");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.total_codepoints, 1);
}

// ===========================================================================
// Zalgo text — combining mark ratio tests
// ===========================================================================

TEST_F(UnicodeClassifierTest, MildZalgoJustBelowThreshold) {
    // With threshold at 0.3, a message where combining marks are < 30%
    // of codepoints should not trigger. 2 base + 0 marks = no score.
    // Let's do: "ab" with one combining mark — 3 codepoints, 1 mark = 33%
    // That's above threshold, so let's do 3 base + 1 mark = 25%
    std::string mild = "abc\xCC\x80"; // 3 base + 1 combining = 25%
    auto r = classifier_.classify(mild);
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.combining_marks, 1);
}

TEST_F(UnicodeClassifierTest, HeavyZalgoHighScore) {
    // Each base letter gets 20 combining marks, far above threshold
    std::string extreme = "a";
    for (int i = 0; i < 20; ++i)
        extreme += "\xCC\x80"; // U+0300
    extreme += "b";
    for (int i = 0; i < 20; ++i)
        extreme += "\xCC\x81"; // U+0301
    auto r = classifier_.classify(extreme);
    EXPECT_GT(r.score, 0.0);
    EXPECT_EQ(r.combining_marks, 40);
    EXPECT_NE(r.reason.find("zalgo"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, ZalgoFromDifferentCombiningBlocks) {
    // Combining marks from the extended block (U+1AB0-U+1AFF)
    std::string msg = "x";
    for (int i = 0; i < 15; ++i)
        msg += "\xE1\xAA\xB0"; // U+1AB0 (3-byte UTF-8)
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.combining_marks, 0);
}

// ===========================================================================
// Exotic script — Cuneiform, Linear B, Tags
// ===========================================================================

TEST_F(UnicodeClassifierTest, LinearBTriggersExotic) {
    // U+10000 LINEAR B SYLLABLE B008 A — F0 90 80 80
    std::string msg;
    for (int i = 0; i < 10; ++i)
        msg += "\xF0\x90\x80\x80";
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_EQ(r.exotic_script, 10);
    EXPECT_NE(r.reason.find("exotic_script"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, TagCharactersAreExotic) {
    // Tags block U+E0000-U+E007F — U+E0041 = F3 A0 81 81
    std::string msg;
    for (int i = 0; i < 10; ++i)
        msg += "\xF3\xA0\x81\x81"; // U+E0041
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.exotic_script, 0);
}

TEST_F(UnicodeClassifierTest, SingleExoticCharInLongMessageBelowThreshold) {
    // One cuneiform character among 50 ASCII chars — ratio = 1/51 = 2%
    // Well below the 30% threshold.
    std::string msg(50, 'a');
    msg += "\xF0\x92\x80\x80"; // U+12000 cuneiform
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.score, 0.0);       // below threshold
    EXPECT_EQ(r.exotic_script, 1); // still counted
}

// ===========================================================================
// Format characters — RLO, ZWJ, ZWNJ, ZWS, etc.
// ===========================================================================

TEST_F(UnicodeClassifierTest, RtlOverrideTriggersFormat) {
    // RLO U+202E (E2 80 AE) + a few normal chars
    std::string msg = "hi";
    for (int i = 0; i < 5; ++i)
        msg += "\xE2\x80\xAE"; // RLO
    // 2 ASCII + 5 format chars, format ratio = 5/7 = 71% (>> 10% threshold)
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
    EXPECT_GT(r.format_chars, 0);
    EXPECT_NE(r.reason.find("format_chars"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, ZeroWidthSpaceIsFormatChar) {
    // ZWSP U+200B (E2 80 8B)
    std::string msg = "a";
    for (int i = 0; i < 5; ++i)
        msg += "\xE2\x80\x8B";
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.format_chars, 0);
    EXPECT_GT(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, LrmRlmAreFormatChars) {
    // LRM U+200E (E2 80 8E), RLM U+200F (E2 80 8F)
    std::string msg = "hi";
    for (int i = 0; i < 5; ++i) {
        msg += "\xE2\x80\x8E"; // LRM
        msg += "\xE2\x80\x8F"; // RLM
    }
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.format_chars, 0);
}

TEST_F(UnicodeClassifierTest, PrivateUseCharsCounted) {
    // U+E000 (EE 80 80) — first private use char
    std::string msg = "hi";
    for (int i = 0; i < 5; ++i)
        msg += "\xEE\x80\x80";
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.private_use, 5);
    // format_ratio includes private_use, so score should be > 0
    EXPECT_GT(r.score, 0.0);
}

// ===========================================================================
// Mixed script detection
// ===========================================================================

TEST_F(UnicodeClassifierTest, LatinAndCyrillicTwoScripts) {
    // Latin 'h' + Cyrillic 'e' (U+0435) — only 2 scripts, not 3
    std::string msg = "h\xD0\xB5llo";
    auto r = classifier_.classify(msg);
    // ASCII and CYRILLIC = 2 scripts; script_mix only triggers at 3+
    EXPECT_EQ(r.script_count, 2);
    // Should NOT trigger script_mix (requires >= 3)
    EXPECT_EQ(r.reason.find("script_mix"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, ThreeScriptMixTriggersInShortMessage) {
    // ASCII + Cyrillic + Greek in a short message
    std::string msg = "h\xD0\xB5\xCE\xBFll"; // h + Cyrillic e + Greek o + l + l
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.script_count, 3);
    EXPECT_GT(r.score, 0.0);
    EXPECT_NE(r.reason.find("script_mix"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, ThreeScriptsInLongMessageNoTrigger) {
    // script_mix only triggers when total_codepoints < 64.
    // Build a 100-char message with three scripts.
    std::string msg(50, 'a'); // ASCII
    for (int i = 0; i < 5; ++i)
        msg += "\xD0\xB0"; // Cyrillic a (2 bytes each)
    for (int i = 0; i < 5; ++i)
        msg += "\xCE\xB1"; // Greek alpha (2 bytes each)
    // 60 codepoints total = still < 64 ... let's add more ASCII
    msg += std::string(20, 'b'); // now 80 codepoints > 64
    auto r = classifier_.classify(msg);
    EXPECT_GE(r.script_count, 3);
    // Score should be 0 because the message is long enough that
    // script mixing is not penalized (and ratios of exotic/format are low)
    EXPECT_EQ(r.reason.find("script_mix"), std::string::npos);
}

// ===========================================================================
// Emoji — should NOT trigger exotic script
// ===========================================================================

TEST_F(UnicodeClassifierTest, MultipleEmojisDoNotTrigger) {
    // Several emoji from U+1F300..U+1FAFF block
    std::string msg = "nice ";
    msg += "\xF0\x9F\x98\x80"; // U+1F600 grinning face
    msg += "\xF0\x9F\x8E\x89"; // U+1F389 party popper
    msg += "\xF0\x9F\x91\x8D"; // U+1F44D thumbs up
    msg += " great";
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.exotic_script, 0);
}

TEST_F(UnicodeClassifierTest, EmojiOnlyMessageScoresZero) {
    std::string msg;
    msg += "\xF0\x9F\x98\x80"; // grinning face
    msg += "\xF0\x9F\x98\x82"; // face with tears of joy
    msg += "\xF0\x9F\x98\x8D"; // smiling face with heart-eyes
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.score, 0.0);
}

// ===========================================================================
// CJK — should NOT trigger exotic for standard CJK
// ===========================================================================

TEST_F(UnicodeClassifierTest, JapaneseHiraganaDoesNotTrigger) {
    // U+3053 (E3 81 93) = こ, U+3093 (E3 82 93) = ん, etc.
    std::string msg = "\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF"; // こんにちは
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
    // Only CJK script, so no script mixing. Score should be 0.
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, ChineseUnifiedCJKDoesNotTrigger) {
    // U+4F60 (E4 BD A0) = 你, U+597D (E5 A5 BD) = 好
    std::string msg = "\xE4\xBD\xA0\xE5\xA5\xBD"; // 你好
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, KoreanHangulDoesNotTrigger) {
    // U+D55C (ED 95 9C) = 한, U+AE00 (EA B8 80) = 글
    std::string msg = "\xED\x95\x9C\xEA\xB8\x80"; // 한글
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, KatakanaDoesNotTrigger) {
    // U+30AB (E3 82 AB) = カ, U+30BF (E3 82 BF) = タ, U+30AB (E3 82 AB) = カ, U+30CA (E3 83 8A) = ナ
    std::string msg = "\xE3\x82\xAB\xE3\x82\xBF\xE3\x82\xAB\xE3\x83\x8A"; // カタカナ
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
    EXPECT_EQ(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, CjkExtBIsExotic) {
    // CJK Extension B (U+20000+) IS classified as exotic
    // U+20000 = F0 A0 80 80
    std::string msg;
    for (int i = 0; i < 10; ++i)
        msg += "\xF0\xA0\x80\x80";
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.exotic_script, 0);
    EXPECT_GT(r.score, 0.0);
}

// ===========================================================================
// Disabled config — all inputs should return zero
// ===========================================================================

TEST_F(UnicodeClassifierTest, DisabledConfigReturnsZeroForAllInputs) {
    UnicodeClassifier disabled;
    UnicodeLayerConfig cfg;
    cfg.enabled = false;
    disabled.configure(cfg);

    EXPECT_EQ(disabled.score("hello world"), 0.0);
    // Zalgo
    std::string zalgo = "h";
    for (int i = 0; i < 20; ++i)
        zalgo += "\xCC\x80";
    EXPECT_EQ(disabled.score(zalgo), 0.0);
    // Cuneiform
    std::string cuneiform;
    for (int i = 0; i < 10; ++i)
        cuneiform += "\xF0\x92\x80\x80";
    EXPECT_EQ(disabled.score(cuneiform), 0.0);
    // Format chars
    std::string format = "hi\xE2\x80\x8D\xE2\x80\xAE\xE2\x80\xAE";
    EXPECT_EQ(disabled.score(format), 0.0);
}

TEST_F(UnicodeClassifierTest, DisabledClassifyReturnsEmptyResult) {
    UnicodeClassifier disabled;
    UnicodeLayerConfig cfg;
    cfg.enabled = false;
    disabled.configure(cfg);

    auto r = disabled.classify("anything");
    EXPECT_EQ(r.score, 0.0);
    EXPECT_EQ(r.total_codepoints, 0);
    EXPECT_EQ(r.combining_marks, 0);
    EXPECT_EQ(r.format_chars, 0);
    EXPECT_EQ(r.exotic_script, 0);
    EXPECT_TRUE(r.reason.empty());
}

// ===========================================================================
// Max score clamping
// ===========================================================================

TEST_F(UnicodeClassifierTest, MaxScoreClampingAtDefault) {
    // Build a message with an extreme combining-mark ratio that
    // produces a raw score > 1.0. With 1 base char and 200 combining
    // marks, the combining ratio is 200/201 ~ 0.995, giving a raw
    // zalgo contribution of (0.995 - 0.3) / 0.7 ~ 0.993. Adding
    // invalid bytes pushes the total above 1.0, and the clamp at
    // max_score (1.0) should cap it.
    std::string extreme = "a";
    for (int i = 0; i < 200; ++i)
        extreme += "\xCC\x80"; // combining grave
    // Append some raw invalid bytes for the invalid_utf8 contributor
    for (int i = 0; i < 50; ++i)
        extreme += "\xFF";
    auto r = classifier_.classify(extreme);
    EXPECT_DOUBLE_EQ(r.score, 1.0); // clamped to max_score
}

TEST_F(UnicodeClassifierTest, CustomMaxScoreClamping) {
    UnicodeClassifier c;
    UnicodeLayerConfig cfg = enabled_config();
    cfg.max_score = 0.5;
    c.configure(cfg);

    // Build noisy input that would normally score above 0.5
    std::string msg = "a";
    for (int i = 0; i < 30; ++i)
        msg += "\xCC\x80";
    auto r = c.classify(msg);
    EXPECT_LE(r.score, 0.5);
    EXPECT_DOUBLE_EQ(r.score, 0.5);
}

TEST_F(UnicodeClassifierTest, MaxScoreZeroMeansAlwaysZero) {
    UnicodeClassifier c;
    UnicodeLayerConfig cfg = enabled_config();
    cfg.max_score = 0.0;
    c.configure(cfg);

    std::string msg = "a";
    for (int i = 0; i < 30; ++i)
        msg += "\xCC\x80";
    auto r = c.classify(msg);
    EXPECT_DOUBLE_EQ(r.score, 0.0);
}

// ===========================================================================
// Invalid UTF-8 — various forms
// ===========================================================================

TEST_F(UnicodeClassifierTest, AllInvalidBytesMaxScore) {
    // A string of all 0xFF bytes — every byte is invalid UTF-8
    std::string garbage(10, '\xFF');
    auto r = classifier_.classify(garbage);
    EXPECT_EQ(r.score, 1.0); // max_score for all-invalid
    EXPECT_EQ(r.total_codepoints, 0);
    EXPECT_NE(r.reason.find("invalid"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, TruncatedMultibyteSequence) {
    // Start of a 3-byte sequence but missing continuations
    std::string msg = "\xE0";
    auto r = classifier_.classify(msg);
    EXPECT_GT(r.score, 0.0);
}

TEST_F(UnicodeClassifierTest, OverlongEncodingRejected) {
    // Overlong 2-byte encoding of U+0000 (C0 80) — should be rejected
    std::string msg = "\xC0\x80";
    auto r = classifier_.classify(msg);
    // Both bytes should fail to decode — counted as invalid
    EXPECT_GT(r.score, 0.0);
}

// ===========================================================================
// Codepoint counting
// ===========================================================================

TEST_F(UnicodeClassifierTest, CodepointCountCorrectForAscii) {
    auto r = classifier_.classify("hello");
    EXPECT_EQ(r.total_codepoints, 5);
}

TEST_F(UnicodeClassifierTest, CodepointCountCorrectForMultibyte) {
    // Each Cyrillic letter is 2 bytes but 1 codepoint
    std::string msg = "\xD0\xB0\xD0\xB1\xD0\xB2"; // а б в
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.total_codepoints, 3);
}

TEST_F(UnicodeClassifierTest, CodepointCountCorrectFor4Byte) {
    // Each emoji is 4 bytes but 1 codepoint
    std::string msg = "\xF0\x9F\x98\x80\xF0\x9F\x98\x82"; // 2 emoji
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.total_codepoints, 2);
}

// ===========================================================================
// Script counting
// ===========================================================================

TEST_F(UnicodeClassifierTest, PureAsciiHasOneScript) {
    auto r = classifier_.classify("hello world");
    EXPECT_EQ(r.script_count, 1); // ASCII only
}

TEST_F(UnicodeClassifierTest, PureCyrillicHasOneScript) {
    // Cyrillic only: привет (U+043F U+0440 U+0438 U+0432 U+0435 U+0442)
    std::string msg = "\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.script_count, 1); // Cyrillic only
}

TEST_F(UnicodeClassifierTest, LatinExtendedIsLatinScript) {
    // Latin Extended: e-acute U+00E9 (C3 A9) is LATIN, not ASCII
    std::string msg = "caf\xC3\xA9"; // "cafe" with accent
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.score, 0.0);
    // ASCII + LATIN = 2 scripts
    EXPECT_EQ(r.script_count, 2);
}

// ===========================================================================
// Reason string composition
// ===========================================================================

TEST_F(UnicodeClassifierTest, ReasonMultipleContributions) {
    // Build a message with both zalgo and format chars triggering
    std::string msg = "a";
    for (int i = 0; i < 10; ++i)
        msg += "\xCC\x80"; // combining
    for (int i = 0; i < 5; ++i)
        msg += "\xE2\x80\xAE"; // RLO
    auto r = classifier_.classify(msg);
    EXPECT_NE(r.reason.find("zalgo"), std::string::npos);
    EXPECT_NE(r.reason.find("format_chars"), std::string::npos);
}

TEST_F(UnicodeClassifierTest, ReasonTrailingSpaceStripped) {
    // Verify the reason string does not end with a trailing space
    std::string msg = "a";
    for (int i = 0; i < 10; ++i)
        msg += "\xCC\x80";
    auto r = classifier_.classify(msg);
    if (!r.reason.empty())
        EXPECT_NE(r.reason.back(), ' ');
}

// ===========================================================================
// Score convenience method
// ===========================================================================

TEST_F(UnicodeClassifierTest, ScoreMethodMatchesClassifyScore) {
    std::string zalgo = "h";
    for (int i = 0; i < 10; ++i)
        zalgo += "\xCC\x80";
    zalgo += "i";
    double s = classifier_.score(zalgo);
    auto r = classifier_.classify(zalgo);
    EXPECT_DOUBLE_EQ(s, r.score);
}

TEST_F(UnicodeClassifierTest, ScoreMethodOnPlainText) {
    EXPECT_DOUBLE_EQ(classifier_.score("just normal text"), 0.0);
}

// ===========================================================================
// Arabic script (common, not exotic)
// ===========================================================================

TEST_F(UnicodeClassifierTest, ArabicTextNotExotic) {
    // U+0627 ARABIC LETTER ALEF = D8 A7, U+0644 = D9 84
    std::string msg = "\xD8\xA7\xD9\x84\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85"; // السلام
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
}

// ===========================================================================
// Hebrew script (common, not exotic)
// ===========================================================================

TEST_F(UnicodeClassifierTest, HebrewTextNotExotic) {
    // U+05E9 (D7 A9) = ש, U+05DC (D7 9C) = ל, U+05D5 (D7 95) = ו, U+05DD (D7 9D) = ם
    std::string msg = "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"; // שלום
    auto r = classifier_.classify(msg);
    EXPECT_EQ(r.exotic_script, 0);
}
