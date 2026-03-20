#include "utils/BlendOps.h"

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// blend_over
// ---------------------------------------------------------------------------

TEST(BlendOver, FullyTransparentSrcLeavesDstUnchanged) {
    uint8_t dst[4] = {100, 150, 200, 255};
    uint8_t src[4] = {0, 0, 0, 0};
    BlendOps::blend_over(dst, src);
    EXPECT_EQ(dst[0], 100);
    EXPECT_EQ(dst[1], 150);
    EXPECT_EQ(dst[2], 200);
    EXPECT_EQ(dst[3], 255);
}

TEST(BlendOver, FullyOpaqueSrcOverwritesDst) {
    uint8_t dst[4] = {100, 150, 200, 255};
    uint8_t src[4] = {10, 20, 30, 255};
    BlendOps::blend_over(dst, src);
    EXPECT_EQ(dst[0], 10);
    EXPECT_EQ(dst[1], 20);
    EXPECT_EQ(dst[2], 30);
    EXPECT_EQ(dst[3], 255);
}

TEST(BlendOver, SemiTransparentSrcBlends) {
    // src = (255, 0, 0, 128) over dst = (0, 0, 255, 255)
    // sa = 128, a ~= 0.502, inv ~= 0.498, da = 1.0
    // oa = 0.502 + 1.0 * 0.498 = 1.0
    // R = (255*0.502 + 0*1.0*0.498) / 1.0 ~= 128
    // B = (0*0.502 + 255*1.0*0.498) / 1.0 ~= 127
    uint8_t dst[4] = {0, 0, 255, 255};
    uint8_t src[4] = {255, 0, 0, 128};
    BlendOps::blend_over(dst, src);
    EXPECT_NEAR(dst[0], 128, 2);
    EXPECT_EQ(dst[1], 0);
    EXPECT_NEAR(dst[2], 127, 2);
    EXPECT_EQ(dst[3], 255);
}

TEST(BlendOver, BlendOntoTransparentDst) {
    // dst is fully transparent; result should be src color with src alpha
    uint8_t dst[4] = {0, 0, 0, 0};
    uint8_t src[4] = {200, 100, 50, 128};
    BlendOps::blend_over(dst, src);
    // oa = 128/255 ~= 0.502, so dst alpha ~= 128
    EXPECT_NEAR(dst[3], 128, 2);
    // Since da=0, color numerator = src*a, denominator = oa = a, so color = src
    EXPECT_EQ(dst[0], 200);
    EXPECT_EQ(dst[1], 100);
    EXPECT_EQ(dst[2], 50);
}

// ---------------------------------------------------------------------------
// blend_color
// ---------------------------------------------------------------------------

TEST(BlendColor, AlphaZeroLeavesDstUnchanged) {
    uint8_t dst[4] = {100, 150, 200, 255};
    BlendOps::blend_color(dst, 10, 20, 30, 0);
    EXPECT_EQ(dst[0], 100);
    EXPECT_EQ(dst[1], 150);
    EXPECT_EQ(dst[2], 200);
    EXPECT_EQ(dst[3], 255);
}

TEST(BlendColor, Alpha255WritesColorDirectly) {
    uint8_t dst[4] = {100, 150, 200, 128};
    BlendOps::blend_color(dst, 10, 20, 30, 255);
    EXPECT_EQ(dst[0], 10);
    EXPECT_EQ(dst[1], 20);
    EXPECT_EQ(dst[2], 30);
    EXPECT_EQ(dst[3], 255);
}

TEST(BlendColor, SemiTransparentBlends) {
    // Same logic as blend_over semi-transparent test
    uint8_t dst[4] = {0, 0, 255, 255};
    BlendOps::blend_color(dst, 255, 0, 0, 128);
    EXPECT_NEAR(dst[0], 128, 2);
    EXPECT_EQ(dst[1], 0);
    EXPECT_NEAR(dst[2], 127, 2);
    EXPECT_EQ(dst[3], 255);
}
