#include "render/Transform.h"

#include <climits>
#include <cmath>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr float kEpsilon = 1e-5f;
static constexpr float kPi = 3.14159265358979323846f;

/// Compare two Mat4 matrices element-wise within epsilon.
static void expect_mat4_near(const Mat4& actual, const Mat4& expected, float eps = kEpsilon) {
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            EXPECT_NEAR(actual[col][row], expected[col][row], eps) << "Mismatch at [" << col << "][" << row << "]";
        }
    }
}

/// Compare a Mat4 against the identity matrix.
static void expect_identity(const Mat4& m) {
    expect_mat4_near(m, Mat4::identity());
}

// ---------------------------------------------------------------------------
// Fixture to save/restore the static aspect_ratio between tests
// ---------------------------------------------------------------------------

class TransformTest : public ::testing::Test {
  protected:
    void SetUp() override {
        saved_aspect_ = Transform::get_aspect_ratio();
        Transform::set_aspect_ratio(1.0f);
    }

    void TearDown() override {
        Transform::set_aspect_ratio(saved_aspect_);
    }

  private:
    float saved_aspect_ = 1.0f;
};

// ---------------------------------------------------------------------------
// 1. Default transform is identity
// ---------------------------------------------------------------------------

TEST_F(TransformTest, DefaultTransformIsIdentity) {
    Transform t;
    expect_identity(t.get_local_transform());
}

// ---------------------------------------------------------------------------
// 2. Translate moves the position (column 3)
// ---------------------------------------------------------------------------

TEST_F(TransformTest, TranslateSetsColumn3) {
    Transform t;
    t.translate({3.0f, -5.0f});

    Mat4 m = t.get_local_transform();

    // Column 3 holds the translation in a column-major matrix.
    EXPECT_NEAR(m[3][0], 3.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], -5.0f, kEpsilon);

    // Rotation/scale part should remain identity.
    EXPECT_NEAR(m[0][0], 1.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 1.0f, kEpsilon);
    EXPECT_NEAR(m[2][2], 1.0f, kEpsilon);
    EXPECT_NEAR(m[3][3], 1.0f, kEpsilon);
}

TEST_F(TransformTest, TranslateNegativeValues) {
    Transform t;
    t.translate({-10.0f, -20.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[3][0], -10.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], -20.0f, kEpsilon);
}

TEST_F(TransformTest, TranslateReplacesNotAccumulates) {
    Transform t;
    t.translate({1.0f, 2.0f});
    t.translate({5.0f, 6.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[3][0], 5.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 6.0f, kEpsilon);
}

// ---------------------------------------------------------------------------
// 3. Scale changes diagonal values
// ---------------------------------------------------------------------------

TEST_F(TransformTest, ScaleChangeDiagonal) {
    Transform t;
    t.scale({2.0f, 3.0f});

    Mat4 m = t.get_local_transform();

    // With aspect_ratio=1, scale goes directly on the diagonal.
    EXPECT_NEAR(m[0][0], 2.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 3.0f, kEpsilon);
    EXPECT_NEAR(m[2][2], 1.0f, kEpsilon);

    // Translation should remain at origin.
    EXPECT_NEAR(m[3][0], 0.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 0.0f, kEpsilon);
}

TEST_F(TransformTest, ScaleReplacesNotAccumulates) {
    Transform t;
    t.scale({2.0f, 3.0f});
    t.scale({4.0f, 5.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[0][0], 4.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 5.0f, kEpsilon);
}

// ---------------------------------------------------------------------------
// 4. Rotate changes rotation components
// ---------------------------------------------------------------------------

TEST_F(TransformTest, Rotate90Degrees) {
    Transform t;
    t.rotate(90.0f);

    Mat4 m = t.get_local_transform();

    // For Z-axis rotation by 90 degrees (aspect=1):
    //   col0 = (cos90, sin90, 0, 0) = (0, 1, 0, 0)
    //   col1 = (-sin90, cos90, 0, 0) = (-1, 0, 0, 0)
    EXPECT_NEAR(m[0][0], std::cos(kPi / 2.0f), kEpsilon);
    EXPECT_NEAR(m[0][1], std::sin(kPi / 2.0f), kEpsilon);
    EXPECT_NEAR(m[1][0], -std::sin(kPi / 2.0f), kEpsilon);
    EXPECT_NEAR(m[1][1], std::cos(kPi / 2.0f), kEpsilon);
}

TEST_F(TransformTest, Rotate180Degrees) {
    Transform t;
    t.rotate(180.0f);

    Mat4 m = t.get_local_transform();

    // cos(180) = -1, sin(180) = 0
    EXPECT_NEAR(m[0][0], -1.0f, kEpsilon);
    EXPECT_NEAR(m[0][1], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][0], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], -1.0f, kEpsilon);
}

