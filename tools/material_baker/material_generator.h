/**
 * Slam Engine - Material Generator
 *
 * Procedural PBR texture generation for 5 material types:
 * - Stone Floor
 * - Stone Wall
 * - Metal (Columns/Props)
 * - Wood (Props/Decoration)
 * - Decorative Trim
 */

#pragma once

#include "noise.h"
#include <vector>
#include <string>
#include <cstdint>

namespace slam {

// RGBA pixel
struct Pixel {
    uint8_t r, g, b, a;

    Pixel() : r(0), g(0), b(0), a(255) {}
    Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    static Pixel from_float(float r, float g, float b, float a = 1.0f) {
        return Pixel(
            static_cast<uint8_t>(noise::saturate(r) * 255),
            static_cast<uint8_t>(noise::saturate(g) * 255),
            static_cast<uint8_t>(noise::saturate(b) * 255),
            static_cast<uint8_t>(noise::saturate(a) * 255)
        );
    }

    static Pixel gray(float v) { return from_float(v, v, v); }
};

// RGB color (floating point)
struct Color {
    float r, g, b;

    Color() : r(0), g(0), b(0) {}
    Color(float r, float g, float b) : r(r), g(g), b(b) {}
    Color(float v) : r(v), g(v), b(v) {}

    Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b); }
    Color operator-(const Color& c) const { return Color(r - c.r, g - c.g, b - c.b); }
    Color operator*(float s) const { return Color(r * s, g * s, b * s); }
    Color operator*(const Color& c) const { return Color(r * c.r, g * c.g, b * c.b); }

    static Color lerp(const Color& a, const Color& b, float t) {
        return Color(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t
        );
    }
};

// Image buffer
class Image {
public:
    Image(int width, int height) : width_(width), height_(height), data_(width * height) {}

    int width() const { return width_; }
    int height() const { return height_; }

    Pixel& at(int x, int y) { return data_[y * width_ + x]; }
    const Pixel& at(int x, int y) const { return data_[y * width_ + x]; }

    // Sample with UV coordinates [0,1]
    Pixel sample(float u, float v) const {
        int x = static_cast<int>(u * (width_ - 1)) % width_;
        int y = static_cast<int>(v * (height_ - 1)) % height_;
        if (x < 0) x += width_;
        if (y < 0) y += height_;
        return data_[y * width_ + x];
    }

    // Save as TGA (simple format, no dependencies)
    bool save_tga(const std::string& path) const;

    // Save as PNG (using stb_image_write if available)
    bool save_png(const std::string& path) const;

    const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(data_.data()); }

private:
    int width_, height_;
    std::vector<Pixel> data_;
};

// PBR Material textures
struct MaterialTextures {
    Image albedo;
    Image normal;
    Image roughness;
    Image metallic;
    Image ao;

    MaterialTextures(int size)
        : albedo(size, size)
        , normal(size, size)
        , roughness(size, size)
        , metallic(size, size)
        , ao(size, size) {}
};

// Material type enum
enum class MaterialType {
    StoneFloor,
    StoneWall,
    Metal,
    Wood,
    DecorativeTrim
};

// Material generator class
class MaterialGenerator {
public:
    MaterialGenerator(uint32_t seed = 0);

    void set_seed(uint32_t seed);

    // Generate specific material type
    MaterialTextures generate(MaterialType type, int resolution);

    // Generate all material types
    void generate_all(int resolution, const std::string& output_dir);

private:
    // Individual material generators
    void generate_stone_floor(MaterialTextures& tex);
    void generate_stone_wall(MaterialTextures& tex);
    void generate_metal(MaterialTextures& tex);
    void generate_wood(MaterialTextures& tex);
    void generate_decorative_trim(MaterialTextures& tex);

    // Helper: Generate normal map from height map
    void height_to_normal(const std::vector<float>& height, Image& normal, float strength = 1.0f);

    // Helper: Generate AO from height map (simple cavity darkening)
    void height_to_ao(const std::vector<float>& height, Image& ao, float strength = 1.0f);

    uint32_t seed_;
    noise::PerlinNoise perlin_;
    noise::SimplexNoise simplex_;
    noise::WorleyNoise worley_;
};

// Convert material type to string
const char* material_type_name(MaterialType type);

} // namespace slam
