#include "ao/game/AOBackground.h"
#include "ao/asset/AOAssetLibrary.h"

#include "asset/AssetLibrary.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

class AOBackgroundTest : public ::testing::Test {
  protected:
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
    AOBackground bg;
};

} // namespace

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, InitialBackgroundIsEmpty) {
    EXPECT_TRUE(bg.background().empty());
}

TEST_F(AOBackgroundTest, InitialPositionIsWit) {
    EXPECT_EQ(bg.position(), "wit");
}

TEST_F(AOBackgroundTest, InitialBgAssetIsNull) {
    EXPECT_EQ(bg.bg_asset(), nullptr);
}

TEST_F(AOBackgroundTest, InitialDeskAssetIsNull) {
    EXPECT_EQ(bg.desk_asset(), nullptr);
}

// ---------------------------------------------------------------------------
// set() updates stored values
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, SetUpdatesBackgroundAndPosition) {
    bg.set("gs4", "def");
    EXPECT_EQ(bg.background(), "gs4");
    EXPECT_EQ(bg.position(), "def");
}

TEST_F(AOBackgroundTest, SetWithSameValuesIsIdempotent) {
    bg.set("gs4", "def");
    // Call reload to clear dirty flag.
    bg.reload_if_needed(ao_assets);

    // Set with same values — should NOT mark dirty.
    bg.set("gs4", "def");
    // reload_if_needed should be a no-op (dirty is still false).
    // We can verify by checking that assets aren't re-queried,
    // but at minimum this must not crash or change state.
    bg.reload_if_needed(ao_assets);

    EXPECT_EQ(bg.background(), "gs4");
    EXPECT_EQ(bg.position(), "def");
}

// ---------------------------------------------------------------------------
// set_position() updates only position
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, SetPositionUpdatesPosition) {
    bg.set("gs4", "def");
    bg.set_position("pro");
    EXPECT_EQ(bg.position(), "pro");
    EXPECT_EQ(bg.background(), "gs4");
}

TEST_F(AOBackgroundTest, SetPositionWithSameValueIsIdempotent) {
    bg.set("gs4", "wit");
    bg.reload_if_needed(ao_assets);

    bg.set_position("wit");
    // dirty should remain false — same position as current.
    bg.reload_if_needed(ao_assets);

    EXPECT_EQ(bg.position(), "wit");
}

// ---------------------------------------------------------------------------
// reload_if_needed with no mounts — assets remain null
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, ReloadWithNoMountsProducesNullBg) {
    bg.set("default", "wit");
    bg.reload_if_needed(ao_assets);
    EXPECT_EQ(bg.bg_asset(), nullptr);
}

TEST_F(AOBackgroundTest, ReloadWithNoMountsProducesNullDesk) {
    bg.set("default", "wit");
    bg.reload_if_needed(ao_assets);
    EXPECT_EQ(bg.desk_asset(), nullptr);
}

TEST_F(AOBackgroundTest, ReloadOnlyRunsWhenDirty) {
    // No set() called — reload should be a no-op even on first call
    // because dirty starts as false.
    bg.reload_if_needed(ao_assets);
    EXPECT_EQ(bg.bg_asset(), nullptr);
    EXPECT_EQ(bg.desk_asset(), nullptr);
}

// ---------------------------------------------------------------------------
// Multiple set() calls before reload
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, MultipleSetCallsBeforeReloadUsesLastValues) {
    bg.set("gs4", "def");
    bg.set("default", "pro");
    bg.set("courtroom", "jud");

    EXPECT_EQ(bg.background(), "courtroom");
    EXPECT_EQ(bg.position(), "jud");

    bg.reload_if_needed(ao_assets);
    // Assets are null because no mounts, but the stored values are correct.
    EXPECT_EQ(bg.background(), "courtroom");
    EXPECT_EQ(bg.position(), "jud");
}

// ---------------------------------------------------------------------------
// set() after reload marks dirty again
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, SetAfterReloadMarksDirtyAgain) {
    bg.set("gs4", "def");
    bg.reload_if_needed(ao_assets);

    bg.set("gs4", "pro");
    EXPECT_EQ(bg.position(), "pro");

    // This reload should run (dirty was set by changing position).
    bg.reload_if_needed(ao_assets);
    // Assets still null (no mounts), but no crash.
    EXPECT_EQ(bg.bg_asset(), nullptr);
}

// ---------------------------------------------------------------------------
// Position values — verify all standard positions can be set
// ---------------------------------------------------------------------------

TEST_F(AOBackgroundTest, AllStandardPositionsAccepted) {
    const std::vector<std::string> positions = {
        "def", "pro", "wit", "jud", "hld", "hlp", "jur", "sea"
    };

    for (const auto& pos : positions) {
        bg.set("default", pos);
        EXPECT_EQ(bg.position(), pos) << "Failed for position: " << pos;
        bg.reload_if_needed(ao_assets);
        // Should not crash for any position.
    }
}

TEST_F(AOBackgroundTest, CustomPositionAccepted) {
    bg.set("default", "custom_pos");
    EXPECT_EQ(bg.position(), "custom_pos");
    bg.reload_if_needed(ao_assets);
    // No crash — custom positions fall through to the default case
    // in resolve_bg_filename / resolve_desk_filename.
}
