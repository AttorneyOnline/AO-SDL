#include "asset/ImageAsset.h"
#include "render/Layer.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static auto make_asset(int frames, int duration_ms = 100) {
    std::vector<DecodedFrame> f;
    for (int i = 0; i < frames; i++) {
        f.push_back(DecodedFrame{std::vector<uint8_t>(4, (uint8_t)i), 1, 1, duration_ms});
    }
    return std::make_shared<ImageAsset>("test", "png", std::move(f));
}

// ---------------------------------------------------------------------------
// Layer
// ---------------------------------------------------------------------------

TEST(Layer, StoresAssetFrameIndexAndZIndex) {
    auto asset = make_asset(3);
    Layer layer(asset, 2, 10);

    EXPECT_EQ(layer.get_asset(), asset);
    EXPECT_EQ(layer.get_frame_index(), 2);
    EXPECT_EQ(layer.get_z_index(), 10);
}

TEST(Layer, NullAssetIsAllowed) {
    Layer layer(nullptr, 0, 0);
    EXPECT_EQ(layer.get_asset(), nullptr);
    EXPECT_EQ(layer.get_frame_index(), 0);
    EXPECT_EQ(layer.get_z_index(), 0);
}

// ---------------------------------------------------------------------------
// LayerGroup
// ---------------------------------------------------------------------------

TEST(LayerGroup, DefaultIsEmpty) {
    LayerGroup group;
    EXPECT_TRUE(group.get_layers().empty());
}

TEST(LayerGroup, AddLayerAndRetrieve) {
    LayerGroup group;
    auto asset = make_asset(1);
    group.add_layer(0, Layer(asset, 0, 5));

    const auto& layers = group.get_layers();
    ASSERT_EQ(layers.size(), 1u);
    EXPECT_NE(layers.find(0), layers.end());
    EXPECT_EQ(layers.at(0).get_asset(), asset);
    EXPECT_EQ(layers.at(0).get_frame_index(), 0);
    EXPECT_EQ(layers.at(0).get_z_index(), 5);
}

TEST(LayerGroup, MultipleLayers) {
    LayerGroup group;
    auto asset_a = make_asset(1);
    auto asset_b = make_asset(2);

    group.add_layer(1, Layer(asset_a, 0, 10));
    group.add_layer(2, Layer(asset_b, 1, 20));

    const auto& layers = group.get_layers();
    ASSERT_EQ(layers.size(), 2u);

    EXPECT_EQ(layers.at(1).get_asset(), asset_a);
    EXPECT_EQ(layers.at(1).get_z_index(), 10);

    EXPECT_EQ(layers.at(2).get_asset(), asset_b);
    EXPECT_EQ(layers.at(2).get_frame_index(), 1);
    EXPECT_EQ(layers.at(2).get_z_index(), 20);
}

TEST(LayerGroup, AddLayerReplacesExistingId) {
    LayerGroup group;
    auto asset_a = make_asset(1);
    auto asset_b = make_asset(1);

    group.add_layer(0, Layer(asset_a, 0, 5));
    group.add_layer(0, Layer(asset_b, 0, 10));

    const auto& layers = group.get_layers();
    ASSERT_EQ(layers.size(), 1u);
    EXPECT_EQ(layers.at(0).get_asset(), asset_b);
    EXPECT_EQ(layers.at(0).get_z_index(), 10);
}
