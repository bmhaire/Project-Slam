/**
 * Slam Engine - Light System Implementation
 */

#include "light.h"
#include "vulkan_context.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace slam {

LightManager::~LightManager() {
    destroy();
}

bool LightManager::init(VulkanContext& context) {
    context_ = &context;
    return create_buffers();
}

void LightManager::destroy() {
    if (!context_ || !context_->device()) return;

    VkDevice device = context_->device();

    // Unmap and destroy light buffer
    if (mapped_lights_) {
        vkUnmapMemory(device, light_memory_);
        mapped_lights_ = nullptr;
    }
    if (light_buffer_) {
        vkDestroyBuffer(device, light_buffer_, nullptr);
        light_buffer_ = VK_NULL_HANDLE;
    }
    if (light_memory_) {
        vkFreeMemory(device, light_memory_, nullptr);
        light_memory_ = VK_NULL_HANDLE;
    }

    // Unmap and destroy uniform buffer
    if (mapped_uniforms_) {
        vkUnmapMemory(device, uniform_memory_);
        mapped_uniforms_ = nullptr;
    }
    if (uniform_buffer_) {
        vkDestroyBuffer(device, uniform_buffer_, nullptr);
        uniform_buffer_ = VK_NULL_HANDLE;
    }
    if (uniform_memory_) {
        vkFreeMemory(device, uniform_memory_, nullptr);
        uniform_memory_ = VK_NULL_HANDLE;
    }

    // Unmap and destroy cluster buffer
    if (mapped_clusters_) {
        vkUnmapMemory(device, cluster_memory_);
        mapped_clusters_ = nullptr;
    }
    if (cluster_buffer_) {
        vkDestroyBuffer(device, cluster_buffer_, nullptr);
        cluster_buffer_ = VK_NULL_HANDLE;
    }
    if (cluster_memory_) {
        vkFreeMemory(device, cluster_memory_, nullptr);
        cluster_memory_ = VK_NULL_HANDLE;
    }

    // Unmap and destroy light index buffer
    if (mapped_light_indices_) {
        vkUnmapMemory(device, light_index_memory_);
        mapped_light_indices_ = nullptr;
    }
    if (light_index_buffer_) {
        vkDestroyBuffer(device, light_index_buffer_, nullptr);
        light_index_buffer_ = VK_NULL_HANDLE;
    }
    if (light_index_memory_) {
        vkFreeMemory(device, light_index_memory_, nullptr);
        light_index_memory_ = VK_NULL_HANDLE;
    }

    lights_.clear();
    context_ = nullptr;
}

bool LightManager::create_buffers() {
    VkMemoryPropertyFlags memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Light buffer
    VkDeviceSize light_size = sizeof(PointLight) * MAX_POINT_LIGHTS;
    context_->create_buffer(light_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        memory_props, light_buffer_, light_memory_);

    vkMapMemory(context_->device(), light_memory_, 0, light_size, 0,
                reinterpret_cast<void**>(&mapped_lights_));

    // Uniform buffer
    VkDeviceSize uniform_size = sizeof(LightUniforms);
    context_->create_buffer(uniform_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        memory_props, uniform_buffer_, uniform_memory_);

    vkMapMemory(context_->device(), uniform_memory_, 0, uniform_size, 0,
                reinterpret_cast<void**>(&mapped_uniforms_));

    // Cluster buffer
    VkDeviceSize cluster_size = sizeof(LightCluster) * TOTAL_CLUSTERS;
    context_->create_buffer(cluster_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        memory_props, cluster_buffer_, cluster_memory_);

    vkMapMemory(context_->device(), cluster_memory_, 0, cluster_size, 0,
                reinterpret_cast<void**>(&mapped_clusters_));

    // Light index buffer (stores light indices per cluster)
    VkDeviceSize index_size = sizeof(uint32_t) * TOTAL_CLUSTERS * MAX_LIGHTS_PER_CLUSTER;
    context_->create_buffer(index_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        memory_props, light_index_buffer_, light_index_memory_);

    vkMapMemory(context_->device(), light_index_memory_, 0, index_size, 0,
                reinterpret_cast<void**>(&mapped_light_indices_));

    return true;
}

