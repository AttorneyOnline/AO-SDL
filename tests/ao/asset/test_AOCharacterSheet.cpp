#include "ao/asset/AOCharacterSheet.h"

#include "asset/AssetLibrary.h"
#include "asset/Mount.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

#include <unordered_map>

// ---------------------------------------------------------------------------
// In-memory Mount for feeding test INI data to AssetLibrary::config().
// ---------------------------------------------------------------------------

namespace {

class MemoryMount : public Mount {
  public:
    MemoryMount() : Mount(std::filesystem::path{}) {
    }

    void add_file(const std::string& path, const std::string& content) {
        files_[path] = std::vector<uint8_t>(content.begin(), content.end());
    }

    void load() override {
    }

    bool seek_file(const std::string& path) const override {
        return files_.count(path) > 0;
    }

    std::vector<uint8_t> fetch_data(const std::string& path) override {
        auto it = files_.find(path);
        if (it != files_.end())
            return it->second;
        return {};
    }

  protected:
    void load_cache() override {
    }
    void save_cache() override {
    }

  private:
    std::unordered_map<std::string, std::vector<uint8_t>> files_;
};

// A well-formed char.ini for testing.
const char* WELL_FORMED_INI = R"ini(
[Options]
showname = Phoenix Wright
side = def
blips = male
effects = realization
chat = aa

[Emotions]
number = 3
1 = Normal#-#normal#0#1
2 = Thinking#think_pre#thinking#5#0
3 = Pointing#point_pre#pointing#1#3

[SoundN]
1 = sfx_blink
2 = 0
3 = sfx_deskslam

[SoundT]
1 = 0
2 = 0
3 = 4

[SoundL]
1 = 0
2 = 0
3 = 1

[Time]
think_pre = 500
point_pre = 800

[pointing_FrameSFX]
3 = sfx_impact
7 = sfx_whoosh

[pointing_FrameScreenshake]
5 = 1
10 = 1

[normal_FrameRealization]
2 = 1
)ini";

// ---------------------------------------------------------------------------
// Fixture: MountManager with an in-memory mount holding a char.ini.
// ---------------------------------------------------------------------------

class AOCharacterSheetTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mount_ = new MemoryMount();
        // MountManager takes ownership via unique_ptr
        mounts_.add_mount(std::unique_ptr<Mount>(mount_));
    }

    void set_char_ini(const std::string& character, const std::string& content) {
        mount_->add_file("characters/" + character + "/char.ini", content);
    }

    MountManager mounts_;
    AssetLibrary assets_{mounts_, 0};
    MemoryMount* mount_ = nullptr; // non-owning; MountManager owns it
};

} // namespace

// ===========================================================================
// 1. Construction and initial state
// ===========================================================================

TEST_F(AOCharacterSheetTest, LoadReturnsNulloptForMissingCharacter) {
    auto sheet = AOCharacterSheet::load(assets_, "NonExistent");
    EXPECT_FALSE(sheet.has_value());
}

TEST_F(AOCharacterSheetTest, LoadReturnsValueForExistingCharacter) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    EXPECT_TRUE(sheet.has_value());
}

TEST_F(AOCharacterSheetTest, NameEqualsCharacterArgument) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->name(), "Phoenix");
}

// ===========================================================================
// 2. Parsing a well-formed char.ini
// ===========================================================================

TEST_F(AOCharacterSheetTest, ShownameParsedFromOptions) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->showname(), "Phoenix Wright");
}

TEST_F(AOCharacterSheetTest, SideParsedFromOptions) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->side(), "def");
}

TEST_F(AOCharacterSheetTest, BlipsParsedFromOptions) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->blips(), "male");
}

TEST_F(AOCharacterSheetTest, EffectsFolderParsedFromOptions) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->effects_folder(), "realization");
}

TEST_F(AOCharacterSheetTest, ChatStyleParsedFromOptions) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->chat_style(), "aa");
}

TEST_F(AOCharacterSheetTest, EmoteFieldsParsedCorrectly) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_GE(sheet->emote_count(), 1);

    // Emote 0: Normal#-#normal#0#1
    const auto& e0 = sheet->ao_emote(0);
    EXPECT_EQ(e0.comment, "Normal");
    EXPECT_EQ(e0.pre_anim, "-");
    EXPECT_EQ(e0.anim_name, "normal");
    EXPECT_EQ(e0.mod, 0);
    EXPECT_EQ(e0.desk_mod, 1);
}

