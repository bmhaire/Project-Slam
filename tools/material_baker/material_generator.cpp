/**
 * Slam Engine - Material Generator Implementation
 */

#include "material_generator.h"
#include <fstream>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <sys/stat.h>

namespace slam {

// ============================================================================
// Image I/O
// ============================================================================

bool Image::save_tga(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    // TGA header
    uint8_t header[18] = {};
    header[2] = 2; // Uncompressed RGB
    header[12] = width_ & 0xFF;
    header[13] = (width_ >> 8) & 0xFF;
    header[14] = height_ & 0xFF;
    header[15] = (height_ >> 8) & 0xFF;
    header[16] = 32; // Bits per pixel (RGBA)
    header[17] = 0x20; // Top-left origin

    file.write(reinterpret_cast<char*>(header), 18);

    // Write pixels as BGRA
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            const Pixel& p = at(x, y);
            uint8_t bgra[4] = {p.b, p.g, p.r, p.a};
            file.write(reinterpret_cast<char*>(bgra), 4);
        }
    }

    return true;
}

bool Image::save_png(const std::string& path) const {
    // For now, fall back to TGA
    // To use PNG, we'd need stb_image_write or similar
    std::string tga_path = path;
    if (path.length() > 4 && path.substr(path.length() - 4) == ".png") {
        tga_path = path.substr(0, path.length() - 4) + ".tga";
    }
    return save_tga(tga_path);
}

// ============================================================================
// Material Generator
// ============================================================================

MaterialGenerator::MaterialGenerator(uint32_t seed)
    : seed_(seed)
    , perlin_(seed)
    , simplex_(seed)
    , worley_(seed) {}

void MaterialGenerator::set_seed(uint32_t seed) {
    seed_ = seed;
    perlin_.reseed(seed);
    simplex_.reseed(seed);
    worley_.reseed(seed);
}

MaterialTextures MaterialGenerator::generate(MaterialType type, int resolution) {
    MaterialTextures tex(resolution);

    switch (type) {
        case MaterialType::StoneFloor:
            generate_stone_floor(tex);
            break;
        case MaterialType::StoneWall:
            generate_stone_wall(tex);
            break;
        case MaterialType::Metal:
            generate_metal(tex);
            break;
        case MaterialType::Wood:
            generate_wood(tex);
            break;
        case MaterialType::DecorativeTrim:
            generate_decorative_trim(tex);
            break;
    }

    return tex;
}

void MaterialGenerator::generate_all(int resolution, const std::string& output_dir) {
    // Create output directory
    mkdir(output_dir.c_str(), 0755);

    MaterialType types[] = {
        MaterialType::StoneFloor,
        MaterialType::StoneWall,
        MaterialType::Metal,
        MaterialType::Wood,
        MaterialType::DecorativeTrim
    };

    for (MaterialType type : types) {
        printf("Generating %s...\n", material_type_name(type));

        MaterialTextures tex = generate(type, resolution);

        std::string name = material_type_name(type);
        tex.albedo.save_tga(output_dir + "/" + name + "_albedo.tga");
        tex.normal.save_tga(output_dir + "/" + name + "_normal.tga");
        tex.roughness.save_tga(output_dir + "/" + name + "_roughness.tga");
        tex.metallic.save_tga(output_dir + "/" + name + "_metallic.tga");
        tex.ao.save_tga(output_dir + "/" + name + "_ao.tga");

        printf("  Saved %s textures\n", name.c_str());
    }
}

// ============================================================================
// Stone Floor
// ============================================================================

