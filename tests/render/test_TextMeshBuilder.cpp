#include "render/TextMeshBuilder.h"

#include "asset/MeshAsset.h"
#include "render/GlyphCache.h"
#include "render/TextRenderer.h"

#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Test fixture: loads a known font so GlyphCache can be constructed.
//
// AO_TEST_FONT_PATH is defined via CMake and points to a .ttf bundled
// in the repository (third-party/imgui/misc/fonts/ProggyClean.ttf).
// ---------------------------------------------------------------------------

class TextMeshBuilderTest : public ::testing::Test {
  protected:
    void SetUp() override {
        bool loaded = renderer.load_font(AO_TEST_FONT_PATH, 13);
        if (!loaded) {
            GTEST_SKIP() << "Could not load test font at " << AO_TEST_FONT_PATH;
        }
        cache = std::make_unique<GlyphCache>(renderer, 256);
    }

    TextRenderer renderer;
    std::unique_ptr<GlyphCache> cache;

    // Convenience: call build() with common defaults
    void build(const std::vector<TextRenderer::GlyphLayout>& layout,
               int chars_visible,
               std::vector<MeshVertex>& verts,
               std::vector<uint32_t>& indices,
               int offset_x = 0, int offset_y = 0,
               int scroll_y = 0, int max_height = 0,
               int base_w = 256, int base_h = 192) {
        TextMeshBuilder::build(*cache, layout, chars_visible,
                               offset_x, offset_y, scroll_y, max_height,
                               base_w, base_h, verts, indices);
    }
};

// ===========================================================================
// Empty layout
// ===========================================================================

TEST_F(TextMeshBuilderTest, EmptyLayoutProducesNoVertices) {
    std::vector<TextRenderer::GlyphLayout> layout;
    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 0, verts, indices);

    EXPECT_TRUE(verts.empty());
    EXPECT_TRUE(indices.empty());
}

TEST_F(TextMeshBuilderTest, EmptyLayoutWithHighCharsVisibleStillEmpty) {
    std::vector<TextRenderer::GlyphLayout> layout;
    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 100, verts, indices);

    EXPECT_TRUE(verts.empty());
    EXPECT_TRUE(indices.empty());
}

// ===========================================================================
// Single visible glyph
// ===========================================================================

TEST_F(TextMeshBuilderTest, SingleGlyphProducesFourVerticesSixIndices) {
    // Use 'A' (codepoint 65) -- a glyph that has nonzero width/height
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0}); // codepoint='A', pen_x=0, pen_y=0, char_index=0

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    EXPECT_EQ(verts.size(), 4u);
    EXPECT_EQ(indices.size(), 6u);
}

TEST_F(TextMeshBuilderTest, SingleGlyphIndicesFormTwoTriangles) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    // Expected index pattern: {0,1,3, 1,2,3}
    ASSERT_EQ(indices.size(), 6u);
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_EQ(indices[2], 3u);
    EXPECT_EQ(indices[3], 1u);
    EXPECT_EQ(indices[4], 2u);
    EXPECT_EQ(indices[5], 3u);
}

// ===========================================================================
// chars_visible gating
// ===========================================================================

TEST_F(TextMeshBuilderTest, ZeroCharsVisibleProducesNothing) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 0, verts, indices);

    EXPECT_TRUE(verts.empty());
    EXPECT_TRUE(indices.empty());
}

TEST_F(TextMeshBuilderTest, CharsVisibleLimitsOutput) {
    // Two glyphs, but only the first is visible
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});  // char_index=0, 'A'
    layout.push_back({66, 10, 0, 1}); // char_index=1, 'B'

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    // Only the first glyph should be emitted
    EXPECT_EQ(verts.size(), 4u);
    EXPECT_EQ(indices.size(), 6u);
}

// ===========================================================================
// Multiple visible glyphs
// ===========================================================================

TEST_F(TextMeshBuilderTest, TwoGlyphsProduceEightVerticesTwelveIndices) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});  // 'A', char_index=0
    layout.push_back({66, 10, 0, 1}); // 'B', char_index=1

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 2, verts, indices);

    EXPECT_EQ(verts.size(), 8u);
    EXPECT_EQ(indices.size(), 12u);
}

TEST_F(TextMeshBuilderTest, ThreeGlyphsProduceTwelveVerticesEighteenIndices) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});  // 'A'
    layout.push_back({66, 10, 0, 1}); // 'B'
    layout.push_back({67, 20, 0, 2}); // 'C'

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 3, verts, indices);

    EXPECT_EQ(verts.size(), 12u);
    EXPECT_EQ(indices.size(), 18u);
}

TEST_F(TextMeshBuilderTest, SecondGlyphIndicesStartAtFour) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});
    layout.push_back({66, 10, 0, 1});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 2, verts, indices);

    // Second quad indices should be {4,5,7, 5,6,7}
    ASSERT_EQ(indices.size(), 12u);
    EXPECT_EQ(indices[6], 4u);
    EXPECT_EQ(indices[7], 5u);
    EXPECT_EQ(indices[8], 7u);
    EXPECT_EQ(indices[9], 5u);
    EXPECT_EQ(indices[10], 6u);
    EXPECT_EQ(indices[11], 7u);
}

