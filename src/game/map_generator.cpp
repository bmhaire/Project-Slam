/**
 * Slam Engine - Map Generator Implementation
 */

#include "map_generator.h"
#include <algorithm>
#include <queue>
#include <cmath>

namespace slam {

MapGenerator::MapGenerator()
    : seed_(12345), rng_(seed_) {
}

MapGenerator::MapGenerator(uint32_t seed)
    : seed_(seed), rng_(seed) {
}

void MapGenerator::generate(int width, int height) {
    width_ = width;
    height_ = height;
    cells_.resize(width * height, CellType::Wall);
    rooms_.clear();
    spawns_.clear();
    props_.clear();

    // Step 1: Random initialization
    initialize_random();

    // Step 2: Cellular automata smoothing
    for (int i = 0; i < smoothing_iterations_; i++) {
        apply_cellular_automata();
    }

    // Step 3: Detect rooms
    detect_rooms();

    // Step 4: Connect rooms
    connect_rooms();

    // Step 5: Place spawn points
    place_spawns(4);

    // Step 6: Place props
    place_props();
}

void MapGenerator::initialize_random() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            // Border is always wall
            if (x == 0 || x == width_ - 1 || y == 0 || y == height_ - 1) {
                cells_[y * width_ + x] = CellType::Wall;
            } else {
                cells_[y * width_ + x] = (dist(rng_) < fill_ratio_) ?
                    CellType::Wall : CellType::Floor;
            }
        }
    }
}

void MapGenerator::apply_cellular_automata() {
    std::vector<CellType> new_cells = cells_;

    for (int y = 1; y < height_ - 1; y++) {
        for (int x = 1; x < width_ - 1; x++) {
            int walls = count_wall_neighbors(x, y);

            if (walls > wall_threshold_) {
                new_cells[y * width_ + x] = CellType::Wall;
            } else if (walls < wall_threshold_) {
                new_cells[y * width_ + x] = CellType::Floor;
            }
            // If equal, keep current state
        }
    }

    cells_ = std::move(new_cells);
}

int MapGenerator::count_wall_neighbors(int x, int y) const {
    int count = 0;

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (!in_bounds(nx, ny) || is_wall(nx, ny)) {
                count++;
            }
        }
    }

    return count;
}

void MapGenerator::detect_rooms() {
    std::vector<int> room_map(width_ * height_, -1);
    int room_id = 0;

    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            if (is_floor(x, y) && room_map[y * width_ + x] == -1) {
                // Start new room
                Room room;
                room.x = x;
                room.y = y;
                room.width = 0;
                room.height = 0;
                room.area = 0;
                room.is_main = false;

                flood_fill_room(x, y, room_id, room_map);

                // Calculate room bounds and center
                int min_x = width_, max_x = 0;
                int min_y = height_, max_y = 0;
                float sum_x = 0, sum_y = 0;
                int count = 0;

                for (int ry = 0; ry < height_; ry++) {
                    for (int rx = 0; rx < width_; rx++) {
                        if (room_map[ry * width_ + rx] == room_id) {
                            min_x = std::min(min_x, rx);
                            max_x = std::max(max_x, rx);
                            min_y = std::min(min_y, ry);
                            max_y = std::max(max_y, ry);
                            sum_x += rx;
                            sum_y += ry;
                            count++;
                        }
                    }
                }

                room.x = min_x;
                room.y = min_y;
                room.width = max_x - min_x + 1;
                room.height = max_y - min_y + 1;
                room.center = vec2(sum_x / count, sum_y / count);
                room.area = count;

                // Only keep rooms above minimum size
                if (room.area >= min_room_size_) {
                    rooms_.push_back(room);
                    room_id++;
                } else {
                    // Fill small rooms with walls
                    for (int ry = 0; ry < height_; ry++) {
                        for (int rx = 0; rx < width_; rx++) {
                            if (room_map[ry * width_ + rx] == room_id) {
                                cells_[ry * width_ + rx] = CellType::Wall;
                                room_map[ry * width_ + rx] = -1;
                            }
                        }
                    }
                }
            }
        }
    }

    // Mark main room (largest)
    if (!rooms_.empty()) {
        auto largest = std::max_element(rooms_.begin(), rooms_.end(),
            [](const Room& a, const Room& b) { return a.area < b.area; });
        largest->is_main = true;
    }
}

