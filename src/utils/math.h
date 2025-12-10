/**
 * Slam Engine - Math Utilities
 *
 * Custom math types: vec2, vec3, vec4, mat4, quat
 * Header-only implementation for simplicity
 */

#pragma once

#include <cmath>
#include <algorithm>

namespace slam {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float HALF_PI = 1.57079632679489661923f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float EPSILON = 1e-6f;

// Forward declarations
struct vec2;
struct vec3;
struct vec4;
struct mat4;
struct quat;

// ============================================================================
// vec2
// ============================================================================
struct vec2 {
    float x, y;

    vec2() : x(0), y(0) {}
    vec2(float s) : x(s), y(s) {}
    vec2(float x, float y) : x(x), y(y) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
    vec2 operator*(float s) const { return vec2(x * s, y * s); }
    vec2 operator/(float s) const { return vec2(x / s, y / s); }
    vec2 operator*(const vec2& v) const { return vec2(x * v.x, y * v.y); }

    vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    vec2 operator-() const { return vec2(-x, -y); }

    float length() const { return std::sqrt(x * x + y * y); }
    float length_squared() const { return x * x + y * y; }
};

inline vec2 operator*(float s, const vec2& v) { return v * s; }
inline float dot(const vec2& a, const vec2& b) { return a.x * b.x + a.y * b.y; }
inline vec2 normalize(const vec2& v) { float len = v.length(); return len > EPSILON ? v / len : vec2(0); }

// ============================================================================
// vec3
// ============================================================================
struct vec3 {
    float x, y, z;

    vec3() : x(0), y(0), z(0) {}
    vec3(float s) : x(s), y(s), z(s) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3(const vec2& v, float z) : x(v.x), y(v.y), z(z) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }
    vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }

    vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

    vec3 operator-() const { return vec3(-x, -y, -z); }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float length_squared() const { return x * x + y * y + z * z; }

    vec2 xy() const { return vec2(x, y); }
    vec2 xz() const { return vec2(x, z); }
};

inline vec3 operator*(float s, const vec3& v) { return v * s; }
inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}
inline vec3 normalize(const vec3& v) { float len = v.length(); return len > EPSILON ? v / len : vec3(0); }
inline vec3 lerp(const vec3& a, const vec3& b, float t) { return a + (b - a) * t; }
inline vec3 reflect(const vec3& v, const vec3& n) { return v - 2.0f * dot(v, n) * n; }

// ============================================================================
// vec4
// ============================================================================
struct vec4 {
    float x, y, z, w;

