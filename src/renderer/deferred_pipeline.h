/**
 * Slam Engine - Deferred Rendering Pipeline
 *
 * Two-pass deferred shading:
 * 1. Geometry pass: Render scene to G-buffer
 * 2. Lighting pass: Calculate lighting from G-buffer
 */

#pragma once

#include "gbuffer.h"
#include "light.h"
#include "shadow_map.h"
#include "utils/math.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace slam {

class VulkanContext;
class Mesh;

// Push constants for geometry pass
struct GeometryPushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
};

// Push constants for lighting pass
struct LightingPushConstants {
    mat4 inv_view_proj;
    vec4 camera_pos;
    vec4 screen_size;  // xy = size, z = near, w = far
};

class DeferredPipeline {
public:
    DeferredPipeline() = default;
    ~DeferredPipeline();

    // Non-copyable
    DeferredPipeline(const DeferredPipeline&) = delete;
    DeferredPipeline& operator=(const DeferredPipeline&) = delete;

    // Initialize pipeline
    bool init(VulkanContext& context, uint32_t width, uint32_t height);

    // Cleanup
    void destroy();

    // Recreate for window resize
    bool resize(uint32_t width, uint32_t height);

    // Begin geometry pass
    void begin_geometry_pass(VkCommandBuffer cmd);
    void end_geometry_pass(VkCommandBuffer cmd);

    // Begin lighting pass (renders to swapchain)
    void begin_lighting_pass(VkCommandBuffer cmd, VkFramebuffer target_framebuffer,
                            VkRenderPass target_render_pass, uint32_t width, uint32_t height);
    void end_lighting_pass(VkCommandBuffer cmd);

    // Set view/projection for geometry pass
    void set_view_projection(const mat4& view, const mat4& proj);

    // Draw mesh in geometry pass
    void draw_mesh(VkCommandBuffer cmd, const Mesh& mesh, const mat4& model);

    // Execute lighting pass (call after begin_lighting_pass)
    void render_lighting(VkCommandBuffer cmd, const vec3& camera_pos,
                        float near_plane, float far_plane);

    // Light management
    LightManager& lights() { return lights_; }
    const LightManager& lights() const { return lights_; }

    // Shadow rendering (call before geometry pass)
    void render_shadows(VkCommandBuffer cmd, const std::vector<Mesh*>& meshes,
                       const std::vector<mat4>& transforms);

    // Access components
    GBuffer& gbuffer() { return gbuffer_; }
    ShadowMapArray& shadows() { return shadows_; }

private:
    bool create_geometry_pipeline();
    bool create_lighting_pipeline();
    bool create_shadow_pipeline();
    bool create_descriptor_sets();
    bool update_descriptor_sets();

    VulkanContext* context_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Subsystems
    GBuffer gbuffer_;
    LightManager lights_;
    ShadowMapArray shadows_;

    // Geometry pass
    VkPipelineLayout geometry_layout_ = VK_NULL_HANDLE;
    VkPipeline geometry_pipeline_ = VK_NULL_HANDLE;

    // Lighting pass
    VkPipelineLayout lighting_layout_ = VK_NULL_HANDLE;
    VkPipeline lighting_pipeline_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout lighting_descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool lighting_descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet lighting_descriptor_set_ = VK_NULL_HANDLE;

    // Shadow pass
    VkPipelineLayout shadow_layout_ = VK_NULL_HANDLE;
    VkPipeline shadow_pipeline_ = VK_NULL_HANDLE;

    // Full-screen quad for lighting
    VkBuffer quad_vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory quad_vertex_memory_ = VK_NULL_HANDLE;

    // Current frame state
    mat4 view_matrix_;
    mat4 proj_matrix_;
};

} // namespace slam