int LightManager::add_light(const PointLight& light) {
    if (lights_.size() >= MAX_POINT_LIGHTS) {
        fprintf(stderr, "LightManager: Maximum light count (%u) reached\n", MAX_POINT_LIGHTS);
        return -1;
    }
    lights_.push_back(light);
    lights_dirty_ = true;
    return static_cast<int>(lights_.size() - 1);
}

void LightManager::remove_light(int index) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_.erase(lights_.begin() + index);
        lights_dirty_ = true;
    } else {
        fprintf(stderr, "LightManager: remove_light() index %d out of bounds (size: %zu)\n",
                index, lights_.size());
    }
}

void LightManager::clear_lights() {
    lights_.clear();
    lights_dirty_ = true;
}

void LightManager::set_light_position(int index, const vec3& position) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index].position = position;
        lights_dirty_ = true;
    } else {
        fprintf(stderr, "LightManager: set_light_position() index %d out of bounds (size: %zu)\n",
                index, lights_.size());
    }
}

void LightManager::set_light_color(int index, const vec3& color) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index].color = color;
        lights_dirty_ = true;
    } else {
        fprintf(stderr, "LightManager: set_light_color() index %d out of bounds (size: %zu)\n",
                index, lights_.size());
    }
}

void LightManager::set_light_intensity(int index, float intensity) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index].intensity = intensity;
        lights_dirty_ = true;
    } else {
        fprintf(stderr, "LightManager: set_light_intensity() index %d out of bounds (size: %zu)\n",
                index, lights_.size());
    }
}

void LightManager::set_light_radius(int index, float radius) {
    if (index >= 0 && index < static_cast<int>(lights_.size())) {
        lights_[index].radius = radius;
        lights_dirty_ = true;
    } else {
        fprintf(stderr, "LightManager: set_light_radius() index %d out of bounds (size: %zu)\n",
                index, lights_.size());
    }
}

void LightManager::set_ambient(const vec3& color, float intensity) {
    ambient_color_ = color;
    ambient_intensity_ = intensity;
    // Ambient changes are always uploaded in upload()
}

void LightManager::upload(const vec3& camera_pos) {
    // Copy lights to GPU only if dirty (reduces unnecessary memory copies)
    if (lights_dirty_ && !lights_.empty() && mapped_lights_) {
        memcpy(mapped_lights_, lights_.data(), sizeof(PointLight) * lights_.size());
        lights_dirty_ = false;
    }

    // Update uniforms (always update camera position and light count)
    if (mapped_uniforms_) {
        mapped_uniforms_->camera_position = vec4(camera_pos.x, camera_pos.y, camera_pos.z, 0.0f);
        mapped_uniforms_->ambient_color = vec4(
            ambient_color_.x * ambient_intensity_,
            ambient_color_.y * ambient_intensity_,
            ambient_color_.z * ambient_intensity_,
            ambient_intensity_
        );
        mapped_uniforms_->num_lights = static_cast<uint32_t>(lights_.size());
    }
}

