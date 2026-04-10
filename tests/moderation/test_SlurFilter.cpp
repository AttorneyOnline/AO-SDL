// SlurFilter tests.
//
// All test inputs use placeholder tokens ("evilword", "badterm",
// "zzznasty") instead of real slurs. The filter itself is agnostic to
// what's in the wordlist — we're testing the normalizer, tokenizer,
// word-boundary matcher, and exception-list suppression, not the
// contents of any particular list. Keeping the test file free of
// real slurs also lets it live in the repo without the "repo contains
// a wordlist" problem the production path is designed to avoid.
#include "moderation/SlurFilter.h"

#include <gtest/gtest.h>

namespace {

using moderation::SlurFilter;
using moderation::SlurLayerConfig;

SlurLayerConfig make_cfg() {
    SlurLayerConfig cfg;
    cfg.enabled = true;
    cfg.match_score = 1.0;
    return cfg;
}

// SlurFilter owns a mutex and is non-copyable; we configure in place
// on a stack-allocated instance inside each test. This helper wraps
// the three-call setup so the test bodies stay focused on behavior.
void setup_filter(SlurFilter& f, const std::vector<std::string>& wordlist,
                  const std::vector<std::string>& exceptions = {}) {
    f.configure(make_cfg());
    f.load_wordlist(wordlist);
    f.load_exceptions(exceptions);
}

} // namespace

// --- Normalizer unit tests ------------------------------------------

TEST(SlurFilterNormalize, LowercasesAscii) {
    EXPECT_EQ(SlurFilter::normalize("HELLO"), "hello");
    EXPECT_EQ(SlurFilter::normalize("Hello World"), "hello world");
}

TEST(SlurFilterNormalize, StripsCombiningMarks) {
    // "a" + U+0301 COMBINING ACUTE ACCENT → "a"
    // The input is UTF-8: 'a' (0x61), then 0xCC 0x81 (the combining mark).
    EXPECT_EQ(SlurFilter::normalize("a\xCC\x81"), "a");
    // A zalgo-y "word" with 3 stacked combining marks.
    EXPECT_EQ(SlurFilter::normalize("h\xCC\x80""e\xCC\x81\xCC\x82llo"), "hello");
}

TEST(SlurFilterNormalize, StripsZeroWidth) {
    // ZWSP (U+200B, UTF-8: E2 80 8B) inserted between letters.
    EXPECT_EQ(SlurFilter::normalize("bad\xE2\x80\x8Bword"), "badword");
    // ZWJ and WORD JOINER also stripped.
    EXPECT_EQ(SlurFilter::normalize("bad\xE2\x80\x8Cword"), "badword");
    EXPECT_EQ(SlurFilter::normalize("bad\xE2\x81\xA0word"), "badword");
}

TEST(SlurFilterNormalize, FoldsCyrillicHomoglyphs) {
    // Cyrillic "а" (U+0430, UTF-8: D0 B0) + regular "bc" → "abc"
    EXPECT_EQ(SlurFilter::normalize("\xD0\xB0""bc"), "abc");
    // "evilword" with every Latin e/i/o replaced by Cyrillic
    // homoglyphs: е U+0435 (D0 B5), і U+0456 (D1 96), о U+043E (D0 BE).
    // Input letter sequence: [е][v][і][l][w][о][r][d]
    EXPECT_EQ(SlurFilter::normalize("\xD0\xB5v\xD1\x96lw\xD0\xBErd"), "evilword");
}

TEST(SlurFilterNormalize, FoldsFullwidth) {
    // "ＡＢＣ" (U+FF21..U+FF23, fullwidth) → "abc"
    EXPECT_EQ(SlurFilter::normalize("\xEF\xBC\xA1\xEF\xBC\xA2\xEF\xBC\xA3"), "abc");
}

TEST(SlurFilterNormalize, FoldsLeetDigits) {
    EXPECT_EQ(SlurFilter::normalize("b4dw0rd"), "badword");
    EXPECT_EQ(SlurFilter::normalize("3v1l"), "evil");
    EXPECT_EQ(SlurFilter::normalize("5up3r"), "super");
}

TEST(SlurFilterNormalize, CollapsesRepeats) {
    // 3+ runs collapse to 1 letter.
    EXPECT_EQ(SlurFilter::normalize("nnnnnnice"), "nice");
    EXPECT_EQ(SlurFilter::normalize("niiiiice"), "nice");
    // Runs of 2 stay (so "book" isn't mangled).
    EXPECT_EQ(SlurFilter::normalize("book"), "book");
    EXPECT_EQ(SlurFilter::normalize("committee"), "committee");
}

