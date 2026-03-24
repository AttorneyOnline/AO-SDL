#include "ao/game/AOSceneCompositor.h"

#include "ao/game/AOBackground.h"
#include "ao/game/AOTextBox.h"
#include "ao/game/ActiveICState.h"
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"
#include "asset/MountManager.h"
#include "render/RenderState.h"

#include <gtest/gtest.h>

namespace {

// Create a minimal 1x1 RGBA ImageAsset for testing
static std::shared_ptr<ImageAsset> make_test_image(const std::string& name) {
    DecodedFrame frame;
    frame.pixels = {255, 0, 0, 255}; // 1x1 red pixel
    frame.width = 1;
    frame.height = 1;
    frame.duration_ms = 0;
    std::vector<DecodedFrame> frames = {frame};
    return std::make_shared<ImageAsset>(name, "png", std::move(frames));
}

class AOSceneCompositorTest : public ::testing::Test {
  protected:
    AOSceneCompositor compositor;
    AOBackground background;
    MountManager mounts;
    AssetLibrary engine_assets{mounts, 0};
    AOAssetLibrary ao_assets{engine_assets};
    AOTextBox textbox;

    void SetUp() override {
        textbox.load(ao_assets);
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Normal mode (no departing IC)
// ---------------------------------------------------------------------------

TEST_F(AOSceneCompositorTest, EmptySceneProducesOneGroup) {
    auto state = compositor.compose(background, nullptr, nullptr, textbox, 0);
    EXPECT_EQ(state.get_layer_groups().size(), 1u);
}

TEST_F(AOSceneCompositorTest, NoBackgroundNoICProducesEmptyScene) {
    auto state = compositor.compose(background, nullptr, nullptr, textbox, 0);
    auto* scene_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(scene_ptr, nullptr);
    auto& scene = *scene_ptr;
    EXPECT_TRUE(scene.get_layers().empty());
}

TEST_F(AOSceneCompositorTest, ActiveICWithNoFrameAddsNoCharLayer) {
    ActiveICState ic;
    auto state = compositor.compose(background, &ic, nullptr, textbox, 0);
    auto* scene_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(scene_ptr, nullptr);
    auto& scene = *scene_ptr;
    EXPECT_TRUE(scene.get_layers().empty());
}

TEST_F(AOSceneCompositorTest, ActiveICWithDeskButNoAssetAddsNoDeskLayer) {
    ActiveICState ic;
    ic.show_desk = true;
    auto state = compositor.compose(background, &ic, nullptr, textbox, 0);
    auto* scene_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(scene_ptr, nullptr);
    auto& scene = *scene_ptr;
    EXPECT_TRUE(scene.get_layers().empty());
}

TEST_F(AOSceneCompositorTest, MutableLayerGroupAccessible) {
    auto state = compositor.compose(background, nullptr, nullptr, textbox, 0);
    EXPECT_NE(state.get_mutable_layer_group(0), nullptr);
}

TEST_F(AOSceneCompositorTest, MutableLayerGroupNullForMissingId) {
    auto state = compositor.compose(background, nullptr, nullptr, textbox, 0);
    EXPECT_EQ(state.get_mutable_layer_group(999), nullptr);
}

TEST_F(AOSceneCompositorTest, TextboxInactiveAddsNoTextLayers) {
    // Default textbox is INACTIVE
    ActiveICState ic;
    auto state = compositor.compose(background, &ic, nullptr, textbox, 0);
    auto* scene_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(scene_ptr, nullptr);
    auto& scene = *scene_ptr;
    // No text layers (z=20, 21, 25) should exist
    EXPECT_EQ(scene.get_layer(20), nullptr);
    EXPECT_EQ(scene.get_layer(21), nullptr);
    EXPECT_EQ(scene.get_layer(25), nullptr);
}

TEST_F(AOSceneCompositorTest, TextboxTickingComposesWithoutCrash) {
    textbox.start_message("Phoenix", "Hello world", 0, ao_assets.text_colors());
    ASSERT_EQ(textbox.text_state(), AOTextBox::TextState::TICKING);

    ActiveICState ic;
    auto state = compositor.compose(background, &ic, nullptr, textbox, 1.0f);
    EXPECT_EQ(state.get_layer_groups().size(), 1u);
}

// ---------------------------------------------------------------------------
// Slide mode (departing IC present)
// ---------------------------------------------------------------------------

TEST_F(AOSceneCompositorTest, DepartingICProducesTwoGroups) {
    ActiveICState departing;
    ActiveICState arriving;
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);
    EXPECT_EQ(state.get_layer_groups().size(), 2u);
}

TEST_F(AOSceneCompositorTest, DepartingGroupUsesSnapshotBg) {
    ActiveICState departing;
    departing.bg_asset = make_test_image("old_bg");

    ActiveICState arriving;
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);

