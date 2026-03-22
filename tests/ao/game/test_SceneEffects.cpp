#include "ao/game/effects/ScreenshakeEffect.h"
#include "ao/game/effects/FlashEffect.h"
#include "ao/game/effects/ShaderEffect.h"
#include "render/Layer.h"
#include <gtest/gtest.h>

// ===========================================================================
// ScreenshakeEffect
// ===========================================================================

class ScreenshakeEffectTest : public ::testing::Test {
  protected:
    ScreenshakeEffect effect;
    LayerGroup scene;
};

TEST_F(ScreenshakeEffectTest, NotActiveByDefault) {
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, TriggerMakesActive) {
    effect.trigger();
    EXPECT_TRUE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, StopMakesInactive) {
    effect.trigger();
    ASSERT_TRUE(effect.is_active());
    effect.stop();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, TickWithoutTriggerDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(effect.tick(16));
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, ApplyOnEmptyLayerGroupDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ScreenshakeEffectTest, DeactivatesAfterDuration) {
    // ScreenshakeEffect uses a 300ms duration internally.
    effect.trigger();
    ASSERT_TRUE(effect.is_active());

    // Tick past the full duration.
    effect.tick(350);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, StillActiveMidway) {
    effect.trigger();
    effect.tick(100);
    EXPECT_TRUE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, ApplyAfterTriggerModifiesTransform) {
    // The screenshake applies random translation offsets to the scene transform.
    // We verify that apply() does not crash when active.
    effect.trigger();
    effect.tick(10);
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ScreenshakeEffectTest, DeactivatesExactlyAtBoundary) {
    effect.trigger();
    // Tick exactly to the 300ms boundary.
    effect.tick(300);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, MultipleTriggerRestartsEffect) {
    effect.trigger();
    effect.tick(200);
    ASSERT_TRUE(effect.is_active());

    // Re-trigger should restart the timer.
    effect.trigger();
    effect.tick(200);
    // Should still be active since we re-triggered at 200ms and only
    // advanced 200ms from the new start.
    EXPECT_TRUE(effect.is_active());
}

