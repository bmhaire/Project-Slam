/**
 * Slam Engine - Map Mesh Generator Implementation
 */

#include "map_mesh.h"
#include "renderer/vulkan_context.h"
#include <cmath>

namespace slam {

bool MapMesh::generate(VulkanContext& context, const MapGenerator& map,
                       const MapMeshConfig& config) {
    // Generate floor
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        generate_floor(map, config, vertices, indices);

        if (!vertices.empty()) {
            floor_mesh_.create(context, vertices, indices);
        }
    }

    // Generate walls
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        if (config.smooth_walls) {
            generate_walls_marching(map, config, vertices, indices);
        } else {
            generate_walls_simple(map, config, vertices, indices);
        }

        if (!vertices.empty()) {
            wall_mesh_.create(context, vertices, indices);
        }
    }

    // Generate ceiling
    if (config.generate_ceiling) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        generate_ceiling(map, config, vertices, indices);

        if (!vertices.empty()) {
            ceiling_mesh_.create(context, vertices, indices);
        }
    }

    return true;
}

void MapMesh::destroy() {
    floor_mesh_.destroy();
    wall_mesh_.destroy();
    ceiling_mesh_.destroy();
}

void MapMesh::generate_floor(const MapGenerator& map, const MapMeshConfig& config,
                             std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vec3 floor_color(0.3f, 0.3f, 0.35f);  // Stone floor color
    vec3 normal(0.0f, 1.0f, 0.0f);

    int width = map.width();
    int height = map.height();
    float cell_size = map.cell_size();

    // Generate floor quads for each floor cell
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (map.is_floor(x, y)) {
                vec3 world = map.cell_to_world(x, y);
                float half = cell_size * 0.5f;

                vec3 p0(world.x - half, config.floor_height, world.z - half);
                vec3 p1(world.x + half, config.floor_height, world.z - half);
                vec3 p2(world.x + half, config.floor_height, world.z + half);
                vec3 p3(world.x - half, config.floor_height, world.z + half);

                add_quad(vertices, indices, p0, p1, p2, p3, normal, floor_color,
                        config.uv_scale, config.uv_scale);
            }
        }
    }
}

void MapMesh::generate_walls_simple(const MapGenerator& map, const MapMeshConfig& config,
                                    std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vec3 wall_color(0.4f, 0.35f, 0.3f);  // Stone wall color

    int width = map.width();
    int height = map.height();
    float cell_size = map.cell_size();

    // Check each floor cell for adjacent walls and generate wall faces
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!map.is_floor(x, y)) continue;

            vec3 world = map.cell_to_world(x, y);
            float half = cell_size * 0.5f;

            // Check each direction
            // North (-Z)
            if (map.is_wall(x, y - 1)) {
                vec3 start(world.x - half, config.floor_height, world.z - half);
                vec3 end(world.x + half, config.floor_height, world.z - half);
                add_wall_segment(vertices, indices, start, end, config.wall_height,
                               config.floor_height, wall_color, config.uv_scale);
            }

            // South (+Z)
            if (map.is_wall(x, y + 1)) {
                vec3 start(world.x + half, config.floor_height, world.z + half);
                vec3 end(world.x - half, config.floor_height, world.z + half);
                add_wall_segment(vertices, indices, start, end, config.wall_height,
                               config.floor_height, wall_color, config.uv_scale);
            }

            // West (-X)
            if (map.is_wall(x - 1, y)) {
                vec3 start(world.x - half, config.floor_height, world.z + half);
                vec3 end(world.x - half, config.floor_height, world.z - half);
                add_wall_segment(vertices, indices, start, end, config.wall_height,
                               config.floor_height, wall_color, config.uv_scale);
            }

            // East (+X)
            if (map.is_wall(x + 1, y)) {
                vec3 start(world.x + half, config.floor_height, world.z - half);
                vec3 end(world.x + half, config.floor_height, world.z + half);
                add_wall_segment(vertices, indices, start, end, config.wall_height,
                               config.floor_height, wall_color, config.uv_scale);
            }
        }
    }
}

