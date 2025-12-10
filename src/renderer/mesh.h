/**
 * Slam Engine - Mesh System
 *
 * Vertex buffer, index buffer, and mesh rendering
 */

#pragma once

#include "utils/math.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace slam {

class VulkanContext;

// Vertex structure matching pipeline layout
struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
    float _padding; // Align to 12 floats

    Vertex() : position(0), color(1), normal(0, 1, 0), uv(0), _padding(0) {}

    Vertex(const vec3& pos, const vec3& col, const vec3& norm, const vec2& tex)
        : position(pos), color(col), normal(norm), uv(tex), _padding(0) {}
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Non-copyable
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Create mesh from vertices and indices
    bool create(VulkanContext& context,
                const std::vector<Vertex>& vertices,
                const std::vector<uint32_t>& indices);

    // Cleanup
    void destroy();

    // Bind and draw
    void bind(VkCommandBuffer command_buffer);
    void draw(VkCommandBuffer command_buffer);

    // Draw with manual count (for instancing, etc.)
    void draw(VkCommandBuffer command_buffer, uint32_t index_count, uint32_t first_index = 0);

    // Getters
    uint32_t vertex_count() const { return vertex_count_; }
    uint32_t index_count() const { return index_count_; }

    // Static mesh generators
    static Mesh create_cube(VulkanContext& context, float size = 1.0f);
    static Mesh create_plane(VulkanContext& context, float size = 1.0f);

private:
    VulkanContext* context_ = nullptr;

    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
    VkBuffer index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory index_buffer_memory_ = VK_NULL_HANDLE;

    uint32_t vertex_count_ = 0;
    uint32_t index_count_ = 0;
};

} // namespace slam