    vec4() : x(0), y(0), z(0), w(0) {}
    vec4(float s) : x(s), y(s), z(s), w(s) {}
    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    vec4(const vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}
    vec4(const vec2& v, float z, float w) : x(v.x), y(v.y), z(z), w(w) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    vec4 operator-(const vec4& v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    vec4 operator*(const vec4& v) const { return vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
    vec4 operator*(float s) const { return vec4(x * s, y * s, z * s, w * s); }
    vec4 operator/(float s) const { return vec4(x / s, y / s, z / s, w / s); }

    vec4& operator+=(const vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    vec4& operator-=(const vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }

    vec4 operator-() const { return vec4(-x, -y, -z, -w); }

    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    float length_squared() const { return x * x + y * y + z * z + w * w; }

    vec3 xyz() const { return vec3(x, y, z); }
    vec2 xy() const { return vec2(x, y); }
};

inline vec4 operator*(float s, const vec4& v) { return v * s; }
inline float dot(const vec4& a, const vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline vec4 normalize(const vec4& v) { float len = v.length(); return len > EPSILON ? v / len : vec4(0); }

// ============================================================================
// mat4 - Column-major 4x4 matrix (Vulkan/OpenGL compatible)
// ============================================================================
struct mat4 {
    vec4 cols[4]; // Column vectors

    mat4() {
        cols[0] = vec4(1, 0, 0, 0);
        cols[1] = vec4(0, 1, 0, 0);
        cols[2] = vec4(0, 0, 1, 0);
        cols[3] = vec4(0, 0, 0, 1);
    }

    mat4(float diagonal) {
        cols[0] = vec4(diagonal, 0, 0, 0);
        cols[1] = vec4(0, diagonal, 0, 0);
        cols[2] = vec4(0, 0, diagonal, 0);
        cols[3] = vec4(0, 0, 0, diagonal);
    }

    mat4(const vec4& c0, const vec4& c1, const vec4& c2, const vec4& c3) {
        cols[0] = c0;
        cols[1] = c1;
        cols[2] = c2;
        cols[3] = c3;
    }

    vec4& operator[](int i) { return cols[i]; }
    const vec4& operator[](int i) const { return cols[i]; }

    // Access element at (row, col)
    float& at(int row, int col) { return cols[col][row]; }
    const float& at(int row, int col) const { return cols[col][row]; }

    // Get pointer to data (for Vulkan uploads)
    const float* data() const { return &cols[0].x; }
    float* data() { return &cols[0].x; }

    mat4 operator*(const mat4& m) const {
        mat4 result;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result.at(row, col) =
                    at(row, 0) * m.at(0, col) +
                    at(row, 1) * m.at(1, col) +
                    at(row, 2) * m.at(2, col) +
                    at(row, 3) * m.at(3, col);
            }
        }
        return result;
    }

    vec4 operator*(const vec4& v) const {
        return vec4(
            cols[0].x * v.x + cols[1].x * v.y + cols[2].x * v.z + cols[3].x * v.w,
            cols[0].y * v.x + cols[1].y * v.y + cols[2].y * v.z + cols[3].y * v.w,
            cols[0].z * v.x + cols[1].z * v.y + cols[2].z * v.z + cols[3].z * v.w,
            cols[0].w * v.x + cols[1].w * v.y + cols[2].w * v.z + cols[3].w * v.w
        );
    }

    static mat4 identity() { return mat4(1.0f); }

    mat4 inverse() const {
        // Compute matrix inverse using cofactors
        const float* m = data();
        float inv[16];

        inv[0] = m[5]  * m[10] * m[15] - m[5]  * m[11] * m[14] - m[9]  * m[6]  * m[15] +
                 m[9]  * m[7]  * m[14] + m[13] * m[6]  * m[11] - m[13] * m[7]  * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4]  * m[11] * m[14] + m[8]  * m[6]  * m[15] -
                 m[8]  * m[7]  * m[14] - m[12] * m[6]  * m[11] + m[12] * m[7]  * m[10];
        inv[8] = m[4]  * m[9]  * m[15] - m[4]  * m[11] * m[13] - m[8]  * m[5]  * m[15] +
                 m[8]  * m[7]  * m[13] + m[12] * m[5]  * m[11] - m[12] * m[7]  * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4]  * m[10] * m[13] + m[8]  * m[5]  * m[14] -
                  m[8]  * m[6] * m[13] - m[12] * m[5]  * m[10] + m[12] * m[6]  * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1]  * m[11] * m[14] + m[9]  * m[2]  * m[15] -
                 m[9]  * m[3]  * m[14] - m[13] * m[2]  * m[11] + m[13] * m[3]  * m[10];
        inv[5] = m[0]  * m[10] * m[15] - m[0]  * m[11] * m[14] - m[8]  * m[2]  * m[15] +
                 m[8]  * m[3]  * m[14] + m[12] * m[2]  * m[11] - m[12] * m[3]  * m[10];
        inv[9] = -m[0] * m[9]  * m[15] + m[0]  * m[11] * m[13] + m[8]  * m[1]  * m[15] -
                 m[8]  * m[3]  * m[13] - m[12] * m[1]  * m[11] + m[12] * m[3]  * m[9];
        inv[13] = m[0] * m[9]  * m[14] - m[0]  * m[10] * m[13] - m[8]  * m[1]  * m[14] +
                  m[8] * m[2]  * m[13] + m[12] * m[1]  * m[10] - m[12] * m[2]  * m[9];
        inv[2] = m[1]  * m[6]  * m[15] - m[1]  * m[7]  * m[14] - m[5]  * m[2]  * m[15] +
                 m[5]  * m[3]  * m[14] + m[13] * m[2]  * m[7]  - m[13] * m[3]  * m[6];
        inv[6] = -m[0] * m[6]  * m[15] + m[0]  * m[7]  * m[14] + m[4]  * m[2]  * m[15] -
                 m[4]  * m[3]  * m[14] - m[12] * m[2]  * m[7]  + m[12] * m[3]  * m[6];
        inv[10] = m[0] * m[5]  * m[15] - m[0]  * m[7]  * m[13] - m[4]  * m[1]  * m[15] +
                  m[4] * m[3]  * m[13] + m[12] * m[1]  * m[7]  - m[12] * m[3]  * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0]  * m[6]  * m[13] + m[4]  * m[1]  * m[14] -
                  m[4]  * m[2] * m[13] - m[12] * m[1]  * m[6]  + m[12] * m[2]  * m[5];
        inv[3] = -m[1] * m[6]  * m[11] + m[1]  * m[7]  * m[10] + m[5]  * m[2]  * m[11] -
                 m[5]  * m[3]  * m[10] - m[9]  * m[2]  * m[7]  + m[9]  * m[3]  * m[6];
        inv[7] = m[0]  * m[6]  * m[11] - m[0]  * m[7]  * m[10] - m[4]  * m[2]  * m[11] +
                 m[4]  * m[3]  * m[10] + m[8]  * m[2]  * m[7]  - m[8]  * m[3]  * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0]  * m[7]  * m[9]  + m[4]  * m[1]  * m[11] -
                  m[4]  * m[3] * m[9]  - m[8]  * m[1]  * m[7]  + m[8]  * m[3]  * m[5];
        inv[15] = m[0] * m[5]  * m[10] - m[0]  * m[6]  * m[9]  - m[4]  * m[1]  * m[10] +
                  m[4] * m[2]  * m[9]  + m[8]  * m[1]  * m[6]  - m[8]  * m[2]  * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (det == 0) return mat4(1.0f); // Return identity if singular

        float inv_det = 1.0f / det;
        mat4 result;
        float* r = result.data();
        for (int i = 0; i < 16; i++) {
            r[i] = inv[i] * inv_det;
        }
        return result;
    }
};