TEST_F(AOCharacterSheetTest, EmoteWithNonZeroModParsedCorrectly) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_GE(sheet->emote_count(), 2);

    // Emote 1: Thinking#think_pre#thinking#5#0
    const auto& e1 = sheet->ao_emote(1);
    EXPECT_EQ(e1.comment, "Thinking");
    EXPECT_EQ(e1.pre_anim, "think_pre");
    EXPECT_EQ(e1.anim_name, "thinking");
    EXPECT_EQ(e1.mod, 5);
    EXPECT_EQ(e1.desk_mod, 0);
}

TEST_F(AOCharacterSheetTest, SoundNParsedIntoEmotes) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_GE(sheet->emote_count(), 3);

    // Emote 0: sfx_blink
    EXPECT_EQ(sheet->ao_emote(0).sfx_name, "sfx_blink");
    // Emote 1: "0" means no SFX
    EXPECT_EQ(sheet->ao_emote(1).sfx_name, "");
    // Emote 2: sfx_deskslam
    EXPECT_EQ(sheet->ao_emote(2).sfx_name, "sfx_deskslam");
}

TEST_F(AOCharacterSheetTest, SoundTParsedIntoEmotes) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_GE(sheet->emote_count(), 3);

    EXPECT_EQ(sheet->ao_emote(0).sfx_delay, 0);
    EXPECT_EQ(sheet->ao_emote(2).sfx_delay, 4);
}

TEST_F(AOCharacterSheetTest, SoundLParsedIntoEmotes) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_GE(sheet->emote_count(), 3);

    EXPECT_FALSE(sheet->ao_emote(0).sfx_looping);
    EXPECT_TRUE(sheet->ao_emote(2).sfx_looping);
}

TEST_F(AOCharacterSheetTest, TimeSectionParsedCorrectly) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    EXPECT_EQ(sheet->preanim_duration_ms("think_pre"), 500);
    EXPECT_EQ(sheet->preanim_duration_ms("point_pre"), 800);
    EXPECT_EQ(sheet->preanim_duration_ms("nonexistent"), 0);
}

TEST_F(AOCharacterSheetTest, FrameSFXParsedAndSorted) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    const auto& sfx = sheet->frame_sfx("pointing");
    ASSERT_EQ(sfx.size(), 2u);
    EXPECT_EQ(sfx[0].first, 3);
    EXPECT_EQ(sfx[0].second, "sfx_impact");
    EXPECT_EQ(sfx[1].first, 7);
    EXPECT_EQ(sfx[1].second, "sfx_whoosh");
}

TEST_F(AOCharacterSheetTest, FrameScreenshakeParsedAndSorted) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    const auto& shake = sheet->frame_screenshake("pointing");
    ASSERT_EQ(shake.size(), 2u);
    EXPECT_EQ(shake[0], 5);
    EXPECT_EQ(shake[1], 10);
}

TEST_F(AOCharacterSheetTest, FrameRealizationParsedAndSorted) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    const auto& real = sheet->frame_realization("normal");
    ASSERT_EQ(real.size(), 1u);
    EXPECT_EQ(real[0], 2);
}

TEST_F(AOCharacterSheetTest, FrameEffectsReturnsEmptyForUnknownAnim) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    EXPECT_TRUE(sheet->frame_sfx("nonexistent").empty());
    EXPECT_TRUE(sheet->frame_screenshake("nonexistent").empty());
    EXPECT_TRUE(sheet->frame_realization("nonexistent").empty());
}

// ===========================================================================
// 3. Finding emotes by name
// ===========================================================================

TEST_F(AOCharacterSheetTest, FindEmoteByAnimNameReturnsCorrectEntry) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    const AOEmoteEntry* found = sheet->find_emote("thinking");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->comment, "Thinking");
    EXPECT_EQ(found->pre_anim, "think_pre");
    EXPECT_EQ(found->mod, 5);
}

TEST_F(AOCharacterSheetTest, FindEmoteReturnsNullptrForUnknownName) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    EXPECT_EQ(sheet->find_emote("nonexistent"), nullptr);
}

// ===========================================================================
// 4. Emote count
// ===========================================================================

TEST_F(AOCharacterSheetTest, EmoteCountMatchesIniData) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->emote_count(), 3);
}

TEST_F(AOCharacterSheetTest, EmotesVectorSizeMatchesCount) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ((int)sheet->emotes().size(), sheet->emote_count());
}

