#include "ao/asset/AOAssetLibrary.h"

#include "asset/AssetLibrary.h"
#include "asset/Mount.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Fixture: empty MountManager so every lookup returns nullptr / nullopt.
// ---------------------------------------------------------------------------

namespace {

class AOAssetLibraryTest : public ::testing::Test {
  protected:
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets, "testtheme"};
};

class AOAssetLibraryDefaultThemeTest : public ::testing::Test {
  protected:
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
};

} // namespace

// ---------------------------------------------------------------------------
// Constructor / theme
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, ConstructorSetsTheme) {
    EXPECT_EQ(ao_assets.theme(), "testtheme");
}

TEST_F(AOAssetLibraryDefaultThemeTest, DefaultThemeIsDefault) {
    EXPECT_EQ(ao_assets.theme(), "default");
}

TEST_F(AOAssetLibraryTest, ThemeReturnsConstructorValue) {
    AOAssetLibrary custom{engine_assets, "custom_theme"};
    EXPECT_EQ(custom.theme(), "custom_theme");
}

// ---------------------------------------------------------------------------
// Character sprites — nullptr with no mounts
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, CharacterEmoteReturnsNullptrWithNoMounts) {
    auto result = ao_assets.character_emote("Phoenix", "normal", "(a)");
    EXPECT_EQ(result, nullptr);
}

TEST_F(AOAssetLibraryTest, CharacterIconReturnsNullptrWithNoMounts) {
    auto result = ao_assets.character_icon("Phoenix");
    EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// Background — nullptr with no mounts
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, BackgroundReturnsNullptrWithNoMounts) {
    auto result = ao_assets.background("gs4", "def");
    EXPECT_EQ(result, nullptr);
}

TEST_F(AOAssetLibraryTest, DeskOverlayReturnsNullptrWithNoMounts) {
    auto result = ao_assets.desk_overlay("gs4", "def");
    EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// Theme / UI — nullptr / nullopt with no mounts
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, ThemeImageLoadsFromEmbeddedAssets) {
    auto result = ao_assets.theme_image("chat");
    // Embedded assets include themes/default/chat.png
    EXPECT_NE(result, nullptr);
}

TEST_F(AOAssetLibraryTest, ThemeConfigLoadsFromEmbeddedAssets) {
    auto result = ao_assets.theme_config("courtroom_design.ini");
    EXPECT_TRUE(result.has_value());
}

// ---------------------------------------------------------------------------
// design_rect — reads from embedded courtroom_design.ini
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, DesignRectReadsFromEmbeddedConfig) {
    AORect rect = ao_assets.design_rect("ao2_chatbox");
    // Embedded courtroom_design.ini has non-zero chatbox dimensions
    EXPECT_GT(rect.w, 0);
    EXPECT_GT(rect.h, 0);
}

// ---------------------------------------------------------------------------
// message_font_spec — sensible defaults with no config
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, MessageFontSpecReturnsSensibleDefaults) {
    AOFontSpec spec = ao_assets.message_font_spec();
    EXPECT_EQ(spec.name, "arial");
    EXPECT_EQ(spec.size_pt, 10);
    EXPECT_TRUE(spec.sharp);
}

// ---------------------------------------------------------------------------
// text_colors — 9 entries with correct defaults
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, TextColorsReturns9Entries) {
    auto colors = ao_assets.text_colors();
    EXPECT_EQ(colors.size(), 9u);
}

TEST_F(AOAssetLibraryTest, TextColorsIndex0IsWhiteAndTalking) {
    auto colors = ao_assets.text_colors();
    ASSERT_GE(colors.size(), 1u);
    EXPECT_EQ(colors[0].r, 247);
    EXPECT_EQ(colors[0].g, 247);
    EXPECT_EQ(colors[0].b, 247);
    EXPECT_TRUE(colors[0].talking);
}

TEST_F(AOAssetLibraryTest, TextColorsIndex3IsOrangeAndNotTalking) {
    auto colors = ao_assets.text_colors();
    ASSERT_GE(colors.size(), 4u);
    EXPECT_EQ(colors[3].r, 247);
    EXPECT_EQ(colors[3].g, 115);
    EXPECT_EQ(colors[3].b, 57);
    EXPECT_FALSE(colors[3].talking);
}

TEST_F(AOAssetLibraryTest, TextColorsIndex4IsBlueAndNotTalking) {
    auto colors = ao_assets.text_colors();
    ASSERT_GE(colors.size(), 5u);
    EXPECT_EQ(colors[4].r, 107);
    EXPECT_EQ(colors[4].g, 198);
    EXPECT_EQ(colors[4].b, 247);
    EXPECT_FALSE(colors[4].talking);
}

// ---------------------------------------------------------------------------
// find_font — nullopt with no mounts
// ---------------------------------------------------------------------------

TEST_F(AOAssetLibraryTest, FindFontReturnsNulloptWithNoMounts) {
    auto result = ao_assets.find_font("arial");
    EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// Position origin and slide duration
// ---------------------------------------------------------------------------

namespace {

class InMemoryMount : public Mount {
  public:
    InMemoryMount() : Mount("memory://") {
    }
    void add(const std::string& path, const std::string& content) {
        files_[path] = {content.begin(), content.end()};
    }
    void load() override {
    }
    bool seek_file(const std::string& path) const override {
        return files_.count(path) > 0;
    }
    std::vector<uint8_t> fetch_data(const std::string& path) override {
        auto it = files_.find(path);
        return it != files_.end() ? it->second : std::vector<uint8_t>{};
    }

  protected:
    void load_cache() override {
    }
    void save_cache() override {
    }

  private:
    std::unordered_map<std::string, std::vector<uint8_t>> files_;
};

} // namespace

TEST(AOAssetLibraryPositionData, SlideDurationDefaultWithNoConfig) {
    MountManager mounts;
    AssetLibrary engine(mounts, 0);
    AOAssetLibrary ao(engine);
    // Returns a sensible default (600ms) when no design.ini exists
    EXPECT_GT(ao.slide_duration_ms("gs4", "def", "wit"), 0);
}

TEST(AOAssetLibraryPositionData, SlideDurationParsed) {
    MountManager mounts;
    auto mount = std::make_unique<InMemoryMount>();
    // QSettings INI format: "def/slide_ms_wit" becomes section [def], key slide_ms_wit
    mount->add("background/gs4/design.ini", "[def]\nslide_ms_wit = 800\n[wit]\nslide_ms_def = 600\n");
    mounts.add_mount(std::move(mount));

    AssetLibrary engine(mounts, 0);
    AOAssetLibrary ao(engine);

    EXPECT_EQ(ao.slide_duration_ms("gs4", "def", "wit"), 800);
    EXPECT_EQ(ao.slide_duration_ms("gs4", "wit", "def"), 600);
    // def→pro not in config, but still returns default
    EXPECT_GT(ao.slide_duration_ms("gs4", "def", "pro"), 0);
}
