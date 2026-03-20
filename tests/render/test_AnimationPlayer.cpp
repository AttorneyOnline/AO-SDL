#include "asset/ImageAsset.h"
#include "render/AnimationPlayer.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static auto make_asset(int frames, int duration_ms = 100) {
    std::vector<ImageFrame> f;
    for (int i = 0; i < frames; i++) {
        f.push_back(ImageFrame{std::vector<uint8_t>(4, (uint8_t)i), 1, 1, duration_ms});
    }
    return std::make_shared<ImageAsset>("test", "png", std::move(f));
}

// ---------------------------------------------------------------------------
// Empty / default state
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, EmptyPlayerHasNoFrame) {
    AnimationPlayer player;
    EXPECT_FALSE(player.has_frame());
    EXPECT_FALSE(player.finished());
    EXPECT_EQ(player.current_frame(), nullptr);
    EXPECT_EQ(player.current_frame_index(), 0);
}

// ---------------------------------------------------------------------------
// Single-frame looping
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, SingleFrameLoopingHasFrame) {
    AnimationPlayer player;
    player.load(make_asset(1), true);
    EXPECT_TRUE(player.has_frame());
}

TEST(AnimationPlayer, SingleFrameLoopingNeverFinishes) {
    AnimationPlayer player;
    player.load(make_asset(1, 100), true);
    player.tick(200);
    EXPECT_FALSE(player.finished());
    EXPECT_EQ(player.current_frame_index(), 0);
}

TEST(AnimationPlayer, SingleFrameLoopingStaysOnFrame0) {
    AnimationPlayer player;
    player.load(make_asset(1), true);
    player.tick(500);
    EXPECT_EQ(player.current_frame_index(), 0);
    EXPECT_NE(player.current_frame(), nullptr);
}

// ---------------------------------------------------------------------------
// Single-frame one-shot
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, SingleFrameOneShotNotFinishedImmediately) {
    AnimationPlayer player;
    player.load(make_asset(1, 100), false);
    EXPECT_FALSE(player.finished());
}

TEST(AnimationPlayer, SingleFrameOneShotFinishesAfterDuration) {
    AnimationPlayer player;
    player.load(make_asset(1, 100), false);
    player.tick(100);
    EXPECT_TRUE(player.finished());
}

TEST(AnimationPlayer, SingleFrameOneShotNotFinishedBeforeDuration) {
    AnimationPlayer player;
    player.load(make_asset(1, 200), false);
    player.tick(100);
    EXPECT_FALSE(player.finished());
    player.tick(100);
    EXPECT_TRUE(player.finished());
}

// ---------------------------------------------------------------------------
// Multi-frame looping
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, MultiFrameLoopingAdvancesFrame) {
    AnimationPlayer player;
    player.load(make_asset(3, 100), true);
    EXPECT_EQ(player.current_frame_index(), 0);

    player.tick(100);
    EXPECT_EQ(player.current_frame_index(), 1);

    player.tick(100);
    EXPECT_EQ(player.current_frame_index(), 2);
}

TEST(AnimationPlayer, MultiFrameLoopingWrapsAround) {
    AnimationPlayer player;
    player.load(make_asset(3, 100), true);

    player.tick(100); // -> frame 1
    player.tick(100); // -> frame 2
    player.tick(100); // -> frame 0 (wrap)
    EXPECT_EQ(player.current_frame_index(), 0);
    EXPECT_FALSE(player.finished());
}

TEST(AnimationPlayer, MultiFrameLoopingNeverFinishes) {
    AnimationPlayer player;
    player.load(make_asset(2, 50), true);
    // Tick enough to wrap several times.
    for (int i = 0; i < 20; i++) {
        player.tick(50);
    }
    EXPECT_FALSE(player.finished());
}

// ---------------------------------------------------------------------------
// Multi-frame one-shot
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, MultiFrameOneShotAdvancesToLastAndFinishes) {
    AnimationPlayer player;
    player.load(make_asset(3, 100), false);

    player.tick(100); // -> frame 1
    EXPECT_FALSE(player.finished());

    player.tick(100); // -> frame 2
    EXPECT_FALSE(player.finished());

    player.tick(100); // past last frame -> done
    EXPECT_TRUE(player.finished());
    EXPECT_EQ(player.current_frame_index(), 2);
}

TEST(AnimationPlayer, MultiFrameOneShotStopsAtLastFrame) {
    AnimationPlayer player;
    player.load(make_asset(3, 100), false);

    // Tick a large delta that overshoots everything.
    player.tick(500);
    EXPECT_TRUE(player.finished());
    EXPECT_EQ(player.current_frame_index(), 2);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, ClearResetsState) {
    AnimationPlayer player;
    player.load(make_asset(3, 100), true);
    player.tick(150);
    EXPECT_TRUE(player.has_frame());

    player.clear();
    EXPECT_FALSE(player.has_frame());
    EXPECT_FALSE(player.finished());
    EXPECT_EQ(player.current_frame_index(), 0);
    EXPECT_EQ(player.current_frame(), nullptr);
    EXPECT_EQ(player.asset(), nullptr);
}

// ---------------------------------------------------------------------------
// Frame duration respected
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, SmallDeltaStaysOnSameFrame) {
    AnimationPlayer player;
    player.load(make_asset(3, 200), true);

    player.tick(50);
    EXPECT_EQ(player.current_frame_index(), 0);

    player.tick(50);
    EXPECT_EQ(player.current_frame_index(), 0);

    // Total 100ms still < 200ms duration.
    player.tick(50);
    EXPECT_EQ(player.current_frame_index(), 0);

    // Now 200ms total -> advance.
    player.tick(50);
    EXPECT_EQ(player.current_frame_index(), 1);
}

TEST(AnimationPlayer, LargeDeltaSkipsFrames) {
    AnimationPlayer player;
    player.load(make_asset(4, 100), true);

    // 250ms should skip past frames 0 and 1, landing on frame 2.
    player.tick(250);
    EXPECT_EQ(player.current_frame_index(), 2);
}

// ---------------------------------------------------------------------------
// current_frame_index
// ---------------------------------------------------------------------------

TEST(AnimationPlayer, CurrentFrameIndexReturnsCorrectValue) {
    AnimationPlayer player;
    player.load(make_asset(5, 100), true);

    for (int expected = 0; expected < 5; expected++) {
        EXPECT_EQ(player.current_frame_index(), expected);
        player.tick(100);
    }
    // Should have wrapped back to 0.
    EXPECT_EQ(player.current_frame_index(), 0);
}

TEST(AnimationPlayer, CurrentFrameReturnsValidPointer) {
    AnimationPlayer player;
    player.load(make_asset(2, 100), true);

    const ImageFrame* frame = player.current_frame();
    ASSERT_NE(frame, nullptr);
    EXPECT_EQ(frame->width, 1);
    EXPECT_EQ(frame->height, 1);
    EXPECT_EQ(frame->pixels.size(), 4u);
}