// ===========================================================================
// Space glyph (zero-size bitmap) is skipped
// ===========================================================================

TEST_F(TextMeshBuilderTest, SpaceGlyphProducesNoVertices) {
    // Space (codepoint 32) has zero width/height in the glyph cache
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({32, 0, 0, 0}); // space

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    EXPECT_TRUE(verts.empty());
    EXPECT_TRUE(indices.empty());
}

// ===========================================================================
// NDC coordinate range
// ===========================================================================

TEST_F(TextMeshBuilderTest, VertexPositionsAreInNdcRange) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices, 0, 0, 0, 0, 256, 192);

    for (const auto& v : verts) {
        EXPECT_GE(v.position[0], -1.0f);
        EXPECT_LE(v.position[0], 1.0f);
        EXPECT_GE(v.position[1], -1.0f);
        EXPECT_LE(v.position[1], 1.0f);
    }
}

TEST_F(TextMeshBuilderTest, TexCoordsAreInZeroOneRange) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    for (const auto& v : verts) {
        EXPECT_GE(v.texcoord[0], 0.0f);
        EXPECT_LE(v.texcoord[0], 1.0f);
        // V coordinates may be inverted (bottom-up atlas), but should be in [0,1]
        EXPECT_GE(v.texcoord[1], 0.0f);
        EXPECT_LE(v.texcoord[1], 1.0f);
    }
}

// ===========================================================================
// Output buffers are cleared on each call
// ===========================================================================

TEST_F(TextMeshBuilderTest, BuildClearsPreviousOutput) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    // First build with a glyph
    build(layout, 1, verts, indices);
    EXPECT_FALSE(verts.empty());
    EXPECT_FALSE(indices.empty());

    // Second build with empty layout -- should clear the output
    std::vector<TextRenderer::GlyphLayout> empty_layout;
    build(empty_layout, 0, verts, indices);
    EXPECT_TRUE(verts.empty());
    EXPECT_TRUE(indices.empty());
}

// ===========================================================================
// Vertex winding order (quad corners)
// ===========================================================================

TEST_F(TextMeshBuilderTest, QuadVerticesFormTopLeftToBottomRightOrder) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    build(layout, 1, verts, indices);

    ASSERT_EQ(verts.size(), 4u);

    // Top-left and top-right share the same Y (position[1])
    EXPECT_FLOAT_EQ(verts[0].position[1], verts[1].position[1]);
    // Bottom-right and bottom-left share the same Y
    EXPECT_FLOAT_EQ(verts[2].position[1], verts[3].position[1]);
    // Top-left and bottom-left share the same X
    EXPECT_FLOAT_EQ(verts[0].position[0], verts[3].position[0]);
    // Top-right and bottom-right share the same X
    EXPECT_FLOAT_EQ(verts[1].position[0], verts[2].position[0]);

    // Top Y > Bottom Y in NDC (GL convention: Y up)
    EXPECT_GT(verts[0].position[1], verts[3].position[1]);
    // Left X < Right X
    EXPECT_LT(verts[0].position[0], verts[1].position[0]);
}

// ===========================================================================
// Offset affects position
// ===========================================================================

TEST_F(TextMeshBuilderTest, OffsetShiftsVertexPositions) {
    std::vector<TextRenderer::GlyphLayout> layout;
    layout.push_back({65, 0, 0, 0});

    std::vector<MeshVertex> verts_no_offset;
    std::vector<uint32_t> idx_no_offset;
    build(layout, 1, verts_no_offset, idx_no_offset, 0, 0);

    std::vector<MeshVertex> verts_with_offset;
    std::vector<uint32_t> idx_with_offset;
    build(layout, 1, verts_with_offset, idx_with_offset, 10, 20);

    ASSERT_EQ(verts_no_offset.size(), verts_with_offset.size());

    // Applying a positive offset_x should shift the vertex X to the right (larger NDC X)
    for (size_t i = 0; i < verts_no_offset.size(); ++i) {
        EXPECT_GT(verts_with_offset[i].position[0], verts_no_offset[i].position[0]);
    }
}

// ===========================================================================
// compute_layout integration: real text produces correct glyph count
// ===========================================================================

TEST_F(TextMeshBuilderTest, ComputeLayoutIntegrationWithBuild) {
    std::string text = "Hi";
    auto layout = renderer.compute_layout(text, 0);

    std::vector<MeshVertex> verts;
    std::vector<uint32_t> indices;

    // Make all characters visible
    build(layout, (int)layout.size(), verts, indices);

    // "Hi" has two renderable characters (neither is a space).
    // Each renderable glyph -> 4 verts, 6 indices.
    // Both 'H' and 'i' should have nonzero bitmaps.
    EXPECT_EQ(verts.size(), 8u);
    EXPECT_EQ(indices.size(), 12u);
}