void LightManager::update_clusters(const mat4& view, const mat4& projection,
                                   float near_plane, float far_plane) {
    if (!mapped_clusters_ || !mapped_light_indices_) return;

    // Clear clusters
    memset(mapped_clusters_, 0, sizeof(LightCluster) * TOTAL_CLUSTERS);

    uint32_t total_indices = 0;

    // For each cluster, find intersecting lights
    for (uint32_t z = 0; z < CLUSTER_Z; z++) {
        for (uint32_t y = 0; y < CLUSTER_Y; y++) {
            for (uint32_t x = 0; x < CLUSTER_X; x++) {
                uint32_t cluster_idx = x + y * CLUSTER_X + z * CLUSTER_X * CLUSTER_Y;

                // Calculate cluster bounds in view space
                // Using exponential depth slicing for better distribution
                float z_near = near_plane * std::pow(far_plane / near_plane, float(z) / CLUSTER_Z);
                float z_far = near_plane * std::pow(far_plane / near_plane, float(z + 1) / CLUSTER_Z);

                // Cluster bounds in NDC
                float ndc_x_min = (float(x) / CLUSTER_X) * 2.0f - 1.0f;
                float ndc_x_max = (float(x + 1) / CLUSTER_X) * 2.0f - 1.0f;
                float ndc_y_min = (float(y) / CLUSTER_Y) * 2.0f - 1.0f;
                float ndc_y_max = (float(y + 1) / CLUSTER_Y) * 2.0f - 1.0f;

                // Simple AABB in view space for culling
                // Calculate view-space corners of cluster frustum
                // Note: projection[1][1] is negative in Vulkan, so use abs
                float aspect = std::abs(projection.data()[5] / projection.data()[0]);
                float tan_half_fov = 1.0f / std::abs(projection.data()[5]);

                // Calculate frustum bounds at both near and far depth slices
                // At each depth, compute the x/y extent based on NDC coords
                float x_min_near = ndc_x_min * z_near * tan_half_fov * aspect;
                float x_max_near = ndc_x_max * z_near * tan_half_fov * aspect;
                float x_min_far = ndc_x_min * z_far * tan_half_fov * aspect;
                float x_max_far = ndc_x_max * z_far * tan_half_fov * aspect;

                float y_min_near = ndc_y_min * z_near * tan_half_fov;
                float y_max_near = ndc_y_max * z_near * tan_half_fov;
                float y_min_far = ndc_y_min * z_far * tan_half_fov;
                float y_max_far = ndc_y_max * z_far * tan_half_fov;

                // AABB encompasses the entire cluster frustum
                vec3 cluster_min(
                    std::min({x_min_near, x_max_near, x_min_far, x_max_far}),
                    std::min({y_min_near, y_max_near, y_min_far, y_max_far}),
                    -z_far  // View space Z is negative
                );
                vec3 cluster_max(
                    std::max({x_min_near, x_max_near, x_min_far, x_max_far}),
                    std::max({y_min_near, y_max_near, y_min_far, y_max_far}),
                    -z_near  // View space Z is negative
                );

                // Set cluster offset
                mapped_clusters_[cluster_idx].offset = total_indices;
                uint32_t count = 0;

                // Test each light against cluster bounds
                for (size_t i = 0; i < lights_.size() && count < MAX_LIGHTS_PER_CLUSTER; i++) {
                    // Transform light to view space
                    vec4 light_pos_view = view * vec4(lights_[i].position, 1.0f);
                    vec3 light_view(light_pos_view.x, light_pos_view.y, light_pos_view.z);
                    float radius = lights_[i].radius;

                    // Sphere-AABB intersection test
                    float dist_sq = 0.0f;

                    if (light_view.x < cluster_min.x)
                        dist_sq += (cluster_min.x - light_view.x) * (cluster_min.x - light_view.x);
                    else if (light_view.x > cluster_max.x)
                        dist_sq += (light_view.x - cluster_max.x) * (light_view.x - cluster_max.x);

                    if (light_view.y < cluster_min.y)
                        dist_sq += (cluster_min.y - light_view.y) * (cluster_min.y - light_view.y);
                    else if (light_view.y > cluster_max.y)
                        dist_sq += (light_view.y - cluster_max.y) * (light_view.y - cluster_max.y);

                    if (light_view.z < cluster_min.z)
                        dist_sq += (cluster_min.z - light_view.z) * (cluster_min.z - light_view.z);
                    else if (light_view.z > cluster_max.z)
                        dist_sq += (light_view.z - cluster_max.z) * (light_view.z - cluster_max.z);

                    if (dist_sq <= radius * radius) {
                        // Light intersects cluster
                        mapped_light_indices_[total_indices + count] = static_cast<uint32_t>(i);
                        count++;
                    }
                }

                mapped_clusters_[cluster_idx].count = count;
                total_indices += count;
            }
        }
    }
}

VkDescriptorBufferInfo LightManager::light_buffer_info() const {
    return {light_buffer_, 0, sizeof(PointLight) * MAX_POINT_LIGHTS};
}

VkDescriptorBufferInfo LightManager::uniform_buffer_info() const {
    return {uniform_buffer_, 0, sizeof(LightUniforms)};
}

VkDescriptorBufferInfo LightManager::cluster_buffer_info() const {
    return {cluster_buffer_, 0, sizeof(LightCluster) * TOTAL_CLUSTERS};
}

VkDescriptorBufferInfo LightManager::light_index_buffer_info() const {
    return {light_index_buffer_, 0, sizeof(uint32_t) * TOTAL_CLUSTERS * MAX_LIGHTS_PER_CLUSTER};
}

} // namespace slam
