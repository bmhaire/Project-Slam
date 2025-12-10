/**
 * Slam Engine - Noise Functions
 *
 * Procedural noise for texture generation: Perlin, Simplex, Worley
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>

namespace slam {
namespace noise {

// Constants
constexpr float PI = 3.14159265358979323846f;

// Simple vec2/vec3 for noise module (avoid dependency on utils/math.h)
struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };

inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline float length(Vec3 v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

// Utility functions
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }
inline float quintic(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
inline float floor_f(float x) { return std::floor(x); }
inline float fract(float x) { return x - floor_f(x); }

// ============================================================================
// Permutation table (shared by Perlin and Simplex)
// ============================================================================
class PermutationTable {
public:
    PermutationTable(uint32_t seed = 0) { reseed(seed); }

    void reseed(uint32_t seed) {
        // Initialize with identity
        for (int i = 0; i < 256; i++) {
            p_[i] = i;
        }

        // Fisher-Yates shuffle
        uint32_t state = seed;
        for (int i = 255; i > 0; i--) {
            state = xorshift32(state);
            int j = state % (i + 1);
            std::swap(p_[i], p_[j]);
        }

        // Duplicate for wrapping
        for (int i = 0; i < 256; i++) {
            p_[256 + i] = p_[i];
        }
    }

    int operator[](int i) const { return p_[i & 511]; }

private:
    static uint32_t xorshift32(uint32_t x) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    }

    std::array<int, 512> p_;
};

// ============================================================================
// Perlin Noise
// ============================================================================
class PerlinNoise {
public:
    PerlinNoise(uint32_t seed = 0) : perm_(seed) {}

    void reseed(uint32_t seed) { perm_.reseed(seed); }

    // 2D Perlin noise
    float noise(float x, float y) const {
        int xi = static_cast<int>(floor_f(x)) & 255;
        int yi = static_cast<int>(floor_f(y)) & 255;

        float xf = fract(x);
        float yf = fract(y);

        float u = quintic(xf);
        float v = quintic(yf);

        int aa = perm_[perm_[xi] + yi];
        int ab = perm_[perm_[xi] + yi + 1];
        int ba = perm_[perm_[xi + 1] + yi];
        int bb = perm_[perm_[xi + 1] + yi + 1];

        float x1 = lerp(grad2d(aa, xf, yf), grad2d(ba, xf - 1, yf), u);
        float x2 = lerp(grad2d(ab, xf, yf - 1), grad2d(bb, xf - 1, yf - 1), u);

        return lerp(x1, x2, v);
    }

    // 3D Perlin noise
    float noise(float x, float y, float z) const {
        int xi = static_cast<int>(floor_f(x)) & 255;
        int yi = static_cast<int>(floor_f(y)) & 255;
        int zi = static_cast<int>(floor_f(z)) & 255;

        float xf = fract(x);
        float yf = fract(y);
        float zf = fract(z);

        float u = quintic(xf);
        float v = quintic(yf);
        float w = quintic(zf);

        int aaa = perm_[perm_[perm_[xi] + yi] + zi];
        int aba = perm_[perm_[perm_[xi] + yi + 1] + zi];
        int aab = perm_[perm_[perm_[xi] + yi] + zi + 1];
        int abb = perm_[perm_[perm_[xi] + yi + 1] + zi + 1];
        int baa = perm_[perm_[perm_[xi + 1] + yi] + zi];
        int bba = perm_[perm_[perm_[xi + 1] + yi + 1] + zi];
        int bab = perm_[perm_[perm_[xi + 1] + yi] + zi + 1];
        int bbb = perm_[perm_[perm_[xi + 1] + yi + 1] + zi + 1];

        float x1, x2, y1, y2;
        x1 = lerp(grad3d(aaa, xf, yf, zf), grad3d(baa, xf - 1, yf, zf), u);
        x2 = lerp(grad3d(aba, xf, yf - 1, zf), grad3d(bba, xf - 1, yf - 1, zf), u);
        y1 = lerp(x1, x2, v);

        x1 = lerp(grad3d(aab, xf, yf, zf - 1), grad3d(bab, xf - 1, yf, zf - 1), u);
        x2 = lerp(grad3d(abb, xf, yf - 1, zf - 1), grad3d(bbb, xf - 1, yf - 1, zf - 1), u);
        y2 = lerp(x1, x2, v);

        return lerp(y1, y2, w);
    }

    // Fractal Brownian Motion (multi-octave noise)
    float fbm(float x, float y, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float max_val = 0.0f;

        for (int i = 0; i < octaves; i++) {
            sum += amp * noise(x * freq, y * freq);
            max_val += amp;
            amp *= gain;
            freq *= lacunarity;
        }

        return sum / max_val;
    }

    float fbm(float x, float y, float z, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float max_val = 0.0f;

        for (int i = 0; i < octaves; i++) {
            sum += amp * noise(x * freq, y * freq, z * freq);
            max_val += amp;
            amp *= gain;
            freq *= lacunarity;
        }

        return sum / max_val;
    }

    // Ridge noise (inverted absolute value - creates sharp ridges)
    float ridge(float x, float y, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float max_val = 0.0f;

        for (int i = 0; i < octaves; i++) {
            float n = noise(x * freq, y * freq);
            n = 1.0f - std::abs(n);
            n = n * n;
            sum += amp * n;
            max_val += amp;
            amp *= gain;
            freq *= lacunarity;
        }

        return sum / max_val;
    }

private:
    float grad2d(int hash, float x, float y) const {
        switch (hash & 3) {
            case 0: return x + y;
            case 1: return -x + y;
            case 2: return x - y;
            case 3: return -x - y;
            default: return 0;
        }
    }

    float grad3d(int hash, float x, float y, float z) const {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    PermutationTable perm_;
};

// ============================================================================
// Simplex Noise (faster, less artifacts than Perlin)
// ============================================================================
class SimplexNoise {
public:
    SimplexNoise(uint32_t seed = 0) : perm_(seed) {}

    void reseed(uint32_t seed) { perm_.reseed(seed); }

    // 2D Simplex noise
    float noise(float x, float y) const {
        constexpr float F2 = 0.366025403784f; // (sqrt(3) - 1) / 2
        constexpr float G2 = 0.211324865405f; // (3 - sqrt(3)) / 6

        float s = (x + y) * F2;
        int i = static_cast<int>(floor_f(x + s));
        int j = static_cast<int>(floor_f(y + s));

        float t = (i + j) * G2;
        float X0 = i - t;
        float Y0 = j - t;
        float x0 = x - X0;
        float y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }

        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;

        int ii = i & 255;
        int jj = j & 255;
        int gi0 = perm_[ii + perm_[jj]] % 12;
        int gi1 = perm_[ii + i1 + perm_[jj + j1]] % 12;
        int gi2 = perm_[ii + 1 + perm_[jj + 1]] % 12;

        float n0, n1, n2;

        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 < 0) n0 = 0.0f;
        else {
            t0 *= t0;
            n0 = t0 * t0 * grad2d(gi0, x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 < 0) n1 = 0.0f;
        else {
            t1 *= t1;
            n1 = t1 * t1 * grad2d(gi1, x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 < 0) n2 = 0.0f;
        else {
            t2 *= t2;
            n2 = t2 * t2 * grad2d(gi2, x2, y2);
        }

        return 70.0f * (n0 + n1 + n2);
    }

    // FBM with Simplex
    float fbm(float x, float y, int octaves, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        float max_val = 0.0f;

        for (int i = 0; i < octaves; i++) {
            sum += amp * noise(x * freq, y * freq);
            max_val += amp;
            amp *= gain;
            freq *= lacunarity;
        }

        return sum / max_val;
    }

private:
    float grad2d(int gi, float x, float y) const {
        static const float grad2[][2] = {
            {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
        };
        return grad2[gi][0] * x + grad2[gi][1] * y;
    }

    PermutationTable perm_;
};

// ============================================================================
// Worley (Cellular) Noise
// ============================================================================
class WorleyNoise {
public:
    WorleyNoise(uint32_t seed = 0) : seed_(seed) {}

    void reseed(uint32_t seed) { seed_ = seed; }

    // 2D Worley noise - returns distance to nearest feature point
    // Returns value in [0, 1] range (approximately)
    float noise(float x, float y, float jitter = 1.0f) const {
        int xi = static_cast<int>(floor_f(x));
        int yi = static_cast<int>(floor_f(y));

        float min_dist = 1e10f;

        // Check surrounding cells
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int cx = xi + dx;
                int cy = yi + dy;

                // Get feature point in this cell
                Vec2 fp = feature_point(cx, cy, jitter);

                float dist = length(Vec2{x - fp.x, y - fp.y});
                min_dist = std::min(min_dist, dist);
            }
        }

        return min_dist;
    }

    // Returns F1 (nearest) and F2 (second nearest) distances
    void noise_f1f2(float x, float y, float& f1, float& f2, float jitter = 1.0f) const {
        int xi = static_cast<int>(floor_f(x));
        int yi = static_cast<int>(floor_f(y));

        f1 = f2 = 1e10f;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int cx = xi + dx;
                int cy = yi + dy;

                Vec2 fp = feature_point(cx, cy, jitter);
                float dist = length(Vec2{x - fp.x, y - fp.y});

                if (dist < f1) {
                    f2 = f1;
                    f1 = dist;
                } else if (dist < f2) {
                    f2 = dist;
                }
            }
        }
    }

    // Cellular noise with cracks (F2 - F1)
    float cracks(float x, float y, float jitter = 1.0f) const {
        float f1, f2;
        noise_f1f2(x, y, f1, f2, jitter);
        return f2 - f1;
    }

private:
    Vec2 feature_point(int cx, int cy, float jitter) const {
        // Hash cell coordinates to get random offset
        uint32_t h = hash(cx, cy, seed_);
        float rx = (h & 0xFFFF) / 65535.0f;
        float ry = ((h >> 16) & 0xFFFF) / 65535.0f;

        return Vec2{
            cx + 0.5f + jitter * (rx - 0.5f),
            cy + 0.5f + jitter * (ry - 0.5f)
        };
    }

    static uint32_t hash(int x, int y, uint32_t seed) {
        uint32_t h = seed;
        h ^= static_cast<uint32_t>(x) * 374761393u;
        h ^= static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h;
    }

    uint32_t seed_;
};

// ============================================================================
// Utility functions
// ============================================================================

// Turbulence (absolute value of FBM - creates cloudy patterns)
inline float turbulence(const PerlinNoise& noise, float x, float y, int octaves) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float max_val = 0.0f;

    for (int i = 0; i < octaves; i++) {
        sum += amp * std::abs(noise.noise(x * freq, y * freq));
        max_val += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }

    return sum / max_val;
}

// Domain warping (distort coordinates using noise)
inline float warped_noise(const PerlinNoise& noise, float x, float y, float warp_strength = 1.0f) {
    float ox = noise.noise(x + 0.0f, y + 0.0f);
    float oy = noise.noise(x + 5.2f, y + 1.3f);
    return noise.noise(x + warp_strength * ox, y + warp_strength * oy);
}

// Clamp value to [0, 1]
inline float saturate(float x) {
    return std::max(0.0f, std::min(1.0f, x));
}

// Remap value from one range to another
inline float remap(float x, float in_min, float in_max, float out_min, float out_max) {
    return out_min + (x - in_min) * (out_max - out_min) / (in_max - in_min);
}

} // namespace noise
} // namespace slam
