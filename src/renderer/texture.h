/**
 * Slam Engine - Texture System
 *
 * Vulkan texture loading and management
 */

#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace slam {

class VulkanContext;

class Texture {
public:
    Texture();
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Move semantics
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Load texture from TGA file
    bool load_tga(VulkanContext& context, const std::string& path);

    // Load texture from raw RGBA data
    bool load_rgba(VulkanContext& context, const uint8_t* data, int width, int height);

    // Create solid color texture (for defaults)
    bool create_solid(VulkanContext& context, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Cleanup
    void destroy();

    // Getters
    VkImage image() const { return image_; }
    VkImageView image_view() const { return image_view_; }
    VkSampler sampler() const { return sampler_; }
    int width() const { return width_; }
    int height() const { return height_; }

    // Descriptor info for shader binding
    VkDescriptorImageInfo descriptor_info() const;

private:
    bool create_image(VulkanContext& context, int width, int height, const uint8_t* data);
    void transition_image_layout(VulkanContext& context, VkImageLayout old_layout, VkImageLayout new_layout);

    VulkanContext* context_ = nullptr;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory image_memory_ = VK_NULL_HANDLE;
    VkImageView image_view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    int width_ = 0;
    int height_ = 0;
};

// PBR Material - collection of textures
struct Material {
    Texture albedo;
    Texture normal;
    Texture roughness;
    Texture metallic;
    Texture ao;

    // Load all textures for a material from directory
    bool load(VulkanContext& context, const std::string& directory, const std::string& material_name);

    void destroy();
};

} // namespace slam
