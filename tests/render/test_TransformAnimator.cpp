#include "render/Transform.h"
#include "render/TransformAnimator.h"

#include <cmath>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr float kEpsilon = 1e-4f;

static void expect_vec2_near(Vec2 actual, Vec2 expected, float eps = kEpsilon) {
    EXPECT_NEAR(actual.x, expected.x, eps) << "Vec2.x mismatch";
    EXPECT_NEAR(actual.y, expected.y, eps) << "Vec2.y mismatch";
}

// ---------------------------------------------------------------------------
// Fixture: save/restore Transform's static aspect ratio
// ---------------------------------------------------------------------------

class TransformAnimatorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        saved_aspect_ = Transform::get_aspect_ratio();
        Transform::set_aspect_ratio(1.0f);
    }

    void TearDown() override {
        Transform::set_aspect_ratio(saved_aspect_);
    }

    /// Build a simple two-keyframe animator: kf0 at t=0, kf1 at t=duration_ms.
    TransformAnimator make_simple(int duration_ms, Vec2 start_pos, Vec2 end_pos, Vec2 start_scale, Vec2 end_scale,
                                  float start_rot, float end_rot) {
        TransformAnimator a;
        a.add_keyframe({0, start_pos, start_scale, start_rot});
        a.add_keyframe({duration_ms, end_pos, end_scale, end_rot});
        return a;
    }

  private:
    float saved_aspect_ = 1.0f;
};

// ===========================================================================
// 1. Default / initial state
// ===========================================================================

TEST_F(TransformAnimatorTest, DefaultStateNotPlayingNotFinished) {
    TransformAnimator a;
    EXPECT_FALSE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
}

TEST_F(TransformAnimatorTest, DefaultCurrentValuesAreZeroIdentity) {
    TransformAnimator a;
    expect_vec2_near(a.current_translation(), {0, 0});
    expect_vec2_near(a.current_scale(), {1, 1});
    EXPECT_NEAR(a.current_rotation(), 0.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, TickWithNoKeyframesReturnsFalse) {
    TransformAnimator a;
    a.play();
    EXPECT_FALSE(a.tick(16));
}

TEST_F(TransformAnimatorTest, TickWithSingleKeyframeReturnsFalse) {
    TransformAnimator a;
    a.add_keyframe({0, {1, 2}, {3, 4}, 45.0f});
    a.play();
    // Needs at least 2 keyframes to interpolate.
    EXPECT_FALSE(a.tick(16));
}

// ===========================================================================
// 2. Starting an animation
// ===========================================================================

TEST_F(TransformAnimatorTest, PlaySetsPlaying) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    EXPECT_TRUE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
}

TEST_F(TransformAnimatorTest, StopClearsPlaying) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.stop();
    EXPECT_FALSE(a.is_playing());
}

TEST_F(TransformAnimatorTest, ResetClearsPlayingAndFinished) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(200); // finishes
    EXPECT_TRUE(a.is_finished());
    EXPECT_FALSE(a.is_playing());

    a.reset();
    EXPECT_FALSE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
}

TEST_F(TransformAnimatorTest, ResetRestoresFirstKeyframe) {
    TransformAnimator a = make_simple(100, {5, 7}, {10, 20}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(100); // finishes at end keyframe
    a.reset();

    // After reset, current should be the first keyframe.
    expect_vec2_near(a.current_translation(), {5, 7});
    expect_vec2_near(a.current_scale(), {1, 1});
    EXPECT_NEAR(a.current_rotation(), 0.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, TickWhileStoppedReturnsFalse) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    // Not yet playing.
    EXPECT_FALSE(a.tick(16));
}

// ===========================================================================
// 3. Ticking the animation forward
// ===========================================================================

TEST_F(TransformAnimatorTest, TickReturnsTrue) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    EXPECT_TRUE(a.tick(50));
}

TEST_F(TransformAnimatorTest, TickAdvancesTranslation) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    a.tick(50); // 50% through with LINEAR easing
    expect_vec2_near(a.current_translation(), {5, 0});
}