// Matrix operations
inline mat4 transpose(const mat4& m) {
    mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.at(i, j) = m.at(j, i);
        }
    }
    return result;
}

inline mat4 translate(const mat4& m, const vec3& v) {
    mat4 result = m;
    result[3] = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
    return result;
}

inline mat4 translate(const vec3& v) {
    mat4 result;
    result[3] = vec4(v, 1.0f);
    return result;
}

inline mat4 scale(const mat4& m, const vec3& v) {
    mat4 result;
    result[0] = m[0] * v.x;
    result[1] = m[1] * v.y;
    result[2] = m[2] * v.z;
    result[3] = m[3];
    return result;
}

inline mat4 scale(const vec3& v) {
    mat4 result;
    result[0][0] = v.x;
    result[1][1] = v.y;
    result[2][2] = v.z;
    return result;
}

inline mat4 rotate(const mat4& m, float angle, const vec3& axis) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    vec3 a = normalize(axis);
    vec3 t = a * (1.0f - c);

    mat4 rot;
    rot[0][0] = c + t.x * a.x;
    rot[0][1] = t.x * a.y + s * a.z;
    rot[0][2] = t.x * a.z - s * a.y;

    rot[1][0] = t.y * a.x - s * a.z;
    rot[1][1] = c + t.y * a.y;
    rot[1][2] = t.y * a.z + s * a.x;

    rot[2][0] = t.z * a.x + s * a.y;
    rot[2][1] = t.z * a.y - s * a.x;
    rot[2][2] = c + t.z * a.z;

    mat4 result;
    result[0] = m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2];
    result[1] = m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2];
    result[2] = m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2];
    result[3] = m[3];
    return result;
}

// Perspective projection matrix (Vulkan clip space: Y down, Z [0,1])
inline mat4 perspective(float fov_y, float aspect, float near_plane, float far_plane) {
    float tan_half_fov = std::tan(fov_y * 0.5f);

    mat4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tan_half_fov);
    result[1][1] = -1.0f / tan_half_fov; // Vulkan Y is flipped
    result[2][2] = far_plane / (near_plane - far_plane);
    result[2][3] = -1.0f;
    result[3][2] = (far_plane * near_plane) / (near_plane - far_plane);
    return result;
}

// Orthographic projection matrix
inline mat4 ortho(float left, float right, float bottom, float top, float near_plane, float far_plane) {
    mat4 result;
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 1.0f / (near_plane - far_plane);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = near_plane / (near_plane - far_plane);
    return result;
}

// Look-at view matrix
inline mat4 look_at(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f = normalize(center - eye);
    vec3 r = normalize(cross(f, up));
    vec3 u = cross(r, f);

    mat4 result;
    result[0][0] = r.x;
    result[1][0] = r.y;
    result[2][0] = r.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -dot(r, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] = dot(f, eye);
    return result;
}

// Inverse of a 4x4 matrix (using cofactors)
inline mat4 inverse(const mat4& m) {
    float coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
    float coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
    float coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
    float coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
    float coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
    float coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];
    float coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
    float coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
    float coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];
    float coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
    float coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
    float coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];
    float coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
    float coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
    float coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];
    float coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
    float coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
    float coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

    vec4 fac0(coef00, coef00, coef02, coef03);
    vec4 fac1(coef04, coef04, coef06, coef07);
    vec4 fac2(coef08, coef08, coef10, coef11);
    vec4 fac3(coef12, coef12, coef14, coef15);
    vec4 fac4(coef16, coef16, coef18, coef19);
    vec4 fac5(coef20, coef20, coef22, coef23);

    vec4 v0(m[1][0], m[0][0], m[0][0], m[0][0]);
    vec4 v1(m[1][1], m[0][1], m[0][1], m[0][1]);
    vec4 v2(m[1][2], m[0][2], m[0][2], m[0][2]);
    vec4 v3(m[1][3], m[0][3], m[0][3], m[0][3]);

    vec4 inv0 = v1 * fac0 - v2 * fac1 + v3 * fac2;
    vec4 inv1 = v0 * fac0 - v2 * fac3 + v3 * fac4;
    vec4 inv2 = v0 * fac1 - v1 * fac3 + v3 * fac5;
    vec4 inv3 = v0 * fac2 - v1 * fac4 + v2 * fac5;

    vec4 sign_a(+1, -1, +1, -1);
    vec4 sign_b(-1, +1, -1, +1);

    mat4 inv(inv0 * sign_a, inv1 * sign_b, inv2 * sign_a, inv3 * sign_b);

    vec4 row0(inv[0][0], inv[1][0], inv[2][0], inv[3][0]);
    float det = dot(m[0], row0);

    if (std::abs(det) < EPSILON) {
        return mat4(); // Return identity if singular
    }

    return mat4(
        inv[0] / det,
        inv[1] / det,
        inv[2] / det,
        inv[3] / det
    );
}

