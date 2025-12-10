/**
 * Slam Engine - Light System
 *
 * Point lights with shadows for deferred rendering
 */

#pragma once

#include "utils/math.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace slam {

// Maximum lights per cluster
constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 32;
constexpr uint32_t MAX_POINT_LIGHTS = 256;

// Cluster grid dimensions
constexpr uint32_t CLUSTER_X = 16;
constexpr uint32_t CLUSTER_Y = 9;
constexpr uint32_t CLUSTER_Z = 24;
constexpr uint32_t TOTAL_CLUSTERS = CLUSTER_X * CLUSTER_Y * CLUSTER_Z;

// Point light data (GPU layout)
struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float intensity;

    PointLight() : position(0), radius(10.0f), color(1.0f), intensity(1.0f) {}
    PointLight(const vec3& pos, float r, const vec3& col, float i)
        : position(pos), radius(r), color(col), intensity(i) {}
};

// Light cluster (for clustered culling)
struct LightCluster {
    uint32_t offset;
    uint32_t count;
};

// Uniform buffer for lights
struct LightUniforms {
    vec4 camera_position;   // xyz = position, w = unused
    vec4 ambient_color;     // xyz = color, w = intensity
    uint32_t num_lights;
    uint32_t pad[3];
};

class VulkanContext;

class LightManager {
public:
    LightManager() = default;
    ~LightManager();

    // Non-copyable
    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    // Initialize buffers
    bool init(VulkanContext& context);

    // Cleanup
    void destroy();

    // Light management
    int add_light(const PointLight& light);
    void remove_light(int index);
    void clear_lights();

    // Update light data
    void set_light_position(int index, const vec3& position);
    void set_light_color(int index, const vec3& color);
    void set_light_intensity(int index, float intensity);
    void set_light_radius(int index, float radius);

    // Ambient light
    void set_ambient(const vec3& color, float intensity);

    // Upload to GPU
    void upload(const vec3& camera_pos);

    // Perform clustered culling (updates cluster buffers)
    void update_clusters(const mat4& view, const mat4& projection,
                        float near_plane, float far_plane);

    // Getters
    const std::vector<PointLight>& lights() const { return lights_; }
    int light_count() const { return static_cast<int>(lights_.size()); }

    VkBuffer light_buffer() const { return light_buffer_; }
    VkBuffer uniform_buffer() const { return uniform_buffer_; }
    VkBuffer cluster_buffer() const { return cluster_buffer_; }
    VkBuffer light_index_buffer() const { return light_index_buffer_; }

    VkDescriptorBufferInfo light_buffer_info() const;
    VkDescriptorBufferInfo uniform_buffer_info() const;
    VkDescriptorBufferInfo cluster_buffer_info() const;
    VkDescriptorBufferInfo light_index_buffer_info() const;

private:
    bool create_buffers();

    VulkanContext* context_ = nullptr;

    // Light data
    std::vector<PointLight> lights_;
    vec3 ambient_color_ = vec3(0.03f);
    float ambient_intensity_ = 1.0f;
    bool lights_dirty_ = false;  // Track if light data needs uploading

    // GPU buffers
    VkBuffer light_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory light_memory_ = VK_NULL_HANDLE;

    VkBuffer uniform_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniform_memory_ = VK_NULL_HANDLE;

    VkBuffer cluster_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory cluster_memory_ = VK_NULL_HANDLE;

    VkBuffer light_index_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory light_index_memory_ = VK_NULL_HANDLE;

    // Mapped pointers
    PointLight* mapped_lights_ = nullptr;
    LightUniforms* mapped_uniforms_ = nullptr;
    LightCluster* mapped_clusters_ = nullptr;
    uint32_t* mapped_light_indices_ = nullptr;
};

} // namespace slam
