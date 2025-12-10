/**
 * Slam Engine - G-Buffer System
 *
 * Multiple render targets for deferred shading:
 * - Position (RGB16F)
 * - Normal (RGB16F)
 * - Albedo (RGBA8)
 * - Material (RG8: roughness, metallic)
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace slam {

class VulkanContext;

struct GBufferAttachment {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
};

class GBuffer {
public:
    GBuffer() = default;
    ~GBuffer();

    // Non-copyable
    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    // Initialize G-buffer with given dimensions
    bool init(VulkanContext& context, uint32_t width, uint32_t height);

    // Cleanup
    void destroy();

    // Recreate for window resize
    bool resize(uint32_t width, uint32_t height);

    // Getters
    VkRenderPass render_pass() const { return render_pass_; }
    VkFramebuffer framebuffer() const { return framebuffer_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    // Attachment access
    const GBufferAttachment& position() const { return position_; }
    const GBufferAttachment& normal() const { return normal_; }
    const GBufferAttachment& albedo() const { return albedo_; }
    const GBufferAttachment& material() const { return material_; }
    const GBufferAttachment& depth() const { return depth_; }

    // Descriptor info for lighting pass
    VkDescriptorImageInfo position_descriptor() const;
    VkDescriptorImageInfo normal_descriptor() const;
    VkDescriptorImageInfo albedo_descriptor() const;
    VkDescriptorImageInfo material_descriptor() const;
    VkDescriptorImageInfo depth_descriptor() const;

    // Sampler for all attachments
    VkSampler sampler() const { return sampler_; }

private:
    bool create_attachment(GBufferAttachment& attachment, VkFormat format,
                          VkImageUsageFlags usage, VkImageAspectFlags aspect);
    void destroy_attachment(GBufferAttachment& attachment);
    bool create_render_pass();
    bool create_framebuffer();
    bool create_sampler();

    VulkanContext* context_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // Attachments
    GBufferAttachment position_;   // RGB16F - world position
    GBufferAttachment normal_;     // RGB16F - world normal
    GBufferAttachment albedo_;     // RGBA8 - base color + AO
    GBufferAttachment material_;   // RG8 - roughness, metallic
    GBufferAttachment depth_;      // D32F - depth buffer

    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

} // namespace slam