TEST(SlurFilterNormalize, NonAsciiBecomesBoundary) {
    // Japanese "bad単語" has "単語" as unmappable → should become
    // " " and break the token.
    auto norm = SlurFilter::normalize("bad\xE5\x8D\x98\xE8\xAA\x9E""word");
    auto toks = SlurFilter::tokenize(norm);
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0], "bad");
    EXPECT_EQ(toks[1], "word");
}

// --- Tokenizer unit tests --------------------------------------------

TEST(SlurFilterTokenize, SplitsOnPunctuation) {
    auto toks = SlurFilter::tokenize("hello, world! how are you?");
    ASSERT_EQ(toks.size(), 5u);
    EXPECT_EQ(toks[0], "hello");
    EXPECT_EQ(toks[4], "you");
}

TEST(SlurFilterTokenize, EmptyOnBlank) {
    EXPECT_TRUE(SlurFilter::tokenize("").empty());
    EXPECT_TRUE(SlurFilter::tokenize("   ").empty());
    EXPECT_TRUE(SlurFilter::tokenize("!!!???").empty());
}

// --- Suffix strip ----------------------------------------------------

TEST(SlurFilterSuffix, StripsCommonInflections) {
    EXPECT_EQ(SlurFilter::strip_suffix("dogs"), "dog");
    EXPECT_EQ(SlurFilter::strip_suffix("wishes"), "wish");
    EXPECT_EQ(SlurFilter::strip_suffix("walked"), "walk");
    EXPECT_EQ(SlurFilter::strip_suffix("running"), "runn");
    EXPECT_EQ(SlurFilter::strip_suffix("builder"), "build");
    EXPECT_EQ(SlurFilter::strip_suffix("builders"), "build");
}

TEST(SlurFilterSuffix, LeavesShortStemsAlone) {
    // "her" should NOT strip to "h".
    EXPECT_EQ(SlurFilter::strip_suffix("her"), "her");
    // "is" has no matching suffix.
    EXPECT_EQ(SlurFilter::strip_suffix("is"), "is");
}

// --- End-to-end scan behavior ----------------------------------------

TEST(SlurFilterScan, InertWhenNoList) {
    SlurFilter f;
    f.configure(make_cfg());
    // No wordlist loaded — is_active() is false, scan returns 0.
    EXPECT_FALSE(f.is_active());
    auto r = f.scan("anything goes");
    EXPECT_EQ(r.score, 0.0);
}

TEST(SlurFilterScan, InertWhenDisabled) {
    SlurFilter f;
    SlurLayerConfig cfg = make_cfg();
    cfg.enabled = false;
    f.configure(cfg);
    f.load_wordlist({"evilword"});
    // Wordlist loaded, but config disabled → inert.
    EXPECT_FALSE(f.is_active());
}

TEST(SlurFilterScan, ExactMatchFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    EXPECT_TRUE(f.is_active());
    auto r = f.scan("you are an evilword stop it");
    EXPECT_EQ(r.score, 1.0);
    ASSERT_EQ(r.matched.size(), 1u);
    EXPECT_EQ(r.matched[0], "evilword");
}

TEST(SlurFilterScan, CaseInsensitive) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    auto r = f.scan("EVILWORD");
    EXPECT_EQ(r.score, 1.0);
}

TEST(SlurFilterScan, SuffixInflectionFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    EXPECT_EQ(f.scan("evilwords").score, 1.0);
    EXPECT_EQ(f.scan("evilwording").score, 1.0);
    EXPECT_EQ(f.scan("evilworder").score, 1.0);
}

TEST(SlurFilterScan, HomoglyphEvasionFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    // Cyrillic e (D0 B5) and o (D0 BE)
    auto r = f.scan("\xD0\xB5vilw\xD0\xBErd");
    EXPECT_EQ(r.score, 1.0);
}

TEST(SlurFilterScan, LeetEvasionFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    EXPECT_EQ(f.scan("3v1lw0rd").score, 1.0);
}

TEST(SlurFilterScan, RepeatEvasionFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    EXPECT_EQ(f.scan("eeeeevilword").score, 1.0);
    EXPECT_EQ(f.scan("evilllllword").score, 1.0);
}

