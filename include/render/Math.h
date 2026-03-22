#pragma once

#include <cmath>
#include <cstring>

#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#define AO_SIMD_NEON 1
#define AO_SIMD_SSE 0
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
#define AO_SIMD_NEON 0
#define AO_SIMD_SSE 1
#else
#define AO_SIMD_NEON 0
#define AO_SIMD_SSE 0
#endif

struct Vec2 {
    float x = 0, y = 0;
};

struct Vec3 {
    float x = 0, y = 0, z = 0;

    explicit Vec3(float v) : x(v), y(v), z(v) {
    }
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {
    }
    Vec3() = default;
};

/// Column-major 4x4 matrix with SIMD-accelerated operations on arm64 and x86_64.
struct Mat4 {
    float m[16];

    /// Access column as float[4]. Usage: mat[col][row].
    float* operator[](int col) {
        return &m[col * 4];
    }
    const float* operator[](int col) const {
        return &m[col * 4];
    }

    /// Pointer to the underlying float[16] (for GL uniforms, Metal copies, etc.).
    const float* data() const {
        return m;
    }

    static Mat4 identity() {
        Mat4 r;
        std::memset(r.m, 0, sizeof(r.m));
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    Mat4 operator*(const Mat4& rhs) const {
#if AO_SIMD_NEON
        Mat4 r;
        float32x4_t a0 = vld1q_f32(&m[0]);
        float32x4_t a1 = vld1q_f32(&m[4]);
        float32x4_t a2 = vld1q_f32(&m[8]);
        float32x4_t a3 = vld1q_f32(&m[12]);
        for (int i = 0; i < 4; i++) {
            float32x4_t col = vmulq_n_f32(a0, rhs.m[i * 4 + 0]);
            col = vmlaq_n_f32(col, a1, rhs.m[i * 4 + 1]);
            col = vmlaq_n_f32(col, a2, rhs.m[i * 4 + 2]);
            col = vmlaq_n_f32(col, a3, rhs.m[i * 4 + 3]);
            vst1q_f32(&r.m[i * 4], col);
        }
        return r;
#elif AO_SIMD_SSE
        Mat4 r;
        __m128 a0 = _mm_loadu_ps(&m[0]);
        __m128 a1 = _mm_loadu_ps(&m[4]);
        __m128 a2 = _mm_loadu_ps(&m[8]);
        __m128 a3 = _mm_loadu_ps(&m[12]);
        for (int i = 0; i < 4; i++) {
            __m128 col = _mm_mul_ps(a0, _mm_set1_ps(rhs.m[i * 4 + 0]));
            col = _mm_add_ps(col, _mm_mul_ps(a1, _mm_set1_ps(rhs.m[i * 4 + 1])));
            col = _mm_add_ps(col, _mm_mul_ps(a2, _mm_set1_ps(rhs.m[i * 4 + 2])));
            col = _mm_add_ps(col, _mm_mul_ps(a3, _mm_set1_ps(rhs.m[i * 4 + 3])));
            _mm_storeu_ps(&r.m[i * 4], col);
        }
        return r;
#else
        Mat4 r;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float sum = 0;
                for (int k = 0; k < 4; k++)
                    sum += m[k * 4 + j] * rhs.m[i * 4 + k];
                r.m[i * 4 + j] = sum;
            }
        }
        return r;
#endif
    }
};

inline Mat4 translate(const Mat4& basis, const Vec3& v) {
    Mat4 r = basis;
    // result[3] = basis[0]*v.x + basis[1]*v.y + basis[2]*v.z + basis[3]
#if AO_SIMD_NEON
    float32x4_t c3 = vld1q_f32(&basis.m[12]);
    c3 = vmlaq_n_f32(c3, vld1q_f32(&basis.m[0]), v.x);
    c3 = vmlaq_n_f32(c3, vld1q_f32(&basis.m[4]), v.y);
    c3 = vmlaq_n_f32(c3, vld1q_f32(&basis.m[8]), v.z);
    vst1q_f32(&r.m[12], c3);
#elif AO_SIMD_SSE
    __m128 c3 = _mm_loadu_ps(&basis.m[12]);
    c3 = _mm_add_ps(c3, _mm_mul_ps(_mm_loadu_ps(&basis.m[0]), _mm_set1_ps(v.x)));
    c3 = _mm_add_ps(c3, _mm_mul_ps(_mm_loadu_ps(&basis.m[4]), _mm_set1_ps(v.y)));
    c3 = _mm_add_ps(c3, _mm_mul_ps(_mm_loadu_ps(&basis.m[8]), _mm_set1_ps(v.z)));
    _mm_storeu_ps(&r.m[12], c3);
#else
    for (int i = 0; i < 4; i++)
        r.m[12 + i] = basis.m[i] * v.x + basis.m[4 + i] * v.y + basis.m[8 + i] * v.z + basis.m[12 + i];
#endif
    return r;
}