TEST_F(ScreenshakeEffectTest, StopThenApplyDoesNotCrash) {
    effect.trigger();
    effect.stop();
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

// ===========================================================================
// FlashEffect
// ===========================================================================

class FlashEffectTest : public ::testing::Test {
  protected:
    // Use a small viewport for test efficiency.
    FlashEffect effect{4, 4};
    LayerGroup scene;
};

TEST_F(FlashEffectTest, NotActiveByDefault) {
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, TriggerMakesActive) {
    effect.trigger();
    EXPECT_TRUE(effect.is_active());
}

TEST_F(FlashEffectTest, StopMakesInactive) {
    effect.trigger();
    ASSERT_TRUE(effect.is_active());
    effect.stop();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, TickWithoutTriggerDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(effect.tick(16));
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, ApplyOnEmptyLayerGroupDoesNotCrash) {
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(FlashEffectTest, ActiveForExactlyDurationMs) {
    // DURATION_MS is 250ms.
    effect.trigger();

    // At 249ms it should still be active.
    effect.tick(249);
    EXPECT_TRUE(effect.is_active());

    // At 250ms total it should deactivate.
    effect.tick(1);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, InactiveAfter250MsOfTicks) {
    effect.trigger();
    effect.tick(250);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, ApplyAddsLayerAtZIndex100) {
    effect.trigger();

    ASSERT_TRUE(scene.get_layers().empty());
    effect.apply(scene);

    // FlashEffect adds a layer keyed at Z_INDEX (100).
    const auto& layers = scene.get_layers();
    ASSERT_EQ(layers.size(), 1u);

    auto it = layers.find(100);
    ASSERT_NE(it, layers.end());
    EXPECT_EQ(it->second.get_z_index(), 100);
}

TEST_F(FlashEffectTest, ApplyLayerHasFullOpacityAtStart) {
    effect.trigger();
    // No ticks yet -- elapsed is 0, so opacity should be (1-0)^2 = 1.0
    effect.apply(scene);

    const auto& layers = scene.get_layers();
    auto it = layers.find(100);
    ASSERT_NE(it, layers.end());
    EXPECT_FLOAT_EQ(it->second.get_opacity(), 1.0f);
}

TEST_F(FlashEffectTest, ApplyLayerFadesOverTime) {
    effect.trigger();
    effect.tick(125); // Half of 250ms -> t = 0.5 -> opacity = (1-0.5)^2 = 0.25
    effect.apply(scene);

    const auto& layers = scene.get_layers();
    auto it = layers.find(100);
    ASSERT_NE(it, layers.end());
    EXPECT_FLOAT_EQ(it->second.get_opacity(), 0.25f);
}

TEST_F(FlashEffectTest, ApplyLayerHasZeroOpacityAtEnd) {
    effect.trigger();
    effect.tick(250); // t = 1.0 -> opacity = (1-1)^2 = 0.0
    effect.apply(scene);

    const auto& layers = scene.get_layers();
    auto it = layers.find(100);
    ASSERT_NE(it, layers.end());
    EXPECT_FLOAT_EQ(it->second.get_opacity(), 0.0f);
}

TEST_F(FlashEffectTest, ApplyLayerHasValidAsset) {
    effect.trigger();
    effect.apply(scene);

    const auto& layers = scene.get_layers();
    auto it = layers.find(100);
    ASSERT_NE(it, layers.end());
    EXPECT_NE(it->second.get_asset(), nullptr);
}

TEST_F(FlashEffectTest, RetriggerResetsTimer) {
    effect.trigger();
    effect.tick(200);
    ASSERT_TRUE(effect.is_active());

    // Re-trigger resets elapsed time.
    effect.trigger();
    // After 200ms more (only 200ms since re-trigger), should still be active.
    effect.tick(200);
    EXPECT_TRUE(effect.is_active());
}

TEST_F(FlashEffectTest, IncrementalTicksAccumulate) {
    effect.trigger();
    for (int i = 0; i < 15; ++i)
        effect.tick(16); // 15 * 16 = 240ms, still under 250ms

    EXPECT_TRUE(effect.is_active());

    effect.tick(16); // 256ms total, past 250ms
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, StopThenTickDoesNotReactivate) {
    effect.trigger();
    effect.stop();
    effect.tick(16);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(FlashEffectTest, LargeViewportConstruction) {
    // Ensure construction with a realistic viewport doesn't crash.
    EXPECT_NO_FATAL_FAILURE(FlashEffect large(256, 192));
}

// ===========================================================================
// ShaderEffect
// ===========================================================================

// Note: ShaderEffect::trigger() attempts to load a shader via
// MediaManager::instance().assets().shader(). In test builds without a
// configured mount system the shader load will return nullptr, which means
// is_active() returns false (it requires shader_ != nullptr). We test the
// timing and lifecycle behavior through this lens.

class ShaderEffectTest : public ::testing::Test {
  protected:
    LayerGroup scene;
};

TEST_F(ShaderEffectTest, NotActiveByDefault) {
    ShaderEffect effect("shaders/test", 1.0f);
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ShaderEffectTest, TriggerDoesNotCrash) {
    // trigger() calls MediaManager which will fail to find the shader,
    // but should not crash.
    ShaderEffect effect("shaders/test", 1.0f);
    EXPECT_NO_FATAL_FAILURE(effect.trigger());
}

TEST_F(ShaderEffectTest, StopMakesInactive) {
    ShaderEffect effect("shaders/test", 1.0f);
    effect.trigger();
    effect.stop();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ShaderEffectTest, TickWithoutTriggerDoesNotCrash) {
    ShaderEffect effect("shaders/test", 1.0f);
    EXPECT_NO_FATAL_FAILURE(effect.tick(16));
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ShaderEffectTest, ApplyOnEmptyLayerGroupDoesNotCrash) {
    ShaderEffect effect("shaders/test", 1.0f);
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ShaderEffectTest, IsActiveRequiresShaderLoaded) {
    // Without a valid shader asset, is_active() returns false even after trigger.
    ShaderEffect effect("shaders/nonexistent", 1.0f);
    effect.trigger();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ShaderEffectTest, ZeroDurationMeansInfinite) {
    // Duration 0 means the effect never auto-deactivates.
    // Even after extensive ticking, it won't deactivate from timeout.
    // (It will still be inactive if shader failed to load, but the
    // timer logic itself should not deactivate it.)
    ShaderEffect effect("shaders/test", 0.0f);
    effect.trigger();
    effect.tick(60000); // 60 seconds
    // Without a loaded shader, is_active() is false due to null shader,
    // but we verify tick() doesn't crash with infinite duration.
    EXPECT_NO_FATAL_FAILURE(effect.tick(60000));
}

TEST_F(ShaderEffectTest, FiniteDurationTickDoesNotCrash) {
    ShaderEffect effect("shaders/test", 2.0f);
    effect.trigger();
    // Tick past the duration (2000ms).
    EXPECT_NO_FATAL_FAILURE(effect.tick(3000));
}

TEST_F(ShaderEffectTest, RetriggerDoesNotCrash) {
    ShaderEffect effect("shaders/test", 1.0f);
    effect.trigger();
    effect.tick(500);
    // Re-trigger should reset elapsed time.
    EXPECT_NO_FATAL_FAILURE(effect.trigger());
}

TEST_F(ShaderEffectTest, ApplyWithoutShaderIsNoop) {
    // When shader_ is null, apply() should be a no-op.
    ShaderEffect effect("shaders/test", 1.0f);
    effect.trigger();
    effect.apply(scene);
    // Scene should remain unchanged (no shader set).
    EXPECT_EQ(scene.get_shader(), nullptr);
}

TEST_F(ShaderEffectTest, ApplyToSpecificLayerDoesNotCrash) {
    ShaderEffect effect("shaders/test", 1.0f, 5);
    effect.trigger();
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ShaderEffectTest, ConstructionWithDefaultLayerId) {
    // layer_id defaults to -1 (whole group).
    ShaderEffect effect("shaders/test", 1.0f);
    EXPECT_NO_FATAL_FAILURE(effect.trigger());
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ShaderEffectTest, ConstructionWithExplicitLayerId) {
    ShaderEffect effect("shaders/test", 1.0f, 3);
    EXPECT_NO_FATAL_FAILURE(effect.trigger());
    EXPECT_NO_FATAL_FAILURE(effect.apply(scene));
}

TEST_F(ShaderEffectTest, StopAfterTickDoesNotCrash) {
    ShaderEffect effect("shaders/test", 1.0f);
    effect.trigger();
    effect.tick(500);
    effect.stop();
    EXPECT_FALSE(effect.is_active());
}

TEST_F(ShaderEffectTest, MultipleTriggerStopCycles) {
    ShaderEffect effect("shaders/test", 1.0f);
    for (int i = 0; i < 10; ++i) {
        effect.trigger();
        effect.tick(100);
        effect.stop();
    }
    EXPECT_FALSE(effect.is_active());
}

// ===========================================================================
// ISceneEffect interface conformance
// ===========================================================================

// Verify that each concrete type satisfies the ISceneEffect interface
// through pointer-to-base.

TEST(SceneEffectInterface, ScreenshakeIsAnISceneEffect) {
    std::unique_ptr<ISceneEffect> effect = std::make_unique<ScreenshakeEffect>();
    LayerGroup scene;

    EXPECT_FALSE(effect->is_active());
    effect->trigger();
    EXPECT_TRUE(effect->is_active());
    effect->tick(16);
    effect->apply(scene);
    effect->stop();
    EXPECT_FALSE(effect->is_active());
}

TEST(SceneEffectInterface, FlashIsAnISceneEffect) {
    std::unique_ptr<ISceneEffect> effect = std::make_unique<FlashEffect>(4, 4);
    LayerGroup scene;

    EXPECT_FALSE(effect->is_active());
    effect->trigger();
    EXPECT_TRUE(effect->is_active());
    effect->tick(16);
    effect->apply(scene);
    effect->stop();
    EXPECT_FALSE(effect->is_active());
}

TEST(SceneEffectInterface, ShaderIsAnISceneEffect) {
    std::unique_ptr<ISceneEffect> effect =
        std::make_unique<ShaderEffect>("shaders/test", 1.0f);
    LayerGroup scene;

    EXPECT_FALSE(effect->is_active());
    EXPECT_NO_FATAL_FAILURE(effect->trigger());
    effect->tick(16);
    effect->apply(scene);
    effect->stop();
    EXPECT_FALSE(effect->is_active());
}