TEST_F(TransformTest, Rotate360IsIdentity) {
    Transform t;
    t.rotate(360.0f);

    Mat4 m = t.get_local_transform();

    // Full rotation should be back to identity.
    EXPECT_NEAR(m[0][0], 1.0f, kEpsilon);
    EXPECT_NEAR(m[0][1], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][0], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 1.0f, kEpsilon);
}

TEST_F(TransformTest, RotateReplacesNotAccumulates) {
    Transform t;
    t.rotate(45.0f);
    t.rotate(90.0f);

    Mat4 m = t.get_local_transform();

    // Should be 90 degrees, not 135.
    EXPECT_NEAR(m[0][0], std::cos(kPi / 2.0f), kEpsilon);
    EXPECT_NEAR(m[0][1], std::sin(kPi / 2.0f), kEpsilon);
}

// ---------------------------------------------------------------------------
// 5. Multiple transforms compose correctly
// ---------------------------------------------------------------------------

TEST_F(TransformTest, ScaleThenTranslate) {
    Transform t;
    t.scale({2.0f, 3.0f});
    t.translate({1.0f, 1.0f});

    Mat4 m = t.get_local_transform();

    // recalculate() applies: translate first, then scale.
    // So translation is in pre-scale space and the scale modifies
    // the basis columns but translation goes into column 3 directly.
    // translate(identity, {1,1,0}) => col3 = (1,1,0,1)
    // then scale({2,3,1}) => col0 *= 2, col1 *= 3
    // col3 is NOT scaled (scale only touches cols 0,1,2).
    EXPECT_NEAR(m[3][0], 1.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 1.0f, kEpsilon);
    EXPECT_NEAR(m[0][0], 2.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 3.0f, kEpsilon);
}

TEST_F(TransformTest, TranslateAndRotate) {
    Transform t;
    t.translate({1.0f, 0.0f});
    t.rotate(90.0f);

    Mat4 m = t.get_local_transform();

    // Translation goes to col3, rotation changes cols 0-1.
    EXPECT_NEAR(m[3][0], 1.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 0.0f, kEpsilon);
    EXPECT_NEAR(m[0][0], std::cos(kPi / 2.0f), kEpsilon);
    EXPECT_NEAR(m[0][1], std::sin(kPi / 2.0f), kEpsilon);
}

TEST_F(TransformTest, AllTransformsCombined) {
    Transform t;
    t.translate({2.0f, 3.0f});
    t.scale({0.5f, 0.5f});
    t.rotate(45.0f);

    Mat4 m = t.get_local_transform();

    // Verify it produces a non-trivial matrix.
    // The translation should be in column 3.
    EXPECT_NEAR(m[3][0], 2.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], 3.0f, kEpsilon);

    // Scale * rotation should be reflected in cols 0 and 1.
    float c45 = std::cos(kPi / 4.0f);
    float s45 = std::sin(kPi / 4.0f);

    // recalculate: translate, then rotate(45), then scale(0.5, 0.5, 1)
    // After rotation: col0 = (cos45, sin45, 0, 0), col1 = (-sin45, cos45, 0, 0)
    // After scale(0.5, 0.5): col0 *= 0.5, col1 *= 0.5
    EXPECT_NEAR(m[0][0], c45 * 0.5f, kEpsilon);
    EXPECT_NEAR(m[0][1], s45 * 0.5f, kEpsilon);
    EXPECT_NEAR(m[1][0], -s45 * 0.5f, kEpsilon);
    EXPECT_NEAR(m[1][1], c45 * 0.5f, kEpsilon);
}

