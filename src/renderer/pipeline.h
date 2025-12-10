/**
 * Slam Engine - Graphics Pipeline
 *
 * Basic Vulkan graphics pipeline for rendering
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace slam {

class VulkanContext;

// Push constants for per-object data
struct PushConstants {
    float model[16];      // Model matrix
    float view[16];       // View matrix
    float projection[16]; // Projection matrix
};

class Pipeline {
public:
    Pipeline();
    ~Pipeline();

    // Non-copyable
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    // Create pipeline
    bool create(VulkanContext& context,
                const std::string& vert_shader_path,
                const std::string& frag_shader_path);

    // Cleanup
    void destroy();

    // Bind pipeline for rendering
    void bind(VkCommandBuffer command_buffer);

    // Push MVP matrices
    void push_constants(VkCommandBuffer command_buffer, const PushConstants& constants);

    // Getters
    VkPipeline handle() const { return pipeline_; }
    VkPipelineLayout layout() const { return pipeline_layout_; }

private:
    // Load shader file
    std::vector<char> read_shader_file(const std::string& path);

    // Create shader module
    VkShaderModule create_shader_module(const std::vector<char>& code);

    VulkanContext* context_ = nullptr;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace slam
