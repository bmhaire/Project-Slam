/**
 * Slam Engine - Shadow Map System
 *
 * Omnidirectional shadow maps using cubemap arrays for point lights
 */

#pragma once

#include "utils/math.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace slam {

class VulkanContext;

// Shadow map resolution
constexpr uint32_t SHADOW_MAP_SIZE = 512;
constexpr uint32_t MAX_SHADOW_CASTERS = 8;

class ShadowMapArray {
public:
    ShadowMapArray() = default;
    ~ShadowMapArray();

    // Non-copyable
    ShadowMapArray(const ShadowMapArray&) = delete;
    ShadowMapArray& operator=(const ShadowMapArray&) = delete;

    // Initialize shadow map array
    bool init(VulkanContext& context, uint32_t resolution = SHADOW_MAP_SIZE,
              uint32_t max_lights = MAX_SHADOW_CASTERS);

    // Cleanup
    void destroy();

    // Get view matrices for cubemap face
    static mat4 get_face_view(const vec3& light_pos, uint32_t face);
    static mat4 get_projection(float near_plane, float far_plane);

    // Getters
    VkRenderPass render_pass() const { return render_pass_; }
    VkFramebuffer framebuffer(uint32_t light_index, uint32_t face) const;
    VkImageView array_view() const { return array_view_; }
    VkSampler sampler() const { return sampler_; }
    uint32_t resolution() const { return resolution_; }
    uint32_t max_lights() const { return max_lights_; }

    VkDescriptorImageInfo descriptor_info() const;

private:
    bool create_cubemap_array();
    bool create_render_pass();
    bool create_framebuffers();
    bool create_sampler();

    VulkanContext* context_ = nullptr;
    uint32_t resolution_ = SHADOW_MAP_SIZE;
    uint32_t max_lights_ = MAX_SHADOW_CASTERS;

    // Cubemap array (6 faces * max_lights layers)
    VkImage cubemap_array_ = VK_NULL_HANDLE;
    VkDeviceMemory cubemap_memory_ = VK_NULL_HANDLE;
    VkImageView array_view_ = VK_NULL_HANDLE;

    // Per-face views for rendering (6 * max_lights)
    std::vector<VkImageView> face_views_;
    std::vector<VkFramebuffer> framebuffers_;

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

// Push constants for shadow pass
struct ShadowPushConstants {
    mat4 light_space_matrix;
    vec4 light_pos;  // xyz = position, w = far plane
};

} // namespace slam