void MapGenerator::flood_fill_room(int x, int y, int room_id, std::vector<int>& room_map) {
    std::queue<std::pair<int, int>> queue;
    queue.push({x, y});
    room_map[y * width_ + x] = room_id;

    while (!queue.empty()) {
        auto [cx, cy] = queue.front();
        queue.pop();

        // Check 4-connected neighbors
        const int dx[] = {0, 1, 0, -1};
        const int dy[] = {-1, 0, 1, 0};

        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (in_bounds(nx, ny) && is_floor(nx, ny) &&
                room_map[ny * width_ + nx] == -1) {
                room_map[ny * width_ + nx] = room_id;
                queue.push({nx, ny});
            }
        }
    }
}

void MapGenerator::connect_rooms() {
    if (rooms_.size() < 2) return;

    // Connect each room to its nearest neighbor using MST approach
    std::vector<bool> connected(rooms_.size(), false);
    connected[0] = true;
    int connected_count = 1;

    while (connected_count < static_cast<int>(rooms_.size())) {
        float best_dist = std::numeric_limits<float>::max();
        int best_from = -1;
        int best_to = -1;

        // Find closest unconnected room to any connected room
        for (size_t i = 0; i < rooms_.size(); i++) {
            if (!connected[i]) continue;

            for (size_t j = 0; j < rooms_.size(); j++) {
                if (connected[j]) continue;

                float dx = rooms_[i].center.x - rooms_[j].center.x;
                float dy = rooms_[i].center.y - rooms_[j].center.y;
                float dist = dx * dx + dy * dy;

                if (dist < best_dist) {
                    best_dist = dist;
                    best_from = static_cast<int>(i);
                    best_to = static_cast<int>(j);
                }
            }
        }

        if (best_from >= 0 && best_to >= 0) {
            create_corridor(rooms_[best_from], rooms_[best_to]);
            connected[best_to] = true;
            connected_count++;
        }
    }
}

void MapGenerator::create_corridor(const Room& a, const Room& b) {
    int x0 = static_cast<int>(a.center.x);
    int y0 = static_cast<int>(a.center.y);
    int x1 = static_cast<int>(b.center.x);
    int y1 = static_cast<int>(b.center.y);

    // Random decision: horizontal first or vertical first
    std::uniform_int_distribution<int> dist(0, 1);
    bool horizontal_first = dist(rng_) == 0;

    int corridor_width = 2;

    if (horizontal_first) {
        // Horizontal then vertical
        for (int x = std::min(x0, x1); x <= std::max(x0, x1); x++) {
            for (int w = -corridor_width; w <= corridor_width; w++) {
                int y = y0 + w;
                if (in_bounds(x, y)) {
                    cells_[y * width_ + x] = CellType::Floor;
                }
            }
        }
        for (int y = std::min(y0, y1); y <= std::max(y0, y1); y++) {
            for (int w = -corridor_width; w <= corridor_width; w++) {
                int x = x1 + w;
                if (in_bounds(x, y)) {
                    cells_[y * width_ + x] = CellType::Floor;
                }
            }
        }
    } else {
        // Vertical then horizontal
        for (int y = std::min(y0, y1); y <= std::max(y0, y1); y++) {
            for (int w = -corridor_width; w <= corridor_width; w++) {
                int x = x0 + w;
                if (in_bounds(x, y)) {
                    cells_[y * width_ + x] = CellType::Floor;
                }
            }
        }
        for (int x = std::min(x0, x1); x <= std::max(x0, x1); x++) {
            for (int w = -corridor_width; w <= corridor_width; w++) {
                int y = y1 + w;
                if (in_bounds(x, y)) {
                    cells_[y * width_ + x] = CellType::Floor;
                }
            }
        }
    }
}

void MapGenerator::place_spawns(int count) {
    if (rooms_.empty()) return;

    // Distribute spawns across different rooms
    std::vector<int> room_indices;
    for (int i = 0; i < static_cast<int>(rooms_.size()); i++) {
        room_indices.push_back(i);
    }

    // Shuffle rooms
    std::shuffle(room_indices.begin(), room_indices.end(), rng_);

    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);

    for (int i = 0; i < count && i < static_cast<int>(rooms_.size()); i++) {
        const Room& room = rooms_[room_indices[i]];

        // Find a valid floor position in the room
        std::vector<std::pair<int, int>> floor_cells;
        for (int y = room.y; y < room.y + room.height; y++) {
            for (int x = room.x; x < room.x + room.width; x++) {
                if (is_floor(x, y)) {
                    // Check if far enough from walls
                    int walls = count_wall_neighbors(x, y);
                    if (walls == 0) {
                        floor_cells.push_back({x, y});
                    }
                }
            }
        }

        if (!floor_cells.empty()) {
            std::uniform_int_distribution<int> cell_dist(0, floor_cells.size() - 1);
            auto [sx, sy] = floor_cells[cell_dist(rng_)];

            SpawnPoint spawn;
            spawn.position = cell_to_world(sx, sy);
            spawn.position.y = 0.0f;  // Ground level
            spawn.rotation = angle_dist(rng_);
            spawn.room_id = room_indices[i];

            spawns_.push_back(spawn);
            cells_[sy * width_ + sx] = CellType::Spawn;
        }
    }
}