// ---------------------------------------------------------------------------
// 6. zindex stores value as Z translation
// ---------------------------------------------------------------------------

TEST_F(TransformTest, ZIndexSetsZTranslation) {
    Transform t;
    t.zindex(0);

    Mat4 m = t.get_local_transform();
    // z=0 => (0/65535)*-2 + (1 - 1/65535) = 1 - 1/65535
    float expected_z = 1.0f - (1.0f / UINT16_MAX);
    EXPECT_NEAR(m[3][2], expected_z, kEpsilon);
}

TEST_F(TransformTest, ZIndexMaxValue) {
    Transform t;
    t.zindex(UINT16_MAX);

    Mat4 m = t.get_local_transform();
    // z=65535 => (65535/65535)*-2 + (1 - 1/65535) = -2 + 1 - 1/65535 = -1 - 1/65535
    float expected_z = ((float)UINT16_MAX / UINT16_MAX) * -2.0f + (1.0f - (1.0f / UINT16_MAX));
    EXPECT_NEAR(m[3][2], expected_z, kEpsilon);
}

TEST_F(TransformTest, ZIndexMidValue) {
    Transform t;
    uint16_t mid = 32768;
    t.zindex(mid);

    Mat4 m = t.get_local_transform();
    float expected_z = ((float)mid / UINT16_MAX) * -2.0f + (1.0f - (1.0f / UINT16_MAX));
    EXPECT_NEAR(m[3][2], expected_z, kEpsilon);
}

TEST_F(TransformTest, ZIndexPreservedAcrossTranslate) {
    Transform t;
    t.zindex(100);
    float z_before = t.get_local_transform()[3][2];

    // translate() should preserve Z set by zindex()
    t.translate({5.0f, 10.0f});
    float z_after = t.get_local_transform()[3][2];

    EXPECT_NEAR(z_before, z_after, kEpsilon);
    EXPECT_NEAR(t.get_local_transform()[3][0], 5.0f, kEpsilon);
    EXPECT_NEAR(t.get_local_transform()[3][1], 10.0f, kEpsilon);
}

// ---------------------------------------------------------------------------
// 7. Aspect ratio static get/set
// ---------------------------------------------------------------------------

TEST_F(TransformTest, AspectRatioDefaultIsOne) {
    // SetUp already resets to 1.0
    EXPECT_NEAR(Transform::get_aspect_ratio(), 1.0f, kEpsilon);
}

TEST_F(TransformTest, AspectRatioSetAndGet) {
    Transform::set_aspect_ratio(16.0f / 9.0f);
    EXPECT_NEAR(Transform::get_aspect_ratio(), 16.0f / 9.0f, kEpsilon);
}

TEST_F(TransformTest, AspectRatioAffectsTransform) {
    // With aspect_ratio=1, a pure scale of {2,2} should give diagonal {2,2,1}.
    Transform t1;
    t1.scale({2.0f, 2.0f});
    Mat4 m1 = t1.get_local_transform();
    EXPECT_NEAR(m1[0][0], 2.0f, kEpsilon);

    // With a different aspect ratio, the matrix should differ.
    Transform::set_aspect_ratio(2.0f);
    Transform t2;
    t2.scale({2.0f, 2.0f});
    Mat4 m2 = t2.get_local_transform();

    // recalculate: scale(1/ar, 1, 1) then rotate then scale(ar, 1, 1) then scale(sx, sy, 1)
    // With no rotation, the aspect compensation cancels out:
    //   (1/2) * 2 * sx = sx, so [0][0] should still be 2.
    EXPECT_NEAR(m2[0][0], 2.0f, kEpsilon);
    EXPECT_NEAR(m2[1][1], 2.0f, kEpsilon);
}

