#include "ao/asset/AOAssetLibrary.h"
#include "ao/game/AOEmotePlayer.h"

#include "asset/AssetLibrary.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

class AOEmotePlayerTest : public ::testing::Test {
  protected:
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
    AOEmotePlayer player;
};

} // namespace

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, InitialStateIsNone) {
    EXPECT_EQ(player.state(), AOEmotePlayer::State::NONE);
}

TEST_F(AOEmotePlayerTest, InitialHasFrameIsFalse) {
    EXPECT_FALSE(player.has_frame());
}

TEST_F(AOEmotePlayerTest, InitialCurrentFrameIsNull) {
    EXPECT_EQ(player.current_frame(), nullptr);
}

TEST_F(AOEmotePlayerTest, InitialCurrentFrameIndexIsZero) {
    EXPECT_EQ(player.current_frame_index(), 0);
}

TEST_F(AOEmotePlayerTest, InitialAssetIsNull) {
    EXPECT_EQ(player.asset(), nullptr);
}

// ---------------------------------------------------------------------------
// start() with EmoteMod::IDLE — goes directly to IDLE
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, StartIdleModSetsStateToTalking) {
    // EmoteMod::IDLE (0) means "skip preanim" — starts in TALKING, not IDLE
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, StartIdleWithNoAssetsHasNoFrame) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_FALSE(player.has_frame());
    EXPECT_EQ(player.current_frame(), nullptr);
}

TEST_F(AOEmotePlayerTest, StartIdleWithNoAssetsAssetIsNull) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.asset(), nullptr);
}

// ---------------------------------------------------------------------------
// start() with EmoteMod::PREANIM — falls through to TALKING when no preanim
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, StartPreanimWithNoAssetFallsToTalking) {
    // pre_emote is "objecting" but the asset won't be found.
    // preanim_asset is null → frame_count check is skipped → state = TALKING.
    player.start(ao_assets, "Phoenix", "normal", "objecting", EmoteMod::PREANIM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, StartPreanimWithEmptyPreEmoteFallsToTalking) {
    player.start(ao_assets, "Phoenix", "normal", "", EmoteMod::PREANIM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, StartPreanimWithDashPreEmoteFallsToTalking) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::PREANIM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

// ---------------------------------------------------------------------------
// start() with EmoteMod::PREANIM_ZOOM — same fallback as PREANIM
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, StartPreanimZoomWithNoAssetFallsToTalking) {
    player.start(ao_assets, "Phoenix", "normal", "objecting", EmoteMod::PREANIM_ZOOM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

// ---------------------------------------------------------------------------
// start() with EmoteMod::ZOOM — always goes to TALKING
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, StartZoomSetsStateToTalking) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::ZOOM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

// ---------------------------------------------------------------------------
// tick() with no assets loaded — must not crash
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, TickInNoneStateDoesNotCrash) {
    player.tick(16);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::NONE);
}

TEST_F(AOEmotePlayerTest, TickInTalkingFromIdleModDoesNotCrash) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    player.tick(16);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, TickInTalkingStateDoesNotCrash) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::ZOOM);
    player.tick(16);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, MultipleTicksInTalkingFromIdleModDoNotCrash) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    for (int i = 0; i < 100; ++i) {
        player.tick(16);
    }
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

// ---------------------------------------------------------------------------
// current_frame() / current_frame_index() after start with no assets
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, CurrentFrameNullInTalking) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::ZOOM);
    EXPECT_EQ(player.current_frame(), nullptr);
}

TEST_F(AOEmotePlayerTest, CurrentFrameIndexZeroInTalking) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::ZOOM);
    EXPECT_EQ(player.current_frame_index(), 0);
}

TEST_F(AOEmotePlayerTest, CurrentFrameNullInIdle) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.current_frame(), nullptr);
}

TEST_F(AOEmotePlayerTest, CurrentFrameIndexZeroInIdle) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.current_frame_index(), 0);
}

// ---------------------------------------------------------------------------
// Restarting: start() can be called multiple times
// ---------------------------------------------------------------------------

TEST_F(AOEmotePlayerTest, IdleModStartsTalking) {
    // EmoteMod::IDLE means "skip preanim, go to talking" — not idle animation
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}

TEST_F(AOEmotePlayerTest, RestartBetweenCharacters) {
    player.start(ao_assets, "Phoenix", "normal", "-", EmoteMod::ZOOM);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);

    player.start(ao_assets, "Edgeworth", "normal", "-", EmoteMod::IDLE);
    EXPECT_EQ(player.state(), AOEmotePlayer::State::TALKING);
}