void MaterialGenerator::generate_stone_floor(MaterialTextures& tex) {
    int size = tex.albedo.width();
    float scale = 4.0f; // Texture scale

    std::vector<float> height(size * size);

    // Base colors
    Color stone_base(0.45f, 0.42f, 0.38f);
    Color stone_light(0.55f, 0.52f, 0.48f);
    Color stone_dark(0.30f, 0.28f, 0.25f);
    Color crack_color(0.15f, 0.12f, 0.10f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float nx = u * scale;
            float ny = v * scale;

            // Multi-octave noise for base pattern
            float base_noise = perlin_.fbm(nx, ny, 6, 2.0f, 0.5f);
            base_noise = base_noise * 0.5f + 0.5f; // Map to [0, 1]

            // Large variation (patches of different stone colors)
            float large_var = perlin_.fbm(nx * 0.5f, ny * 0.5f, 3, 2.0f, 0.5f);
            large_var = large_var * 0.5f + 0.5f;

            // Cracks using Worley noise
            float f1, f2;
            worley_.noise_f1f2(nx * 2.0f, ny * 2.0f, f1, f2, 0.9f);
            float cracks = 1.0f - noise::saturate((f2 - f1) * 3.0f);
            cracks = cracks * cracks * cracks; // Sharpen cracks

            // Fine detail noise
            float detail = perlin_.fbm(nx * 8.0f, ny * 8.0f, 4, 2.0f, 0.5f);
            detail = detail * 0.5f + 0.5f;

            // Combine for albedo
            Color base = Color::lerp(stone_dark, stone_light, large_var);
            base = Color::lerp(base, stone_base, base_noise * 0.5f);
            base = base + Color(detail * 0.1f - 0.05f); // Add fine detail

            // Add crack darkening
            base = Color::lerp(base, crack_color, cracks * 0.8f);

            // Add slight color variation
            float hue_shift = perlin_.noise(nx * 1.5f, ny * 1.5f) * 0.05f;
            base.r += hue_shift;
            base.b -= hue_shift;

            tex.albedo.at(x, y) = Pixel::from_float(base.r, base.g, base.b);

            // Height map
            float h = base_noise * 0.3f + detail * 0.2f;
            h -= cracks * 0.4f; // Cracks are lower
            height[y * size + x] = h;

            // Roughness (cracks are rougher, worn areas smoother)
            float rough = 0.7f + detail * 0.2f;
            rough += cracks * 0.2f;
            rough -= (1.0f - large_var) * 0.15f; // Worn patches smoother
            tex.roughness.at(x, y) = Pixel::gray(rough);

            // Metallic (stone is not metallic)
            tex.metallic.at(x, y) = Pixel::gray(0.0f);
        }
    }

    // Generate normal map from height
    height_to_normal(height, tex.normal, 2.0f);

    // Generate AO
    height_to_ao(height, tex.ao, 1.5f);
}

// ============================================================================
// Stone Wall
// ============================================================================

void MaterialGenerator::generate_stone_wall(MaterialTextures& tex) {
    int size = tex.albedo.width();
    float scale = 4.0f;

    std::vector<float> height(size * size);

    Color wall_base(0.50f, 0.47f, 0.42f);
    Color wall_light(0.60f, 0.55f, 0.48f);
    Color wall_dark(0.35f, 0.32f, 0.28f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float nx = u * scale;
            float ny = v * scale;

            // Horizontal stratification (stone layers)
            float layers = std::sin(ny * 8.0f + perlin_.noise(nx * 2.0f, ny * 0.5f) * 2.0f) * 0.5f + 0.5f;

            // Block pattern (subtle rectangular grid)
            float block_x = std::abs(std::sin(nx * 4.0f * noise::PI));
            float block_y = std::abs(std::sin(ny * 2.0f * noise::PI));
            float blocks = std::min(block_x, block_y);
            blocks = noise::smoothstep(blocks * blocks);

            // Vertical weathering streaks
            float streaks = perlin_.ridge(nx * 0.5f, ny * 4.0f, 4, 2.0f, 0.5f);
            streaks = 1.0f - streaks * 0.3f;

            // Base texture noise
            float base_noise = perlin_.fbm(nx * 2.0f, ny * 2.0f, 5, 2.0f, 0.5f);
            base_noise = base_noise * 0.5f + 0.5f;

            // Fine detail
            float detail = perlin_.fbm(nx * 16.0f, ny * 16.0f, 3, 2.0f, 0.5f);
            detail = detail * 0.5f + 0.5f;

            // Combine for albedo
            Color base = Color::lerp(wall_dark, wall_light, layers);
            base = Color::lerp(base, wall_base, base_noise * 0.4f);
            base = base * streaks;

            // Block edge darkening
            base = Color::lerp(base, wall_dark, (1.0f - blocks) * 0.3f);

            tex.albedo.at(x, y) = Pixel::from_float(base.r, base.g, base.b);

            // Height
            float h = 0.5f + layers * 0.2f + base_noise * 0.15f;
            h += (blocks - 0.5f) * 0.1f; // Block edges lower
            h += detail * 0.1f;
            height[y * size + x] = h;

            // Roughness
            float rough = 0.75f + detail * 0.15f;
            rough += (1.0f - streaks) * 0.1f;
            tex.roughness.at(x, y) = Pixel::gray(rough);

            tex.metallic.at(x, y) = Pixel::gray(0.0f);
        }
    }

    height_to_normal(height, tex.normal, 1.5f);
    height_to_ao(height, tex.ao, 1.0f);
}

