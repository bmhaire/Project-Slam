/**
 * Slam Engine - Map Generator
 *
 * Procedural dungeon/arena generation using cellular automata
 */

#pragma once

#include "utils/math.h"
#include <vector>
#include <cstdint>
#include <random>

namespace slam {

// Map cell types
enum class CellType : uint8_t {
    Wall = 0,
    Floor = 1,
    Spawn = 2,
    Prop = 3
};

// Room data
struct Room {
    int x, y;           // Top-left corner
    int width, height;  // Dimensions
    vec2 center;        // Center position
    int area;           // Total floor cells
    bool is_main;       // Is main/largest room
};

// Spawn point
struct SpawnPoint {
    vec3 position;
    float rotation;     // Y-axis rotation
    int room_id;        // Which room this spawn is in
};

// Prop placement
struct PropPlacement {
    vec3 position;
    float rotation;
    int prop_type;      // 0 = column, 1 = crate, 2 = barrel, etc.
    float scale;
};

class MapGenerator {
public:
    MapGenerator();
    explicit MapGenerator(uint32_t seed);

    // Generate map
    void generate(int width = 1024, int height = 1024);

    // Generation parameters
    void set_fill_ratio(float ratio) { fill_ratio_ = ratio; }
    void set_smoothing_iterations(int n) { smoothing_iterations_ = n; }
    void set_min_room_size(int size) { min_room_size_ = size; }
    void set_wall_threshold(int n) { wall_threshold_ = n; }

    // Access map data
    CellType get_cell(int x, int y) const;
    void set_cell(int x, int y, CellType type);
    bool is_wall(int x, int y) const;
    bool is_floor(int x, int y) const;
    bool in_bounds(int x, int y) const;

    // Map dimensions
    int width() const { return width_; }
    int height() const { return height_; }
    float cell_size() const { return cell_size_; }

    // World coordinates
    vec3 cell_to_world(int x, int y) const;
    void world_to_cell(const vec3& pos, int& x, int& y) const;

    // Room data
    const std::vector<Room>& rooms() const { return rooms_; }
    const Room* get_room(int index) const;
    int room_count() const { return static_cast<int>(rooms_.size()); }

    // Spawn points
    const std::vector<SpawnPoint>& spawns() const { return spawns_; }
    int spawn_count() const { return static_cast<int>(spawns_.size()); }

    // Props
    const std::vector<PropPlacement>& props() const { return props_; }
    int prop_count() const { return static_cast<int>(props_.size()); }

    // Raw data access
    const std::vector<CellType>& data() const { return cells_; }

private:
    void initialize_random();
    void apply_cellular_automata();
    int count_wall_neighbors(int x, int y) const;

    void detect_rooms();
    void flood_fill_room(int x, int y, int room_id, std::vector<int>& room_map);
    void connect_rooms();
    void create_corridor(const Room& a, const Room& b);

    void place_spawns(int count = 4);
    void place_props();

    // Map data
    int width_ = 0;
    int height_ = 0;
    float cell_size_ = 1.0f;  // World units per cell
    std::vector<CellType> cells_;

    // Rooms
    std::vector<Room> rooms_;

    // Spawns and props
    std::vector<SpawnPoint> spawns_;
    std::vector<PropPlacement> props_;

    // Generation parameters
    float fill_ratio_ = 0.45f;
    int smoothing_iterations_ = 5;
    int min_room_size_ = 50;
    int wall_threshold_ = 4;

    // Random
    std::mt19937 rng_;
    uint32_t seed_;
};

} // namespace slam
