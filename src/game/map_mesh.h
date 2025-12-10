/**
 * Slam Engine - Map Mesh Generator
 *
 * Converts 2D map data to 3D mesh using marching squares
 */

#pragma once

#include "map_generator.h"
#include "renderer/mesh.h"
#include <memory>
#include <vector>

namespace slam {

class VulkanContext;

// Configuration for map mesh generation
struct MapMeshConfig {
    float wall_height = 4.0f;       // Height of walls
    float floor_height = 0.0f;      // Y position of floor
    float ceiling_height = 4.0f;    // Y position of ceiling (if any)
    float uv_scale = 0.25f;         // Texture coordinate scale
    bool generate_ceiling = false;   // Generate ceiling geometry
    bool smooth_walls = true;        // Use marching squares for smoother walls
};

class MapMesh {
public:
    MapMesh() = default;
    ~MapMesh() = default;

    // Generate mesh from map
    bool generate(VulkanContext& context, const MapGenerator& map,
                 const MapMeshConfig& config = MapMeshConfig());

    // Cleanup
    void destroy();

    // Access meshes
    Mesh& floor_mesh() { return floor_mesh_; }
    Mesh& wall_mesh() { return wall_mesh_; }
    Mesh& ceiling_mesh() { return ceiling_mesh_; }

    const Mesh& floor_mesh() const { return floor_mesh_; }
    const Mesh& wall_mesh() const { return wall_mesh_; }
    const Mesh& ceiling_mesh() const { return ceiling_mesh_; }

    // Check if meshes are valid
    bool has_floor() const { return floor_mesh_.vertex_count() > 0; }
    bool has_walls() const { return wall_mesh_.vertex_count() > 0; }
    bool has_ceiling() const { return ceiling_mesh_.vertex_count() > 0; }

private:
    void generate_floor(const MapGenerator& map, const MapMeshConfig& config,
                       std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    void generate_walls_simple(const MapGenerator& map, const MapMeshConfig& config,
                              std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    void generate_walls_marching(const MapGenerator& map, const MapMeshConfig& config,
                                std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    void generate_ceiling(const MapGenerator& map, const MapMeshConfig& config,
                         std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    // Helper to add a quad
    void add_quad(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                  const vec3& p0, const vec3& p1, const vec3& p2, const vec3& p3,
                  const vec3& normal, const vec3& color,
                  float u_scale, float v_scale);

    // Helper to add a wall segment
    void add_wall_segment(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                         const vec3& start, const vec3& end, float height, float floor_y,
                         const vec3& color, float uv_scale);

    Mesh floor_mesh_;
    Mesh wall_mesh_;
    Mesh ceiling_mesh_;
};

// Prop mesh generator
class PropMeshGenerator {
public:
    // Generate different prop types
    static std::unique_ptr<Mesh> generate_column(VulkanContext& context, float radius, float height);
    static std::unique_ptr<Mesh> generate_crate(VulkanContext& context, float size);
    static std::unique_ptr<Mesh> generate_barrel(VulkanContext& context, float radius, float height);
};

} // namespace slam