void MapMesh::generate_walls_marching(const MapGenerator& map, const MapMeshConfig& config,
                                      std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vec3 wall_color(0.4f, 0.35f, 0.3f);

    int width = map.width();
    int height = map.height();
    float cell_size = map.cell_size();

    // Marching squares lookup table for wall edges
    // Each entry defines which edges have walls: 0=none, bits: 1=N, 2=E, 4=S, 8=W
    // Configuration is based on 2x2 cell pattern (TL, TR, BL, BR)

    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            // Get 2x2 cell configuration
            bool tl = !map.is_floor(x, y);      // Top-left
            bool tr = !map.is_floor(x + 1, y);  // Top-right
            bool bl = !map.is_floor(x, y + 1);  // Bottom-left
            bool br = !map.is_floor(x + 1, y + 1); // Bottom-right

            int config_idx = (tl ? 1 : 0) | (tr ? 2 : 0) | (bl ? 4 : 0) | (br ? 8 : 0);

            if (config_idx == 0 || config_idx == 15) continue;  // All floor or all wall

            // Cell center in world space
            vec3 center = map.cell_to_world(x, y);
            center.x += cell_size * 0.5f;
            center.z += cell_size * 0.5f;
            float half = cell_size * 0.5f;

            // Midpoints
            vec3 n(center.x, config.floor_height, center.z - half);  // North
            vec3 s(center.x, config.floor_height, center.z + half);  // South
            vec3 w(center.x - half, config.floor_height, center.z);  // West
            vec3 e(center.x + half, config.floor_height, center.z);  // East

            // Generate wall segments based on marching squares configuration
            switch (config_idx) {
                case 1:  // TL only
                    add_wall_segment(vertices, indices, n, w, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 2:  // TR only
                    add_wall_segment(vertices, indices, e, n, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 3:  // TL + TR
                    add_wall_segment(vertices, indices, e, w, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 4:  // BL only
                    add_wall_segment(vertices, indices, w, s, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 5:  // TL + BL
                    add_wall_segment(vertices, indices, n, s, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 6:  // TR + BL (saddle)
                    add_wall_segment(vertices, indices, e, n, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    add_wall_segment(vertices, indices, w, s, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 7:  // TL + TR + BL
                    add_wall_segment(vertices, indices, e, s, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 8:  // BR only
                    add_wall_segment(vertices, indices, s, e, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 9:  // TL + BR (saddle)
                    add_wall_segment(vertices, indices, n, w, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    add_wall_segment(vertices, indices, s, e, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 10: // TR + BR
                    add_wall_segment(vertices, indices, s, n, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 11: // TL + TR + BR
                    add_wall_segment(vertices, indices, s, w, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 12: // BL + BR
                    add_wall_segment(vertices, indices, w, e, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 13: // TL + BL + BR
                    add_wall_segment(vertices, indices, n, e, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
                case 14: // TR + BL + BR
                    add_wall_segment(vertices, indices, w, n, config.wall_height,
                                   config.floor_height, wall_color, config.uv_scale);
                    break;
            }
        }
    }
}

void MapMesh::generate_ceiling(const MapGenerator& map, const MapMeshConfig& config,
                               std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vec3 ceiling_color(0.25f, 0.25f, 0.28f);
    vec3 normal(0.0f, -1.0f, 0.0f);  // Facing down

    int width = map.width();
    int height = map.height();
    float cell_size = map.cell_size();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (map.is_floor(x, y)) {
                vec3 world = map.cell_to_world(x, y);
                float half = cell_size * 0.5f;

                // Ceiling faces down, so wind counter-clockwise from below
                vec3 p0(world.x - half, config.ceiling_height, world.z + half);
                vec3 p1(world.x + half, config.ceiling_height, world.z + half);
                vec3 p2(world.x + half, config.ceiling_height, world.z - half);
                vec3 p3(world.x - half, config.ceiling_height, world.z - half);

                add_quad(vertices, indices, p0, p1, p2, p3, normal, ceiling_color,
                        config.uv_scale, config.uv_scale);
            }
        }
    }
}

void MapMesh::add_quad(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                       const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                       const vec3& normal, const vec3& color,
                       float u_scale, float v_scale) {
    uint32_t base = static_cast<uint32_t>(vertices.size());

    // Calculate UVs based on world position
    Vertex v0, v1, v2, v3;

    v0.position = p0;
    v0.normal = normal;
    v0.color = color;
    v0.uv = vec2(p0.x * u_scale, p0.z * v_scale);

    v1.position = p1;
    v1.normal = normal;
    v1.color = color;
    v1.uv = vec2(p1.x * u_scale, p1.z * v_scale);

    v2.position = p2;
    v2.normal = normal;
    v2.color = color;
    v2.uv = vec2(p2.x * u_scale, p2.z * v_scale);

    v3.position = p3;
    v3.normal = normal;
    v3.color = color;
    v3.uv = vec2(p3.x * u_scale, p3.z * v_scale);

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    // Two triangles
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
}

void MapMesh::add_wall_segment(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                               const vec3& start, const vec3& end, float height, float floor_y,
                               const vec3& color, float uv_scale) {
    // Calculate wall normal (perpendicular to wall direction, facing outward)
    vec3 dir = end - start;
    vec3 normal = normalize(vec3(-dir.z, 0.0f, dir.x));

    // Wall corners
    vec3 p0 = start;
    vec3 p1 = end;
    vec3 p2 = vec3(end.x, floor_y + height, end.z);
    vec3 p3 = vec3(start.x, floor_y + height, start.z);

    uint32_t base = static_cast<uint32_t>(vertices.size());

    // Calculate UV based on wall length and height
    float wall_length = length(dir);

    Vertex v0, v1, v2, v3;

    v0.position = p0;
    v0.normal = normal;
    v0.color = color;
    v0.uv = vec2(0.0f, 0.0f);

    v1.position = p1;
    v1.normal = normal;
    v1.color = color;
    v1.uv = vec2(wall_length * uv_scale, 0.0f);

    v2.position = p2;
    v2.normal = normal;
    v2.color = color;
    v2.uv = vec2(wall_length * uv_scale, height * uv_scale);

    v3.position = p3;
    v3.normal = normal;
    v3.color = color;
    v3.uv = vec2(0.0f, height * uv_scale);

    vertices.push_back(v0);
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
}

// ============================================================================
// Prop Mesh Generator
// ============================================================================

std::unique_ptr<Mesh> PropMeshGenerator::generate_column(VulkanContext& context,
                                                         float radius, float height) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const int segments = 12;
    vec3 color(0.5f, 0.45f, 0.4f);  // Stone color

    // Generate cylinder
    for (int i = 0; i <= segments; i++) {
        float angle = (float(i) / segments) * 2.0f * M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        vec3 normal = normalize(vec3(x, 0.0f, z));
        float u = float(i) / segments;

        // Bottom vertex
        Vertex vb;
        vb.position = vec3(x, 0.0f, z);
        vb.normal = normal;
        vb.color = color;
        vb.uv = vec2(u, 0.0f);
        vertices.push_back(vb);

        // Top vertex
        Vertex vt;
        vt.position = vec3(x, height, z);
        vt.normal = normal;
        vt.color = color;
        vt.uv = vec2(u, height * 0.25f);
        vertices.push_back(vt);
    }

    // Generate side indices
    for (int i = 0; i < segments; i++) {
        uint32_t b0 = i * 2;
        uint32_t t0 = i * 2 + 1;
        uint32_t b1 = (i + 1) * 2;
        uint32_t t1 = (i + 1) * 2 + 1;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t1);
        indices.push_back(b0);
        indices.push_back(t1);
        indices.push_back(t0);
    }

    // Top cap
    uint32_t center_top = static_cast<uint32_t>(vertices.size());
    Vertex vc_top;
    vc_top.position = vec3(0.0f, height, 0.0f);
    vc_top.normal = vec3(0.0f, 1.0f, 0.0f);
    vc_top.color = color;
    vc_top.uv = vec2(0.5f, 0.5f);
    vertices.push_back(vc_top);

    for (int i = 0; i <= segments; i++) {
        float angle = (float(i) / segments) * 2.0f * M_PI;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;

        Vertex v;
        v.position = vec3(x, height, z);
        v.normal = vec3(0.0f, 1.0f, 0.0f);
        v.color = color;
        v.uv = vec2(0.5f + x / (2.0f * radius), 0.5f + z / (2.0f * radius));
        vertices.push_back(v);
    }

    for (int i = 0; i < segments; i++) {
        indices.push_back(center_top);
        indices.push_back(center_top + 1 + i);
        indices.push_back(center_top + 2 + i);
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->create(context, vertices, indices);
    return mesh;
}

std::unique_ptr<Mesh> PropMeshGenerator::generate_crate(VulkanContext& context, float size) {
    auto mesh = std::make_unique<Mesh>();
    mesh->create_cube(context, size, vec3(0.6f, 0.4f, 0.2f));  // Wood color
    return mesh;
}

std::unique_ptr<Mesh> PropMeshGenerator::generate_barrel(VulkanContext& context,
                                                         float radius, float height) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const int segments = 12;
    vec3 color(0.5f, 0.35f, 0.2f);  // Wood color

    // Barrel has wider middle
    auto barrel_radius = [radius](float y, float h) {
        float t = y / h;
        float bulge = 1.0f + 0.15f * std::sin(t * M_PI);
        return radius * bulge;
    };

    const int rings = 8;

    // Generate rings
    for (int r = 0; r <= rings; r++) {
        float y = (float(r) / rings) * height;
        float ring_radius = barrel_radius(y, height);

        for (int i = 0; i <= segments; i++) {
            float angle = (float(i) / segments) * 2.0f * M_PI;
            float x = std::cos(angle) * ring_radius;
            float z = std::sin(angle) * ring_radius;

            // Calculate normal (approximate)
            vec3 normal = normalize(vec3(x, 0.0f, z));

            Vertex v;
            v.position = vec3(x, y, z);
            v.normal = normal;
            v.color = color;
            v.uv = vec2(float(i) / segments, y / height);
            vertices.push_back(v);
        }
    }

    // Generate side indices
    for (int r = 0; r < rings; r++) {
        for (int i = 0; i < segments; i++) {
            uint32_t row0 = r * (segments + 1);
            uint32_t row1 = (r + 1) * (segments + 1);

            indices.push_back(row0 + i);
            indices.push_back(row0 + i + 1);
            indices.push_back(row1 + i + 1);
            indices.push_back(row0 + i);
            indices.push_back(row1 + i + 1);
            indices.push_back(row1 + i);
        }
    }

    // Top cap
    uint32_t center_top = static_cast<uint32_t>(vertices.size());
    Vertex vc_top;
    vc_top.position = vec3(0.0f, height, 0.0f);
    vc_top.normal = vec3(0.0f, 1.0f, 0.0f);
    vc_top.color = color * 0.8f;
    vc_top.uv = vec2(0.5f, 0.5f);
    vertices.push_back(vc_top);

    float top_radius = barrel_radius(height, height);
    for (int i = 0; i <= segments; i++) {
        float angle = (float(i) / segments) * 2.0f * M_PI;
        float x = std::cos(angle) * top_radius;
        float z = std::sin(angle) * top_radius;

        Vertex v;
        v.position = vec3(x, height, z);
        v.normal = vec3(0.0f, 1.0f, 0.0f);
        v.color = color * 0.8f;
        v.uv = vec2(0.5f + x / (2.0f * top_radius), 0.5f + z / (2.0f * top_radius));
        vertices.push_back(v);
    }

    for (int i = 0; i < segments; i++) {
        indices.push_back(center_top);
        indices.push_back(center_top + 1 + i);
        indices.push_back(center_top + 2 + i);
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->create(context, vertices, indices);
    return mesh;
}

} // namespace slam