TEST_F(AOCharacterSheetTest, EmoteInterfaceMethodReturnsCorrectData) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    // Test the ICharacterSheet::emote() method (returns EmoteEntry)
    EmoteEntry e = sheet->emote(2);
    EXPECT_EQ(e.comment, "Pointing");
    EXPECT_EQ(e.pre_anim, "point_pre");
    EXPECT_EQ(e.anim_name, "pointing");
    EXPECT_EQ(e.mod, 1);
    EXPECT_EQ(e.desk_mod, 3);
    EXPECT_EQ(e.sfx_name, "sfx_deskslam");
    EXPECT_EQ(e.sfx_delay, 4);
}

// ===========================================================================
// 5. Missing / malformed INI data
// ===========================================================================

TEST_F(AOCharacterSheetTest, MissingOptionsSectionUsesDefaults) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Normal#-#normal#0#0
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());

    // showname defaults to character name
    EXPECT_EQ(sheet->showname(), "TestChar");
    // side defaults to "wit"
    EXPECT_EQ(sheet->side(), "wit");
    // blips defaults to "male"
    EXPECT_EQ(sheet->blips(), "male");
    // chat defaults to "default"
    EXPECT_EQ(sheet->chat_style(), "default");
    // effects defaults to empty
    EXPECT_EQ(sheet->effects_folder(), "");
}

TEST_F(AOCharacterSheetTest, MissingEmotionsSectionResultsInZeroEmotes) {
    const char* ini = R"ini(
[Options]
showname = Test
side = pro
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->emote_count(), 0);
}

TEST_F(AOCharacterSheetTest, EmoteEntryWithOnlyComment) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = JustAComment
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);
    EXPECT_EQ(sheet->ao_emote(0).comment, "JustAComment");
    EXPECT_EQ(sheet->ao_emote(0).pre_anim, "");
    EXPECT_EQ(sheet->ao_emote(0).anim_name, "");
    EXPECT_EQ(sheet->ao_emote(0).mod, 0);
    EXPECT_EQ(sheet->ao_emote(0).desk_mod, 0);
}

TEST_F(AOCharacterSheetTest, EmoteEntryWithPartialFields) {
    // Only comment, pre_anim, and anim_name (no mod or desk_mod)
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Comment#preanim#animname
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);
    EXPECT_EQ(sheet->ao_emote(0).comment, "Comment");
    EXPECT_EQ(sheet->ao_emote(0).pre_anim, "preanim");
    EXPECT_EQ(sheet->ao_emote(0).anim_name, "animname");
    EXPECT_EQ(sheet->ao_emote(0).mod, 0);
    EXPECT_EQ(sheet->ao_emote(0).desk_mod, 0);
}

TEST_F(AOCharacterSheetTest, EmoteNumberHigherThanActualEntries) {
    // number says 5, but only 2 entries present
    const char* ini = R"ini(
[Emotions]
number = 5
1 = A#-#anim_a#0#0
3 = C#-#anim_c#0#0
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    // Only entries 1 and 3 exist, entries 2, 4, 5 are skipped
    EXPECT_EQ(sheet->emote_count(), 2);
}

TEST_F(AOCharacterSheetTest, EmoteNumberZeroResultsInNoEmotes) {
    const char* ini = R"ini(
[Emotions]
number = 0
1 = A#-#anim_a#0#0
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->emote_count(), 0);
}

TEST_F(AOCharacterSheetTest, SoundNWithValue1IsIgnored) {
    // "1" is treated as no-SFX, same as "0"
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Normal#-#normal#0#0

[SoundN]
1 = 1
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);
    EXPECT_EQ(sheet->ao_emote(0).sfx_name, "");
}

TEST_F(AOCharacterSheetTest, SoundNOutOfRangeIsIgnored) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Normal#-#normal#0#0

[SoundN]
5 = sfx_outofrange
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);
    // Out-of-range SoundN entry should not crash or affect anything
    EXPECT_EQ(sheet->ao_emote(0).sfx_name, "");
}

TEST_F(AOCharacterSheetTest, TimeZeroOrNegativeIsIgnored) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Normal#-#normal#0#0

[Time]
normal = 0
other = -100
valid = 300
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->preanim_duration_ms("normal"), 0);
    EXPECT_EQ(sheet->preanim_duration_ms("other"), 0);
    EXPECT_EQ(sheet->preanim_duration_ms("valid"), 300);
}