void MapGenerator::place_props() {
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> scale_dist(0.8f, 1.2f);
    std::uniform_int_distribution<int> type_dist(0, 2);

    // Place props near walls but on floor
    for (int y = 2; y < height_ - 2; y++) {
        for (int x = 2; x < width_ - 2; x++) {
            if (!is_floor(x, y)) continue;

            // Check if adjacent to wall
            bool near_wall = false;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (is_wall(x + dx, y + dy)) {
                        near_wall = true;
                        break;
                    }
                }
                if (near_wall) break;
            }

            if (near_wall && prob_dist(rng_) < 0.02f) {
                // Check we're not blocking a path (at least 2 adjacent floors)
                int adjacent_floors = 0;
                if (is_floor(x - 1, y)) adjacent_floors++;
                if (is_floor(x + 1, y)) adjacent_floors++;
                if (is_floor(x, y - 1)) adjacent_floors++;
                if (is_floor(x, y + 1)) adjacent_floors++;

                if (adjacent_floors >= 2) {
                    PropPlacement prop;
                    prop.position = cell_to_world(x, y);
                    prop.position.y = 0.0f;
                    prop.rotation = angle_dist(rng_);
                    prop.prop_type = type_dist(rng_);
                    prop.scale = scale_dist(rng_);

                    props_.push_back(prop);
                    cells_[y * width_ + x] = CellType::Prop;
                }
            }
        }
    }

    // Place columns in large open areas
    for (const Room& room : rooms_) {
        if (room.area > 500) {
            // Large room - add some columns
            int num_columns = room.area / 200;
            std::uniform_int_distribution<int> x_dist(room.x + 3, room.x + room.width - 4);
            std::uniform_int_distribution<int> y_dist(room.y + 3, room.y + room.height - 4);

            for (int i = 0; i < num_columns; i++) {
                int cx = x_dist(rng_);
                int cy = y_dist(rng_);

                if (is_floor(cx, cy) && count_wall_neighbors(cx, cy) == 0) {
                    PropPlacement prop;
                    prop.position = cell_to_world(cx, cy);
                    prop.position.y = 0.0f;
                    prop.rotation = 0.0f;
                    prop.prop_type = 0;  // Column
                    prop.scale = 1.5f;

                    props_.push_back(prop);
                    cells_[cy * width_ + cx] = CellType::Prop;
                }
            }
        }
    }
}

CellType MapGenerator::get_cell(int x, int y) const {
    if (!in_bounds(x, y)) return CellType::Wall;
    return cells_[y * width_ + x];
}

void MapGenerator::set_cell(int x, int y, CellType type) {
    if (in_bounds(x, y)) {
        cells_[y * width_ + x] = type;
    }
}

bool MapGenerator::is_wall(int x, int y) const {
    return get_cell(x, y) == CellType::Wall;
}

bool MapGenerator::is_floor(int x, int y) const {
    CellType cell = get_cell(x, y);
    return cell == CellType::Floor || cell == CellType::Spawn || cell == CellType::Prop;
}

bool MapGenerator::in_bounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

vec3 MapGenerator::cell_to_world(int x, int y) const {
    return vec3(
        (x - width_ / 2.0f) * cell_size_,
        0.0f,
        (y - height_ / 2.0f) * cell_size_
    );
}

void MapGenerator::world_to_cell(const vec3& pos, int& x, int& y) const {
    x = static_cast<int>(pos.x / cell_size_ + width_ / 2.0f);
    y = static_cast<int>(pos.z / cell_size_ + height_ / 2.0f);
}

const Room* MapGenerator::get_room(int index) const {
    if (index >= 0 && index < static_cast<int>(rooms_.size())) {
        return &rooms_[index];
    }
    return nullptr;
}

} // namespace slam