// ============================================================================
// Metal
// ============================================================================

void MaterialGenerator::generate_metal(MaterialTextures& tex) {
    int size = tex.albedo.width();
    float scale = 8.0f;

    std::vector<float> height(size * size);

    Color metal_base(0.7f, 0.7f, 0.75f);
    Color rust_color(0.5f, 0.25f, 0.1f);
    Color dark_rust(0.3f, 0.15f, 0.08f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float nx = u * scale;
            float ny = v * scale;

            // Brushed metal (directional noise)
            float brush = perlin_.fbm(nx * 0.5f, ny * 8.0f, 4, 2.0f, 0.5f);
            brush = brush * 0.5f + 0.5f;

            // Rust patches using Worley
            float rust_noise = worley_.noise(nx * 0.5f, ny * 0.5f, 0.8f);
            float rust = noise::saturate(rust_noise * 2.0f - 1.0f);
            rust = 1.0f - rust;

            // Threshold rust to create patches
            rust = noise::saturate((rust - 0.3f) * 3.0f);
            rust = rust * rust; // Smooth falloff

            // Add noise to rust edges
            float rust_detail = perlin_.fbm(nx * 4.0f, ny * 4.0f, 4, 2.0f, 0.6f);
            rust_detail = rust_detail * 0.5f + 0.5f;
            rust = rust * rust_detail;

            // Dents (low frequency bumps)
            float dents = perlin_.fbm(nx * 0.3f, ny * 0.3f, 3, 2.0f, 0.5f);
            dents = dents * 0.5f + 0.5f;

            // Fine scratches
            float scratches = perlin_.fbm(nx * 16.0f, ny * 16.0f, 2, 2.0f, 0.5f);
            scratches = scratches * 0.5f + 0.5f;

            // Albedo
            Color base = metal_base + Color(brush * 0.1f - 0.05f);
            Color rust_col = Color::lerp(rust_color, dark_rust, rust_detail);
            base = Color::lerp(base, rust_col, rust);

            tex.albedo.at(x, y) = Pixel::from_float(base.r, base.g, base.b);

            // Height
            float h = 0.5f + brush * 0.1f - dents * 0.15f;
            h += scratches * 0.05f;
            h -= rust * 0.1f; // Rust is slightly recessed
            height[y * size + x] = h;

            // Roughness (polished metal is smooth, rust is rough)
            float rough = 0.25f + brush * 0.1f;
            rough = noise::lerp(rough, 0.7f, rust);
            tex.roughness.at(x, y) = Pixel::gray(rough);

            // Metallic (high for metal, low for rust)
            float metallic = noise::lerp(0.9f, 0.2f, rust);
            tex.metallic.at(x, y) = Pixel::gray(metallic);
        }
    }

    height_to_normal(height, tex.normal, 1.0f);
    height_to_ao(height, tex.ao, 0.5f);
}

// ============================================================================
// Wood
// ============================================================================