TEST_F(TransformTest, AspectRatioAffectsRotation) {
    // With non-unit aspect ratio, rotation includes aspect compensation.
    Transform::set_aspect_ratio(2.0f);
    Transform t;
    t.rotate(90.0f);

    Mat4 m = t.get_local_transform();

    // recalculate: scale(0.5, 1, 1) * rotate(90) * scale(2, 1, 1)
    // rotate(90) col0 = (0, 1, 0, 0), col1 = (-1, 0, 0, 0)
    // scale(2,1,1): col0 *= 2 => (0, 2, 0, 0), col1 unchanged => (-1, 0, 0, 0)
    // scale(0.5,1,1): col0 row0 *= 0.5 => (0, 2, 0, 0) but row-scale only affects
    //   columns, so scale(0.5,1,1) => col0 *= 0.5 => (0, 1, 0, 0), col1 *= 1 => (-1, 0, 0, 0)
    // Actually, ::scale multiplies entire column vectors:
    //   scale(basis, {0.5, 1, 1}): col0 *= 0.5, col1 *= 1, col2 *= 1
    //
    // Let me trace through recalculate with aspect_ratio=2, rotation=90:
    //   basis = identity
    //   basis = translate(identity, {0,0,0}) = identity
    //   basis = scale(identity, {1/2, 1, 1})
    //     => col0 = (0.5,0,0,0), col1 = (0,1,0,0), col2 = (0,0,1,0), col3 = (0,0,0,1)
    //   basis = rotate(basis, 90deg, Z)
    //     => col0 = basis_col0*cos + basis_col1*sin = (0.5,0,0,0)*0 + (0,1,0,0)*1 = (0,1,0,0)
    //     => col1 = basis_col0*(-sin) + basis_col1*cos = (0.5,0,0,0)*(-1) + (0,1,0,0)*0 = (-0.5,0,0,0)
    //   basis = scale(basis, {2, 1, 1})
    //     => col0 *= 2 = (0,2,0,0), col1 *= 1 = (-0.5,0,0,0)
    //   basis = scale(basis, {1, 1, 1}) [scale was {1,1}]
    //     => no change
    // Final: [0][0]=0, [0][1]=2, [1][0]=-0.5, [1][1]=0

    EXPECT_NEAR(m[0][0], 0.0f, kEpsilon);
    EXPECT_NEAR(m[0][1], 2.0f, kEpsilon);
    EXPECT_NEAR(m[1][0], -0.5f, kEpsilon);
    EXPECT_NEAR(m[1][1], 0.0f, kEpsilon);
}

TEST_F(TransformTest, AspectRatioIsSharedAcrossInstances) {
    Transform::set_aspect_ratio(4.0f / 3.0f);

    Transform t1;
    Transform t2;

    EXPECT_NEAR(Transform::get_aspect_ratio(), 4.0f / 3.0f, kEpsilon);
    // Both should produce the same matrix for the same operations.
    t1.scale({2.0f, 2.0f});
    t2.scale({2.0f, 2.0f});

    Mat4 m1 = t1.get_local_transform();
    Mat4 m2 = t2.get_local_transform();
    expect_mat4_near(m1, m2);
}

// ---------------------------------------------------------------------------
// 8. Identity transform doesn't change coordinates
// ---------------------------------------------------------------------------

TEST_F(TransformTest, IdentityTransformDoesNotChangeCoordinates) {
    Transform t;
    Mat4 m = t.get_local_transform();
    Mat4 id = Mat4::identity();

    // Multiplying identity * a point should return the same point.
    // Verify the matrix IS identity.
    expect_mat4_near(m, id);
}

// ---------------------------------------------------------------------------
// 9. Scale by {1,1} is identity
// ---------------------------------------------------------------------------

TEST_F(TransformTest, ScaleByOneIsIdentity) {
    Transform t;
    t.scale({1.0f, 1.0f});

    expect_identity(t.get_local_transform());
}

// ---------------------------------------------------------------------------
// 10. Rotate by 0 is identity
// ---------------------------------------------------------------------------