TEST_F(TransformAnimatorTest, TickAdvancesScale) {
    TransformAnimator a = make_simple(100, {0, 0}, {0, 0}, {1, 1}, {3, 3}, 0, 0);
    a.play();
    a.tick(50);
    expect_vec2_near(a.current_scale(), {2, 2});
}

TEST_F(TransformAnimatorTest, TickAdvancesRotation) {
    TransformAnimator a = make_simple(100, {0, 0}, {0, 0}, {1, 1}, {1, 1}, 0, 90);
    a.play();
    a.tick(50);
    EXPECT_NEAR(a.current_rotation(), 45.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, MultipleTicks) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    a.tick(25);
    expect_vec2_near(a.current_translation(), {25, 0});
    a.tick(25);
    expect_vec2_near(a.current_translation(), {50, 0});
    a.tick(25);
    expect_vec2_near(a.current_translation(), {75, 0});
}

// ===========================================================================
// 4. Completion detection
// ===========================================================================

TEST_F(TransformAnimatorTest, FinishesAtExactDuration) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(100);
    EXPECT_TRUE(a.is_finished());
    EXPECT_FALSE(a.is_playing());
}

TEST_F(TransformAnimatorTest, FinishesWhenOvershootingDuration) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(200);
    EXPECT_TRUE(a.is_finished());
    EXPECT_FALSE(a.is_playing());
}

TEST_F(TransformAnimatorTest, FinalValuesMatchLastKeyframe) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 20}, {1, 1}, {3, 4}, 0, 90);
    a.play();
    a.tick(100);
    expect_vec2_near(a.current_translation(), {10, 20});
    expect_vec2_near(a.current_scale(), {3, 4});
    EXPECT_NEAR(a.current_rotation(), 90.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, TickAfterFinishedReturnsFalse) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 0);
    a.play();
    a.tick(100);
    EXPECT_TRUE(a.is_finished());
    EXPECT_FALSE(a.tick(50)); // already finished
}

TEST_F(TransformAnimatorTest, PlayAfterFinishedResumesFromEnd) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 0);
    a.play();
    a.tick(100);
    EXPECT_TRUE(a.is_finished());

    // play() clears finished and sets playing, but doesn't reset elapsed
    a.play();
    EXPECT_TRUE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
    // Ticking further should immediately finish again (elapsed is still >= total).
    a.tick(0);
    EXPECT_TRUE(a.is_finished());
}

// ===========================================================================
// 5. Interpolation correctness
// ===========================================================================

TEST_F(TransformAnimatorTest, LinearInterpolationQuarterPoints) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 200}, {1, 1}, {5, 5}, 0, 360);
    a.set_easing(Easing::LINEAR);
    a.play();

    a.tick(25);
    expect_vec2_near(a.current_translation(), {25, 50});
    expect_vec2_near(a.current_scale(), {2, 2});
    EXPECT_NEAR(a.current_rotation(), 90.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, QuadInEasing) {
    // QUAD_IN: ease(t) = t*t
    // At t=0.5 (50ms of 100ms), ease = 0.25
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_easing(Easing::QUAD_IN);
    a.play();
    a.tick(50);
    // t=0.5, eased = 0.25, translation.x = lerp(0, 100, 0.25) = 25
    expect_vec2_near(a.current_translation(), {25, 0});
}

TEST_F(TransformAnimatorTest, QuadOutEasing) {
    // QUAD_OUT: ease(t) = t * (2 - t)
    // At t=0.5, ease = 0.5*(2-0.5) = 0.75
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_easing(Easing::QUAD_OUT);
    a.play();
    a.tick(50);
    expect_vec2_near(a.current_translation(), {75, 0});
}

TEST_F(TransformAnimatorTest, QuadInOutEasingFirstHalf) {
    // QUAD_IN_OUT: t < 0.5 => 2*t*t
    // At t=0.25 (25ms of 100ms): ease = 2*0.25*0.25 = 0.125
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_easing(Easing::QUAD_IN_OUT);
    a.play();
    a.tick(25);
    expect_vec2_near(a.current_translation(), {12.5f, 0});
}