inline Mat4 scale(const Mat4& basis, const Vec3& v) {
    Mat4 r = basis;
#if AO_SIMD_NEON
    vst1q_f32(&r.m[0], vmulq_n_f32(vld1q_f32(&basis.m[0]), v.x));
    vst1q_f32(&r.m[4], vmulq_n_f32(vld1q_f32(&basis.m[4]), v.y));
    vst1q_f32(&r.m[8], vmulq_n_f32(vld1q_f32(&basis.m[8]), v.z));
#elif AO_SIMD_SSE
    _mm_storeu_ps(&r.m[0], _mm_mul_ps(_mm_loadu_ps(&basis.m[0]), _mm_set1_ps(v.x)));
    _mm_storeu_ps(&r.m[4], _mm_mul_ps(_mm_loadu_ps(&basis.m[4]), _mm_set1_ps(v.y)));
    _mm_storeu_ps(&r.m[8], _mm_mul_ps(_mm_loadu_ps(&basis.m[8]), _mm_set1_ps(v.z)));
#else
    for (int i = 0; i < 4; i++)
        r.m[i] = basis.m[i] * v.x;
    for (int i = 0; i < 4; i++)
        r.m[4 + i] = basis.m[4 + i] * v.y;
    for (int i = 0; i < 4; i++)
        r.m[8 + i] = basis.m[8 + i] * v.z;
#endif
    return r;
}

/// Rotate around an arbitrary axis. Only Z-axis rotation is used in this project.
inline Mat4 rotate(const Mat4& basis, float radians, const Vec3& axis) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;

    // Build rotation matrix columns
    Vec3 r0(t * x * x + c, t * x * y + s * z, t * x * z - s * y);
    Vec3 r1(t * x * y - s * z, t * y * y + c, t * y * z + s * x);
    Vec3 r2(t * x * z + s * y, t * y * z - s * x, t * z * z + c);

    Mat4 r;
#if AO_SIMD_NEON
    float32x4_t a0 = vld1q_f32(&basis.m[0]);
    float32x4_t a1 = vld1q_f32(&basis.m[4]);
    float32x4_t a2 = vld1q_f32(&basis.m[8]);

    float32x4_t c0 = vmulq_n_f32(a0, r0.x);
    c0 = vmlaq_n_f32(c0, a1, r0.y);
    c0 = vmlaq_n_f32(c0, a2, r0.z);
    vst1q_f32(&r.m[0], c0);

    float32x4_t c1 = vmulq_n_f32(a0, r1.x);
    c1 = vmlaq_n_f32(c1, a1, r1.y);
    c1 = vmlaq_n_f32(c1, a2, r1.z);
    vst1q_f32(&r.m[4], c1);

    float32x4_t c2 = vmulq_n_f32(a0, r2.x);
    c2 = vmlaq_n_f32(c2, a1, r2.y);
    c2 = vmlaq_n_f32(c2, a2, r2.z);
    vst1q_f32(&r.m[8], c2);
#elif AO_SIMD_SSE
    __m128 a0 = _mm_loadu_ps(&basis.m[0]);
    __m128 a1 = _mm_loadu_ps(&basis.m[4]);
    __m128 a2 = _mm_loadu_ps(&basis.m[8]);

    __m128 c0 = _mm_mul_ps(a0, _mm_set1_ps(r0.x));
    c0 = _mm_add_ps(c0, _mm_mul_ps(a1, _mm_set1_ps(r0.y)));
    c0 = _mm_add_ps(c0, _mm_mul_ps(a2, _mm_set1_ps(r0.z)));
    _mm_storeu_ps(&r.m[0], c0);

    __m128 c1 = _mm_mul_ps(a0, _mm_set1_ps(r1.x));
    c1 = _mm_add_ps(c1, _mm_mul_ps(a1, _mm_set1_ps(r1.y)));
    c1 = _mm_add_ps(c1, _mm_mul_ps(a2, _mm_set1_ps(r1.z)));
    _mm_storeu_ps(&r.m[4], c1);

    __m128 c2 = _mm_mul_ps(a0, _mm_set1_ps(r2.x));
    c2 = _mm_add_ps(c2, _mm_mul_ps(a1, _mm_set1_ps(r2.y)));
    c2 = _mm_add_ps(c2, _mm_mul_ps(a2, _mm_set1_ps(r2.z)));
    _mm_storeu_ps(&r.m[8], c2);
#else
    for (int i = 0; i < 4; i++) {
        r.m[i] = basis.m[i] * r0.x + basis.m[4 + i] * r0.y + basis.m[8 + i] * r0.z;
        r.m[4 + i] = basis.m[i] * r1.x + basis.m[4 + i] * r1.y + basis.m[8 + i] * r1.z;
        r.m[8 + i] = basis.m[i] * r2.x + basis.m[4 + i] * r2.y + basis.m[8 + i] * r2.z;
    }
#endif
    // Column 3 unchanged
    std::memcpy(&r.m[12], &basis.m[12], 4 * sizeof(float));
    return r;
}

inline float radians(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}