void MaterialGenerator::generate_wood(MaterialTextures& tex) {
    int size = tex.albedo.width();
    float scale = 4.0f;

    std::vector<float> height(size * size);

    Color wood_light(0.6f, 0.45f, 0.25f);
    Color wood_dark(0.35f, 0.22f, 0.1f);
    Color knot_color(0.25f, 0.15f, 0.08f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float nx = u * scale;
            float ny = v * scale;

            // Wood grain (rings distorted by noise)
            float grain_distort = perlin_.noise(nx * 0.5f, ny * 8.0f) * 0.5f;
            float grain = std::sin((nx + grain_distort) * 20.0f + ny * 2.0f);
            grain = grain * 0.5f + 0.5f;
            grain = grain * grain; // Sharpen grain

            // Large grain variation
            float large_grain = perlin_.fbm(nx * 0.3f, ny * 4.0f, 3, 2.0f, 0.5f);
            large_grain = large_grain * 0.5f + 0.5f;

            // Knots (circular features)
            float knot = 0.0f;
            // Add a few knots at fixed positions (seeded)
            for (int i = 0; i < 3; i++) {
                float kx = perlin_.noise(i * 10.0f, 0.0f) * 0.5f + 0.5f;
                float ky = perlin_.noise(0.0f, i * 10.0f) * 0.5f + 0.5f;
                float dist = std::sqrt((u - kx) * (u - kx) + (v - ky) * (v - ky));
                float k = noise::saturate(1.0f - dist * 8.0f);
                k = k * k;
                knot = std::max(knot, k);
            }

            // Fine detail
            float detail = perlin_.fbm(nx * 16.0f, ny * 16.0f, 3, 2.0f, 0.5f);
            detail = detail * 0.5f + 0.5f;

            // Age/wear variation
            float wear = perlin_.fbm(nx * 1.0f, ny * 1.0f, 3, 2.0f, 0.5f);
            wear = wear * 0.5f + 0.5f;

            // Albedo
            Color base = Color::lerp(wood_dark, wood_light, grain);
            base = Color::lerp(base, wood_light * 0.8f, large_grain * 0.3f);
            base = Color::lerp(base, knot_color, knot);
            base = base + Color(wear * 0.1f - 0.05f);

            tex.albedo.at(x, y) = Pixel::from_float(base.r, base.g, base.b);

            // Height
            float h = 0.5f + grain * 0.1f - knot * 0.2f;
            h += detail * 0.05f;
            height[y * size + x] = h;

            // Roughness
            float rough = 0.55f + detail * 0.1f;
            rough += knot * 0.2f;
            tex.roughness.at(x, y) = Pixel::gray(rough);

            tex.metallic.at(x, y) = Pixel::gray(0.0f);
        }
    }

    height_to_normal(height, tex.normal, 1.0f);
    height_to_ao(height, tex.ao, 1.0f);
}

// ============================================================================
// Decorative Trim
// ============================================================================