TEST_F(TransformTest, RotateByZeroIsIdentity) {
    Transform t;
    t.rotate(0.0f);

    expect_identity(t.get_local_transform());
}

// ---------------------------------------------------------------------------
// 11. Translate by {0,0} is identity
// ---------------------------------------------------------------------------

TEST_F(TransformTest, TranslateByZeroIsIdentity) {
    Transform t;
    t.translate({0.0f, 0.0f});

    expect_identity(t.get_local_transform());
}

// ---------------------------------------------------------------------------
// Additional edge-case and robustness tests
// ---------------------------------------------------------------------------

TEST_F(TransformTest, NegativeScale) {
    Transform t;
    t.scale({-1.0f, -1.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[0][0], -1.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], -1.0f, kEpsilon);
}

TEST_F(TransformTest, ScaleByZero) {
    Transform t;
    t.scale({0.0f, 0.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[0][0], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 0.0f, kEpsilon);
}

TEST_F(TransformTest, LargeTranslation) {
    Transform t;
    t.translate({1000.0f, -2000.0f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[3][0], 1000.0f, kEpsilon);
    EXPECT_NEAR(m[3][1], -2000.0f, kEpsilon);
}

TEST_F(TransformTest, RotateNegativeDegrees) {
    Transform t;
    t.rotate(-90.0f);

    Mat4 m = t.get_local_transform();

    float c = std::cos(-kPi / 2.0f);
    float s = std::sin(-kPi / 2.0f);

    EXPECT_NEAR(m[0][0], c, kEpsilon);
    EXPECT_NEAR(m[0][1], s, kEpsilon);
    EXPECT_NEAR(m[1][0], -s, kEpsilon);
    EXPECT_NEAR(m[1][1], c, kEpsilon);
}

TEST_F(TransformTest, NonUniformScale) {
    Transform t;
    t.scale({3.0f, 0.5f});

    Mat4 m = t.get_local_transform();
    EXPECT_NEAR(m[0][0], 3.0f, kEpsilon);
    EXPECT_NEAR(m[1][1], 0.5f, kEpsilon);
    // Off-diagonal should be zero (no rotation).
    EXPECT_NEAR(m[0][1], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][0], 0.0f, kEpsilon);
}

TEST_F(TransformTest, ZIndexOrderingLowerIsCloser) {
    // Lower zindex values should map to higher Z (closer to camera in NDC).
    Transform t_low, t_high;
    t_low.zindex(10);
    t_high.zindex(1000);

    float z_low = t_low.get_local_transform()[3][2];
    float z_high = t_high.get_local_transform()[3][2];

    // Higher index => more negative Z => farther from camera.
    EXPECT_GT(z_low, z_high);
}

TEST_F(TransformTest, Rotate45Degrees) {
    Transform t;
    t.rotate(45.0f);

    Mat4 m = t.get_local_transform();

    float c = std::cos(kPi / 4.0f);
    float s = std::sin(kPi / 4.0f);

    EXPECT_NEAR(m[0][0], c, kEpsilon);
    EXPECT_NEAR(m[0][1], s, kEpsilon);
    EXPECT_NEAR(m[1][0], -s, kEpsilon);
    EXPECT_NEAR(m[1][1], c, kEpsilon);

    // Z column and row 2 should be unaffected.
    EXPECT_NEAR(m[2][2], 1.0f, kEpsilon);
    EXPECT_NEAR(m[0][2], 0.0f, kEpsilon);
    EXPECT_NEAR(m[1][2], 0.0f, kEpsilon);
}

TEST_F(TransformTest, GetLocalTransformReturnsConsistentCopy) {
    Transform t;
    t.translate({1.0f, 2.0f});
    t.scale({3.0f, 4.0f});
    t.rotate(30.0f);

    Mat4 m1 = t.get_local_transform();
    Mat4 m2 = t.get_local_transform();

    // Two calls with no mutation in between should give the same result.
    expect_mat4_near(m1, m2);
}