TEST_F(TransformAnimatorTest, QuadInOutEasingSecondHalf) {
    // QUAD_IN_OUT: t >= 0.5 => -1 + (4 - 2*t)*t
    // At t=0.75 (75ms of 100ms): ease = -1 + (4 - 1.5)*0.75 = -1 + 1.875 = 0.875
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_easing(Easing::QUAD_IN_OUT);
    a.play();
    a.tick(75);
    expect_vec2_near(a.current_translation(), {87.5f, 0});
}

TEST_F(TransformAnimatorTest, EasingEndpointsAreExact) {
    // All easing functions should map 0->0 and 1->1.
    // At t=0, values should be the start keyframe.
    // At t=total, values should be the end keyframe.
    for (auto easing : {Easing::LINEAR, Easing::QUAD_IN, Easing::QUAD_OUT, Easing::QUAD_IN_OUT}) {
        TransformAnimator a = make_simple(100, {0, 0}, {50, 50}, {1, 1}, {3, 3}, 0, 180);
        a.set_easing(easing);
        a.play();
        a.tick(100);
        expect_vec2_near(a.current_translation(), {50, 50});
        expect_vec2_near(a.current_scale(), {3, 3});
        EXPECT_NEAR(a.current_rotation(), 180.0f, kEpsilon);
    }
}

// ===========================================================================
// 5b. Multi-keyframe interpolation
// ===========================================================================

TEST_F(TransformAnimatorTest, ThreeKeyframesFirstSegment) {
    TransformAnimator a;
    a.add_keyframe({0, {0, 0}, {1, 1}, 0});
    a.add_keyframe({100, {10, 0}, {1, 1}, 0});
    a.add_keyframe({200, {10, 20}, {1, 1}, 0});
    a.play();

    a.tick(50); // 50ms into [0..100] segment
    expect_vec2_near(a.current_translation(), {5, 0});
}

TEST_F(TransformAnimatorTest, ThreeKeyframesSecondSegment) {
    TransformAnimator a;
    a.add_keyframe({0, {0, 0}, {1, 1}, 0});
    a.add_keyframe({100, {10, 0}, {1, 1}, 0});
    a.add_keyframe({200, {10, 20}, {1, 1}, 0});
    a.play();

    a.tick(150); // 50ms into [100..200] segment
    // t = (150-100) / (200-100) = 0.5
    // lerp({10,0}, {10,20}, 0.5) = {10, 10}
    expect_vec2_near(a.current_translation(), {10, 10});
}

TEST_F(TransformAnimatorTest, ThreeKeyframesCompletion) {
    TransformAnimator a;
    a.add_keyframe({0, {0, 0}, {1, 1}, 0});
    a.add_keyframe({100, {10, 0}, {1, 1}, 0});
    a.add_keyframe({200, {10, 20}, {1, 1}, 0});
    a.play();

    a.tick(200);
    EXPECT_TRUE(a.is_finished());
    expect_vec2_near(a.current_translation(), {10, 20});
}

TEST_F(TransformAnimatorTest, KeyframesAddedOutOfOrderAreSorted) {
    TransformAnimator a;
    // Add in reverse order -- should still work because add_keyframe sorts.
    a.add_keyframe({200, {20, 0}, {1, 1}, 0});
    a.add_keyframe({0, {0, 0}, {1, 1}, 0});
    a.add_keyframe({100, {10, 0}, {1, 1}, 0});
    a.play();

    a.tick(50);
    expect_vec2_near(a.current_translation(), {5, 0});

    a.tick(100); // elapsed = 150
    expect_vec2_near(a.current_translation(), {15, 0});
}

// ===========================================================================
// 6. Edge cases
// ===========================================================================

TEST_F(TransformAnimatorTest, ZeroDeltaTick) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    EXPECT_TRUE(a.tick(0));
    expect_vec2_near(a.current_translation(), {0, 0});
}

TEST_F(TransformAnimatorTest, ClearKeyframesResetsState) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(50);
    a.clear_keyframes();

    EXPECT_FALSE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
    // After clearing, tick should return false (no keyframes).
    a.play();
    EXPECT_FALSE(a.tick(16));
}