void MaterialGenerator::generate_decorative_trim(MaterialTextures& tex) {
    int size = tex.albedo.width();
    float scale = 2.0f;

    std::vector<float> height(size * size);

    Color gold_base(0.83f, 0.69f, 0.22f);
    Color gold_dark(0.6f, 0.45f, 0.1f);
    Color patina(0.3f, 0.5f, 0.35f);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float nx = u * scale;
            float ny = v * scale;

            // Geometric pattern (Celtic-like interlocking)
            float pattern = 0.0f;

            // Horizontal and vertical lines
            float line_h = std::abs(std::sin(ny * 8.0f * noise::PI));
            float line_v = std::abs(std::sin(nx * 8.0f * noise::PI));

            // Create interlocking pattern
            float diag1 = std::abs(std::sin((nx + ny) * 4.0f * noise::PI));
            float diag2 = std::abs(std::sin((nx - ny) * 4.0f * noise::PI));

            pattern = std::min(std::min(line_h, line_v), std::min(diag1, diag2));
            pattern = 1.0f - noise::smoothstep(noise::saturate(pattern * 2.0f));

            // Border frame
            float border = std::min(std::min(u, 1.0f - u), std::min(v, 1.0f - v));
            border = noise::saturate(border * 10.0f);
            border = 1.0f - border;

            // Combine pattern with border
            float relief = std::max(pattern * 0.7f, border);

            // Patina in recesses
            float patina_noise = perlin_.fbm(nx * 4.0f, ny * 4.0f, 4, 2.0f, 0.6f);
            patina_noise = patina_noise * 0.5f + 0.5f;
            float patina_amount = (1.0f - relief) * patina_noise * 0.5f;

            // Surface wear
            float wear = perlin_.fbm(nx * 8.0f, ny * 8.0f, 3, 2.0f, 0.5f);
            wear = wear * 0.5f + 0.5f;

            // Albedo
            Color base = Color::lerp(gold_dark, gold_base, relief);
            base = base + Color(wear * 0.1f - 0.05f);
            base = Color::lerp(base, patina, patina_amount);

            tex.albedo.at(x, y) = Pixel::from_float(base.r, base.g, base.b);

            // Height (pattern creates relief)
            float h = relief * 0.4f + 0.3f;
            h += wear * 0.05f;
            height[y * size + x] = h;

            // Roughness (polished on high points, rough in recesses)
            float rough = 0.35f + (1.0f - relief) * 0.3f;
            rough += patina_amount * 0.2f;
            tex.roughness.at(x, y) = Pixel::gray(rough);

            // Metallic (gold is metallic, patina less so)
            float metallic = 0.9f - patina_amount * 0.5f;
            tex.metallic.at(x, y) = Pixel::gray(metallic);
        }
    }

    height_to_normal(height, tex.normal, 2.0f);
    height_to_ao(height, tex.ao, 1.5f);
}

// ============================================================================
// Helper Functions
// ============================================================================

void MaterialGenerator::height_to_normal(const std::vector<float>& height, Image& normal, float strength) {
    int size = normal.width();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Sample neighboring heights (with wrapping)
            float h_l = height[y * size + ((x - 1 + size) % size)];
            float h_r = height[y * size + ((x + 1) % size)];
            float h_d = height[((y - 1 + size) % size) * size + x];
            float h_u = height[((y + 1) % size) * size + x];

            // Calculate gradient
            float dx = (h_r - h_l) * strength;
            float dy = (h_u - h_d) * strength;

            // Normal vector
            float nx = -dx;
            float ny = -dy;
            float nz = 1.0f;

            // Normalize
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= len;
            ny /= len;
            nz /= len;

            // Map to [0, 255] range (tangent space normal map)
            normal.at(x, y) = Pixel::from_float(
                nx * 0.5f + 0.5f,
                ny * 0.5f + 0.5f,
                nz * 0.5f + 0.5f
            );
        }
    }
}

void MaterialGenerator::height_to_ao(const std::vector<float>& height, Image& ao, float strength) {
    int size = ao.width();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float h = height[y * size + x];

            // Simple cavity AO - sample neighbors and darken if lower
            float ao_val = 0.0f;
            int samples = 0;

            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = (x + dx + size) % size;
                    int ny = (y + dy + size) % size;

                    float neighbor_h = height[ny * size + nx];
                    float diff = neighbor_h - h;

                    if (diff > 0) {
                        float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                        ao_val += diff / (dist + 1.0f);
                    }
                    samples++;
                }
            }

            ao_val = ao_val / samples * strength;
            float final_ao = 1.0f - noise::saturate(ao_val * 5.0f);

            ao.at(x, y) = Pixel::gray(final_ao);
        }
    }
}

const char* material_type_name(MaterialType type) {
    switch (type) {
        case MaterialType::StoneFloor: return "stone_floor";
        case MaterialType::StoneWall: return "stone_wall";
        case MaterialType::Metal: return "metal";
        case MaterialType::Wood: return "wood";
        case MaterialType::DecorativeTrim: return "decorative_trim";
        default: return "unknown";
    }
}

} // namespace slam