    // Group 0 (departing) should have a layer from the snapshotted bg
    auto* dep_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(dep_ptr, nullptr);
    auto& dep_group = *dep_ptr;
    EXPECT_NE(dep_group.get_layer(0), nullptr); // bg at z=0
}

TEST_F(AOSceneCompositorTest, DepartingGroupUsesSnapshotDesk) {
    ActiveICState departing;
    departing.bg_asset = make_test_image("old_bg");
    departing.desk_asset = make_test_image("old_desk");
    departing.show_desk = true;

    ActiveICState arriving;
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);

    auto* dep_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(dep_ptr, nullptr);
    auto& dep_group = *dep_ptr;
    EXPECT_NE(dep_group.get_layer(10), nullptr); // desk at z=10
}

TEST_F(AOSceneCompositorTest, DepartingGroupNoDeskWhenHidden) {
    ActiveICState departing;
    departing.bg_asset = make_test_image("old_bg");
    departing.desk_asset = make_test_image("old_desk");
    departing.show_desk = false;

    ActiveICState arriving;
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);

    auto* dep_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(dep_ptr, nullptr);
    auto& dep_group = *dep_ptr;
    EXPECT_EQ(dep_group.get_layer(10), nullptr);
}

TEST_F(AOSceneCompositorTest, ArrivingGroupUsesLiveBackground) {
    // departing has a snapshot, arriving should use the shared background
    ActiveICState departing;
    departing.bg_asset = make_test_image("old_bg");

    ActiveICState arriving;
    // background has no asset loaded (empty), so arriving group has no bg layer
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);
    auto* arr_ptr = state.get_mutable_layer_group(1);
    ASSERT_NE(arr_ptr, nullptr);
    auto& arr_group = *arr_ptr;
    // No bg layer because shared background has no asset
    EXPECT_EQ(arr_group.get_layer(0), nullptr);
}

TEST_F(AOSceneCompositorTest, TextboxLayersOnlyInArrivingGroup) {
    textbox.start_message("Phoenix", "Hello", 0, ao_assets.text_colors());

    ActiveICState departing;
    departing.bg_asset = make_test_image("old_bg");
    ActiveICState arriving;

    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);

    // Departing group should have no textbox layers
    auto* dep_ptr = state.get_mutable_layer_group(0);
    ASSERT_NE(dep_ptr, nullptr);
    auto& dep_group = *dep_ptr;
    EXPECT_EQ(dep_group.get_layer(20), nullptr);
    EXPECT_EQ(dep_group.get_layer(21), nullptr);
}

TEST_F(AOSceneCompositorTest, BothGroupsMutableDuringSlide) {
    ActiveICState departing;
    ActiveICState arriving;
    auto state = compositor.compose(background, &arriving, &departing, textbox, 0);
    EXPECT_NE(state.get_mutable_layer_group(0), nullptr);
    EXPECT_NE(state.get_mutable_layer_group(1), nullptr);
}