// ============================================================================
// quat - Quaternion for rotations
// ============================================================================
struct quat {
    float x, y, z, w;

    quat() : x(0), y(0), z(0), w(1) {} // Identity quaternion
    quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    // Construct from axis-angle
    static quat from_axis_angle(const vec3& axis, float angle) {
        float half_angle = angle * 0.5f;
        float s = std::sin(half_angle);
        vec3 a = normalize(axis);
        return quat(a.x * s, a.y * s, a.z * s, std::cos(half_angle));
    }

    // Construct from Euler angles (pitch, yaw, roll in radians)
    static quat from_euler(float pitch, float yaw, float roll) {
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);

        return quat(
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        );
    }

    quat operator*(const quat& q) const {
        return quat(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    // Rotate a vector
    vec3 operator*(const vec3& v) const {
        vec3 qv(x, y, z);
        vec3 uv = cross(qv, v);
        vec3 uuv = cross(qv, uv);
        return v + ((uv * w) + uuv) * 2.0f;
    }

    quat conjugate() const { return quat(-x, -y, -z, w); }

    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    float length_squared() const { return x * x + y * y + z * z + w * w; }

    // Convert to rotation matrix
    mat4 to_mat4() const {
        float xx = x * x, xy = x * y, xz = x * z, xw = x * w;
        float yy = y * y, yz = y * z, yw = y * w;
        float zz = z * z, zw = z * w;

        mat4 result;
        result[0][0] = 1.0f - 2.0f * (yy + zz);
        result[0][1] = 2.0f * (xy + zw);
        result[0][2] = 2.0f * (xz - yw);

        result[1][0] = 2.0f * (xy - zw);
        result[1][1] = 1.0f - 2.0f * (xx + zz);
        result[1][2] = 2.0f * (yz + xw);

        result[2][0] = 2.0f * (xz + yw);
        result[2][1] = 2.0f * (yz - xw);
        result[2][2] = 1.0f - 2.0f * (xx + yy);

        return result;
    }

    // Get forward direction (-Z axis)
    vec3 forward() const { return *this * vec3(0, 0, -1); }
    // Get right direction (+X axis)
    vec3 right() const { return *this * vec3(1, 0, 0); }
    // Get up direction (+Y axis)
    vec3 up() const { return *this * vec3(0, 1, 0); }
};

inline quat normalize(const quat& q) {
    float len = q.length();
    return len > EPSILON ? quat(q.x / len, q.y / len, q.z / len, q.w / len) : quat();
}

inline float dot(const quat& a, const quat& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Spherical linear interpolation
inline quat slerp(const quat& a, const quat& b, float t) {
    quat q = b;
    float cos_theta = dot(a, b);

    // If cos_theta < 0, negate one quaternion to take shorter path
    if (cos_theta < 0.0f) {
        q = quat(-q.x, -q.y, -q.z, -q.w);
        cos_theta = -cos_theta;
    }

    // If very close, use linear interpolation
    if (cos_theta > 0.9995f) {
        return normalize(quat(
            a.x + t * (q.x - a.x),
            a.y + t * (q.y - a.y),
            a.z + t * (q.z - a.z),
            a.w + t * (q.w - a.w)
        ));
    }

    float theta = std::acos(cos_theta);
    float sin_theta = std::sin(theta);
    float wa = std::sin((1.0f - t) * theta) / sin_theta;
    float wb = std::sin(t * theta) / sin_theta;

    return quat(
        wa * a.x + wb * q.x,
        wa * a.y + wb * q.y,
        wa * a.z + wb * q.z,
        wa * a.w + wb * q.w
    );
}

// ============================================================================
// Utility functions
// ============================================================================

inline float clamp(float x, float min_val, float max_val) {
    return std::max(min_val, std::min(max_val, x));
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float radians(float degrees) {
    return degrees * DEG_TO_RAD;
}

inline float degrees(float radians) {
    return radians * RAD_TO_DEG;
}

} // namespace slam