TEST_F(TransformAnimatorTest, NegativeTranslationValues) {
    TransformAnimator a = make_simple(100, {-10, -20}, {10, 20}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    a.tick(50);
    expect_vec2_near(a.current_translation(), {0, 0});
    a.tick(50);
    expect_vec2_near(a.current_translation(), {10, 20});
}

TEST_F(TransformAnimatorTest, NegativeRotation) {
    TransformAnimator a = make_simple(100, {0, 0}, {0, 0}, {1, 1}, {1, 1}, -90, 90);
    a.play();
    a.tick(50);
    EXPECT_NEAR(a.current_rotation(), 0.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, VeryLargeDelta) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {2, 2}, 0, 90);
    a.play();
    a.tick(999999);
    EXPECT_TRUE(a.is_finished());
    expect_vec2_near(a.current_translation(), {10, 0});
}

// ===========================================================================
// 7. Looping
// ===========================================================================

TEST_F(TransformAnimatorTest, LoopingDoesNotFinish) {
    TransformAnimator a = make_simple(100, {0, 0}, {10, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_looping(true);
    a.play();
    a.tick(100);
    EXPECT_FALSE(a.is_finished());
    EXPECT_TRUE(a.is_playing());
}

TEST_F(TransformAnimatorTest, LoopingWrapsElapsedTime) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_looping(true);
    a.play();

    // Tick 150ms: elapsed wraps to 50ms (150 % 100 = 50)
    a.tick(150);
    expect_vec2_near(a.current_translation(), {50, 0});
}

TEST_F(TransformAnimatorTest, LoopingMultipleCycles) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.set_looping(true);
    a.play();

    // Complete two full cycles and land at 25%.
    a.tick(225);
    // 225 % 100 = 25
    expect_vec2_near(a.current_translation(), {25, 0});
    EXPECT_TRUE(a.is_playing());
    EXPECT_FALSE(a.is_finished());
}

// ===========================================================================
// 8. apply() writes to a Transform
// ===========================================================================

TEST_F(TransformAnimatorTest, ApplyWritesToTransform) {
    TransformAnimator a = make_simple(100, {0, 0}, {5, 10}, {1, 1}, {2, 3}, 0, 0);
    a.play();
    a.tick(100); // land on final keyframe

    Transform t;
    a.apply(t);

    Mat4 m = t.get_local_transform();
    // Translation should be in column 3.
    EXPECT_NEAR(m[3][0], 5.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 10.0f, kEpsilon);
    // Scale should be on diagonal.
    EXPECT_NEAR(m[0][0], 2.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 3.0f, kEpsilon);
}

TEST_F(TransformAnimatorTest, ApplyWithRotation) {
    TransformAnimator a = make_simple(100, {0, 0}, {0, 0}, {1, 1}, {1, 1}, 0, 90);
    a.play();
    a.tick(100);

    Transform t;
    a.apply(t);

    Mat4 m = t.get_local_transform();
    float c = std::cos(90.0f * 3.14159265358979323846f / 180.0f);
    float s = std::sin(90.0f * 3.14159265358979323846f / 180.0f);
    EXPECT_NEAR(m[0][0], c, kEpsilon);
    EXPECT_NEAR(m[0][1], s, kEpsilon);
}

// ===========================================================================
// 9. Interaction between play/stop/reset
// ===========================================================================

TEST_F(TransformAnimatorTest, StopThenPlayResumes) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    a.tick(30);
    expect_vec2_near(a.current_translation(), {30, 0});

    a.stop();
    EXPECT_FALSE(a.tick(20)); // stopped, no advance

    a.play();
    a.tick(20); // resumes from elapsed=30, now 50
    expect_vec2_near(a.current_translation(), {50, 0});
}

TEST_F(TransformAnimatorTest, ResetThenPlayRestartsFromBeginning) {
    TransformAnimator a = make_simple(100, {0, 0}, {100, 0}, {1, 1}, {1, 1}, 0, 0);
    a.play();
    a.tick(80);
    expect_vec2_near(a.current_translation(), {80, 0});

    a.reset(); // elapsed back to 0
    a.play();
    a.tick(20);
    expect_vec2_near(a.current_translation(), {20, 0});
}
