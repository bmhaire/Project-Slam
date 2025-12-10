/**
 * Slam Engine - Material Baker Tool
 *
 * Standalone executable for generating procedural PBR textures
 *
 * Usage:
 *   material_baker [options]
 *
 * Options:
 *   --output <dir>     Output directory (default: ./materials)
 *   --resolution <n>   Texture resolution (default: 2048)
 *   --seed <n>         Random seed (default: 12345)
 *   --type <name>      Generate specific type (stone_floor, stone_wall, metal, wood, decorative_trim)
 *   --all              Generate all material types (default)
 *   --help             Show this help message
 */

#include "material_generator.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>

void print_usage(const char* program_name) {
    printf("Slam Engine - Material Baker\n");
    printf("Procedural PBR texture generator\n\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  --output <dir>     Output directory (default: ./materials)\n");
    printf("  --resolution <n>   Texture resolution (default: 2048)\n");
    printf("  --seed <n>         Random seed (default: 12345)\n");
    printf("  --type <name>      Generate specific type:\n");
    printf("                       stone_floor, stone_wall, metal, wood, decorative_trim\n");
    printf("  --all              Generate all material types (default)\n");
    printf("  --help             Show this help message\n");
}

slam::MaterialType parse_material_type(const char* name) {
    if (strcmp(name, "stone_floor") == 0) return slam::MaterialType::StoneFloor;
    if (strcmp(name, "stone_wall") == 0) return slam::MaterialType::StoneWall;
    if (strcmp(name, "metal") == 0) return slam::MaterialType::Metal;
    if (strcmp(name, "wood") == 0) return slam::MaterialType::Wood;
    if (strcmp(name, "decorative_trim") == 0) return slam::MaterialType::DecorativeTrim;
    return slam::MaterialType::StoneFloor; // Default
}

int main(int argc, char* argv[]) {
    // Default options
    std::string output_dir = "./materials";
    int resolution = 2048;
    uint32_t seed = 12345;
    bool generate_all = true;
    slam::MaterialType specific_type = slam::MaterialType::StoneFloor;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        }
        else if (strcmp(argv[i], "--resolution") == 0 && i + 1 < argc) {
            resolution = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = static_cast<uint32_t>(atoi(argv[++i]));
        }
        else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            specific_type = parse_material_type(argv[++i]);
            generate_all = false;
        }
        else if (strcmp(argv[i], "--all") == 0) {
            generate_all = true;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Validate resolution
    if (resolution < 64 || resolution > 8192) {
        printf("Resolution must be between 64 and 8192\n");
        return 1;
    }

    printf("Material Baker\n");
    printf("  Output:     %s\n", output_dir.c_str());
    printf("  Resolution: %dx%d\n", resolution, resolution);
    printf("  Seed:       %u\n", seed);
    printf("\n");

    // Create generator
    slam::MaterialGenerator generator(seed);

    auto start_time = std::chrono::high_resolution_clock::now();

    if (generate_all) {
        generator.generate_all(resolution, output_dir);
    } else {
        printf("Generating %s...\n", slam::material_type_name(specific_type));

        slam::MaterialTextures tex = generator.generate(specific_type, resolution);

        std::string name = slam::material_type_name(specific_type);
        tex.albedo.save_tga(output_dir + "/" + name + "_albedo.tga");
        tex.normal.save_tga(output_dir + "/" + name + "_normal.tga");
        tex.roughness.save_tga(output_dir + "/" + name + "_roughness.tga");
        tex.metallic.save_tga(output_dir + "/" + name + "_metallic.tga");
        tex.ao.save_tga(output_dir + "/" + name + "_ao.tga");

        printf("  Saved %s textures\n", name.c_str());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    printf("\nGeneration complete in %.2f seconds\n", duration.count() / 1000.0f);

    return 0;
}