TEST(SlurFilterScan, ZeroWidthEvasionFires) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    // "evil" + ZWSP + "word"
    EXPECT_EQ(f.scan("evil\xE2\x80\x8Bword").score, 1.0);
}

// The Scunthorpe suite. Each of these innocent words contains a
// shorter "bad" wordlist entry as a SUBSTRING. A substring matcher
// would fire; our word-boundary matcher must not.
TEST(SlurFilterScan, ScunthorpeSafe) {
    // Wordlist contains the inner substring, but the real word is
    // longer and wraps it. Classic Scunthorpe.
    SlurFilter f;
    setup_filter(f, {"cunt"});
    EXPECT_EQ(f.scan("the town of scunthorpe").score, 0.0);
    EXPECT_EQ(f.scan("Scunthorpe United is a football club").score, 0.0);
}

TEST(SlurFilterScan, TherapistSafe) {
    SlurFilter f;
    setup_filter(f, {"rapist"});
    EXPECT_EQ(f.scan("I visited my therapist today").score, 0.0);
    EXPECT_EQ(f.scan("group therapy with a therapist").score, 0.0);
}

TEST(SlurFilterScan, ClassifySafe) {
    SlurFilter f;
    setup_filter(f, {"ass"});
    EXPECT_EQ(f.scan("we need to classify these results").score, 0.0);
    EXPECT_EQ(f.scan("assassin creed is a game").score, 0.0);
    EXPECT_EQ(f.scan("passive voice is bad style").score, 0.0);
}

TEST(SlurFilterScan, AnalysisSafe) {
    SlurFilter f;
    setup_filter(f, {"anal"});
    EXPECT_EQ(f.scan("statistical analysis of the data").score, 0.0);
    EXPECT_EQ(f.scan("a deep analysis shows").score, 0.0);
}

TEST(SlurFilterScan, BangaloreSafe) {
    // Contains "bang" as a substring.
    SlurFilter f;
    setup_filter(f, {"bang"});
    EXPECT_EQ(f.scan("flew into bangalore airport").score, 0.0);
}

TEST(SlurFilterScan, ExceptionListSuppresses) {
    // Add the wordlist entry, but also add it to the exception list —
    // real deployments use this for reclaimed language used in-group.
    SlurFilter f;
    setup_filter(f, {"badterm"}, {"badterm"});
    auto r = f.scan("hey badterm");
    EXPECT_EQ(r.score, 0.0);
}

TEST(SlurFilterScan, ExceptionListDoesNotMaskOthers) {
    // Only "badterm" is in the exception list; "zzznasty" still fires.
    SlurFilter f;
    setup_filter(f, {"badterm", "zzznasty"}, {"badterm"});
    EXPECT_EQ(f.scan("hey badterm").score, 0.0);
    EXPECT_EQ(f.scan("hey zzznasty").score, 1.0);
}

TEST(SlurFilterScan, MultipleMatchesReported) {
    SlurFilter f;
    setup_filter(f, {"badterm", "zzznasty"});
    auto r = f.scan("badterm and zzznasty in the same message");
    EXPECT_EQ(r.score, 1.0);
    EXPECT_EQ(r.matched.size(), 2u);
}

TEST(SlurFilterScan, WordlistSizeReports) {
    // Duplicates and entries that normalize to the same token collapse.
    SlurFilter f;
    f.configure(make_cfg());
    f.load_wordlist({"badterm", "BadTerm", "BADTERM", "badterm "});
    EXPECT_EQ(f.wordlist_size(), 1u);
}

TEST(SlurFilterScan, EmptyMessageNoop) {
    SlurFilter f;
    setup_filter(f, {"evilword"});
    EXPECT_EQ(f.scan("").score, 0.0);
    EXPECT_EQ(f.scan("   ").score, 0.0);
}

TEST(SlurFilterScan, SourceCodeReferencesClean) {
    // Regression: code-adjacent chatter shouldn't trip the filter.
    // These mirror the "SourceCodeReferencesNotDetected" guard in
    // test_UrlExtractor.cpp.
    SlurFilter f;
    setup_filter(f, {"ass", "cunt"});
    for (const auto& msg : {"check main.cpp for the bug", "see classify() in game.h", "the passthrough is broken",
                            "scunthorpe FC just signed a midfielder"}) {
        auto r = f.scan(msg);
        EXPECT_EQ(r.score, 0.0) << "false positive on: " << msg;
    }
}

