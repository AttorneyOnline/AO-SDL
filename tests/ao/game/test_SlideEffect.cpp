#include "ao/game/effects/SlideEffect.h"

#include "render/Layer.h"

#include <gtest/gtest.h>

class SlideEffectTest : public ::testing::Test {
  protected:
    SlideEffect effect;
    LayerGroup departing;
    LayerGroup arriving;
};

TEST_F(SlideEffectTest, InactiveByDefault) {
    EXPECT_FALSE(effect.is_active());
}

TEST_F(SlideEffectTest, ActiveAfterTrigger) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    EXPECT_TRUE(effect.is_active());
}

TEST_F(SlideEffectTest, StopMakesInactive) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.stop();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(SlideEffectTest, TickWithoutTriggerDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(effect.tick(16));
    EXPECT_FALSE(effect.is_active());
}

TEST_F(SlideEffectTest, ActiveDuringPreDelay) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(100); // < 300ms bookend
    EXPECT_TRUE(effect.is_active());
}

TEST_F(SlideEffectTest, ActiveDuringSlide) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(350); // past 300ms pre-delay, into slide
    EXPECT_TRUE(effect.is_active());
}

TEST_F(SlideEffectTest, InactiveAfterFullDuration) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(300); // pre-delay done
    effect.tick(500); // slide done
    effect.tick(300); // post-delay done
    EXPECT_FALSE(effect.is_active());
}

TEST_F(SlideEffectTest, PreDelayArrivingIsOffscreen) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(100); // still in pre-delay

    effect.apply_in(arriving);
    auto mat = arriving.transform().get_local_transform();
    EXPECT_FLOAT_EQ(mat.m[12], 2.0f); // off-screen right
}

TEST_F(SlideEffectTest, PreDelayDepartingIsAtOrigin) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(100); // still in pre-delay

    effect.apply_out(departing);
    // No transform applied during pre-delay — departing stays at origin
    auto mat = departing.transform().get_local_transform();
    EXPECT_FLOAT_EQ(mat.m[12], 0.0f);
}

TEST_F(SlideEffectTest, SlidePhaseMovesBothGroups) {
    effect.configure(SlideEffect::Direction::LEFT, 500);
    effect.trigger();
    effect.tick(550); // 300ms pre-delay + 250ms into slide (halfway)

    effect.apply_out(departing);
    effect.apply_in(arriving);

    float out_x = departing.transform().get_local_transform().m[12];
    float in_x = arriving.transform().get_local_transform().m[12];
    // Departing should be moving left (negative)
    EXPECT_LT(out_x, 0.0f);
    // Arriving should be between 2.0 and 0 (moving in from right)
    EXPECT_GT(in_x, 0.0f);
    EXPECT_LT(in_x, 2.0f);
}