// ===========================================================================
// 6. Edge cases
// ===========================================================================

TEST_F(AOCharacterSheetTest, EmptyFileProducesDefaults) {
    set_char_ini("Empty", "");
    auto sheet = AOCharacterSheet::load(assets_, "Empty");
    ASSERT_TRUE(sheet.has_value());

    EXPECT_EQ(sheet->name(), "Empty");
    EXPECT_EQ(sheet->showname(), "Empty");
    EXPECT_EQ(sheet->side(), "wit");
    EXPECT_EQ(sheet->blips(), "male");
    EXPECT_EQ(sheet->chat_style(), "default");
    EXPECT_EQ(sheet->effects_folder(), "");
    EXPECT_EQ(sheet->emote_count(), 0);
}

TEST_F(AOCharacterSheetTest, WhitespaceOnlyFileProducesDefaults) {
    set_char_ini("Whitespace", "   \n\n  \n");
    auto sheet = AOCharacterSheet::load(assets_, "Whitespace");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->emote_count(), 0);
    EXPECT_EQ(sheet->showname(), "Whitespace");
}

TEST_F(AOCharacterSheetTest, CommentsOnlyFileProducesDefaults) {
    set_char_ini("Comments", "; This is a comment\n# Also a comment\n");
    auto sheet = AOCharacterSheet::load(assets_, "Comments");
    ASSERT_TRUE(sheet.has_value());
    EXPECT_EQ(sheet->emote_count(), 0);
}

TEST_F(AOCharacterSheetTest, EmoteWithEmptyPreanim) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Test##animname#2#1
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);
    EXPECT_EQ(sheet->ao_emote(0).pre_anim, "");
    EXPECT_EQ(sheet->ao_emote(0).anim_name, "animname");
}

TEST_F(AOCharacterSheetTest, MultipleCharactersSameMount) {
    set_char_ini("CharA", R"ini(
[Options]
showname = Character A
side = def

[Emotions]
number = 1
1 = A#-#anim_a#0#0
)ini");
    set_char_ini("CharB", R"ini(
[Options]
showname = Character B
side = pro

[Emotions]
number = 2
1 = B1#-#anim_b1#0#0
2 = B2#-#anim_b2#0#0
)ini");

    auto sheetA = AOCharacterSheet::load(assets_, "CharA");
    auto sheetB = AOCharacterSheet::load(assets_, "CharB");

    ASSERT_TRUE(sheetA.has_value());
    ASSERT_TRUE(sheetB.has_value());

    EXPECT_EQ(sheetA->showname(), "Character A");
    EXPECT_EQ(sheetA->emote_count(), 1);

    EXPECT_EQ(sheetB->showname(), "Character B");
    EXPECT_EQ(sheetB->emote_count(), 2);
}

TEST_F(AOCharacterSheetTest, EmoteAccessViaAtThrowsOnOutOfRange) {
    set_char_ini("Phoenix", WELL_FORMED_INI);
    auto sheet = AOCharacterSheet::load(assets_, "Phoenix");
    ASSERT_TRUE(sheet.has_value());

    EXPECT_THROW(sheet->ao_emote(999), std::out_of_range);
    EXPECT_THROW(sheet->emote(-1), std::out_of_range);
}

TEST_F(AOCharacterSheetTest, FindEmoteReturnsFirstMatch) {
    // If two emotes have the same anim_name, find_emote returns the first
    const char* ini = R"ini(
[Emotions]
number = 2
1 = First#-#samename#0#0
2 = Second#-#samename#1#1
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());

    const AOEmoteEntry* found = sheet->find_emote("samename");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->comment, "First");
}

TEST_F(AOCharacterSheetTest, MissingSoundSectionsLeaveDefaults) {
    const char* ini = R"ini(
[Emotions]
number = 1
1 = Normal#-#normal#0#0
)ini";
    set_char_ini("TestChar", ini);
    auto sheet = AOCharacterSheet::load(assets_, "TestChar");
    ASSERT_TRUE(sheet.has_value());
    ASSERT_EQ(sheet->emote_count(), 1);

    EXPECT_EQ(sheet->ao_emote(0).sfx_name, "");
    EXPECT_EQ(sheet->ao_emote(0).sfx_delay, 0);
    EXPECT_FALSE(sheet->ao_emote(0).sfx_looping);
}