// --- Minimum-length guard on load_wordlist ---------------------------

TEST(SlurFilterLoad, RejectsShortTwoLetterTokens) {
    // The 3-char floor blocks ambiguous 2-letter tokens from ever
    // entering the active wordlist. "ai", "it", "us", "eu" are all
    // common abbreviations that would false-positive on nearly every
    // message if they snuck in.
    SlurFilter f;
    setup_filter(f, {"ai", "it", "us", "eu", "is", "as"});
    EXPECT_EQ(f.wordlist_size(), 0u);
    EXPECT_FALSE(f.is_active());
}

TEST(SlurFilterLoad, RejectsSingleCharTokens) {
    SlurFilter f;
    setup_filter(f, {"a", "i", "x"});
    EXPECT_EQ(f.wordlist_size(), 0u);
}

TEST(SlurFilterLoad, AcceptsThreeLetterTokens) {
    // Three characters is the floor — right at the boundary, loads OK.
    SlurFilter f;
    setup_filter(f, {"xyz"});
    EXPECT_EQ(f.wordlist_size(), 1u);
    EXPECT_EQ(f.scan("the xyz problem").score, 1.0);
}

TEST(SlurFilterLoad, NumericEntryDoesNotFireOnAI) {
    // Regression: "41%" is a known transphobic dog whistle. Our
    // single-token filter normalizes it via leet-digit fold
    // (4 -> a, 1 -> i, % -> boundary) to the two-letter token "ai".
    // Without the min-length guard, loading the entry would put
    // "ai" in the active wordlist and match any message about
    // artificial intelligence. The guard makes the entry safe to
    // include in the source file as documentation / future phrase-
    // matching without breaking mundane AI chat.
    SlurFilter f;
    setup_filter(f, {"41%"});
    EXPECT_EQ(f.wordlist_size(), 0u);

    // Verify the AI false-positive specifically does NOT fire.
    auto r = f.scan("AI research is advancing quickly");
    EXPECT_EQ(r.score, 0.0);
    auto r2 = f.scan("the state of ai in 2026");
    EXPECT_EQ(r2.score, 0.0);
}

TEST(SlurFilterLoad, MixedListKeepsValidEntries) {
    // Mixed load: some entries pass the guard, some don't. The
    // passing ones should still arrive in the wordlist; the failing
    // ones should be silently dropped without disabling the layer.
    SlurFilter f;
    setup_filter(f, {"41%", "evilword", "ai", "badterm", "xy"});
    EXPECT_EQ(f.wordlist_size(), 2u);
    EXPECT_TRUE(f.is_active());
    EXPECT_EQ(f.scan("hey evilword").score, 1.0);
    EXPECT_EQ(f.scan("hey badterm").score, 1.0);
    // But the rejected short entries still don't fire.
    EXPECT_EQ(f.scan("talking about ai").score, 0.0);
    EXPECT_EQ(f.scan("the xy plane").score, 0.0);
}

// --- parse_text_list + TextListFetcher parser ------------------------

#include "moderation/TextListFetcher.h"

TEST(TextListParse, BasicLines) {
    auto entries = moderation::parse_text_list("foo\nbar\nbaz\n");
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0], "foo");
    EXPECT_EQ(entries[1], "bar");
    EXPECT_EQ(entries[2], "baz");
}

TEST(TextListParse, SkipsCommentsAndBlanks) {
    auto entries = moderation::parse_text_list(
        "# this is a comment\n"
        "\n"
        "foo\n"
        "   \n"
        "# another comment\n"
        "bar\n");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0], "foo");
    EXPECT_EQ(entries[1], "bar");
}

TEST(TextListParse, TrimsWhitespace) {
    auto entries = moderation::parse_text_list("  foo  \n\tbar\t\n");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0], "foo");
    EXPECT_EQ(entries[1], "bar");
}

TEST(TextListParse, HandlesCRLF) {
    auto entries = moderation::parse_text_list("foo\r\nbar\r\n");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0], "foo");
    EXPECT_EQ(entries[1], "bar");
}

TEST(TextListParse, StripsBOM) {
    // UTF-8 BOM EF BB BF prepended.
    auto entries = moderation::parse_text_list("\xEF\xBB\xBF""foo\nbar\n");
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0], "foo");
}
