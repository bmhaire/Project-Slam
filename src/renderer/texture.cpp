/**
 * Slam Engine - Texture System Implementation
 */

#include "texture.h"
#include "vulkan_context.h"
#include <fstream>
#include <cstring>
#include <cstdio>

namespace slam {

Texture::Texture() = default;

Texture::~Texture() {
    destroy();
}

Texture::Texture(Texture&& other) noexcept
    : context_(other.context_)
    , image_(other.image_)
    , image_memory_(other.image_memory_)
    , image_view_(other.image_view_)
    , sampler_(other.sampler_)
    , width_(other.width_)
    , height_(other.height_) {
    other.context_ = nullptr;
    other.image_ = VK_NULL_HANDLE;
    other.image_memory_ = VK_NULL_HANDLE;
    other.image_view_ = VK_NULL_HANDLE;
    other.sampler_ = VK_NULL_HANDLE;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        destroy();
        context_ = other.context_;
        image_ = other.image_;
        image_memory_ = other.image_memory_;
        image_view_ = other.image_view_;
        sampler_ = other.sampler_;
        width_ = other.width_;
        height_ = other.height_;
        other.context_ = nullptr;
        other.image_ = VK_NULL_HANDLE;
        other.image_memory_ = VK_NULL_HANDLE;
        other.image_view_ = VK_NULL_HANDLE;
        other.sampler_ = VK_NULL_HANDLE;
    }
    return *this;
}

void Texture::destroy() {
    if (context_ && context_->device()) {
        if (sampler_) {
            vkDestroySampler(context_->device(), sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (image_view_) {
            vkDestroyImageView(context_->device(), image_view_, nullptr);
            image_view_ = VK_NULL_HANDLE;
        }
        if (image_) {
            vkDestroyImage(context_->device(), image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
        if (image_memory_) {
            vkFreeMemory(context_->device(), image_memory_, nullptr);
            image_memory_ = VK_NULL_HANDLE;
        }
    }
    context_ = nullptr;
}

bool Texture::load_tga(VulkanContext& context, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open texture file: %s\n", path.c_str());
        return false;
    }

    // Read TGA header
    uint8_t header[18];
    file.read(reinterpret_cast<char*>(header), 18);

    int id_length = header[0];
    int color_map_type = header[1];
    int image_type = header[2];
    int width = header[12] | (header[13] << 8);
    int height = header[14] | (header[15] << 8);
    int bits_per_pixel = header[16];
    int descriptor = header[17];

    // Skip ID field
    if (id_length > 0) {
        file.seekg(id_length, std::ios::cur);
    }

    // Only support uncompressed RGB/RGBA
    if (color_map_type != 0 || (image_type != 2 && image_type != 3)) {
        fprintf(stderr, "Unsupported TGA format in: %s\n", path.c_str());
        return false;
    }

    bool has_alpha = (bits_per_pixel == 32);
    int bytes_per_pixel = bits_per_pixel / 8;

    // Read pixel data
    std::vector<uint8_t> raw_data(width * height * bytes_per_pixel);
    file.read(reinterpret_cast<char*>(raw_data.data()), raw_data.size());

    // Convert to RGBA
    std::vector<uint8_t> rgba_data(width * height * 4);

    bool bottom_up = !(descriptor & 0x20);

    for (int y = 0; y < height; y++) {
        int src_y = bottom_up ? (height - 1 - y) : y;
        for (int x = 0; x < width; x++) {
            int src_idx = (src_y * width + x) * bytes_per_pixel;
            int dst_idx = (y * width + x) * 4;

            // TGA stores BGRA
            rgba_data[dst_idx + 0] = raw_data[src_idx + 2]; // R
            rgba_data[dst_idx + 1] = raw_data[src_idx + 1]; // G
            rgba_data[dst_idx + 2] = raw_data[src_idx + 0]; // B
            rgba_data[dst_idx + 3] = has_alpha ? raw_data[src_idx + 3] : 255; // A
        }
    }

    return load_rgba(context, rgba_data.data(), width, height);
}

bool Texture::load_rgba(VulkanContext& context, const uint8_t* data, int width, int height) {
    context_ = &context;
    width_ = width;
    height_ = height;

    return create_image(context, width, height, data);
}

bool Texture::create_solid(VulkanContext& context, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint8_t pixel[4] = {r, g, b, a};
    return load_rgba(context, pixel, 1, 1);
}

bool Texture::create_image(VulkanContext& context, int width, int height, const uint8_t* data) {
    VkDeviceSize image_size = width * height * 4;

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    context.create_buffer(image_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_memory);

    // Copy data to staging buffer
    void* mapped;
    vkMapMemory(context.device(), staging_memory, 0, image_size, 0, &mapped);
    memcpy(mapped, data, image_size);
    vkUnmapMemory(context.device(), staging_memory);

    // Create image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(context.device(), &image_info, nullptr, &image_) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture image\n");
        return false;
    }

    // Allocate memory
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context.device(), image_, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = context.find_memory_type(
        mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(context.device(), &alloc_info, nullptr, &image_memory_) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate texture memory\n");
        return false;
    }

    vkBindImageMemory(context.device(), image_, image_memory_, 0);

    // Transition to transfer destination
    transition_image_layout(context, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy buffer to image
    VkCommandBuffer cmd = context.begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

    vkCmdCopyBufferToImage(cmd, staging_buffer, image_,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    context.end_single_time_commands(cmd);

    // Transition to shader read
    transition_image_layout(context, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup staging buffer
    vkDestroyBuffer(context.device(), staging_buffer, nullptr);
    vkFreeMemory(context.device(), staging_memory, nullptr);

    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context.device(), &view_info, nullptr, &image_view_) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture image view\n");
        return false;
    }

    // Create sampler
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(context.device(), &sampler_info, nullptr, &sampler_) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture sampler\n");
        return false;
    }

    return true;
}

void Texture::transition_image_layout(VulkanContext& context, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkCommandBuffer cmd = context.begin_single_time_commands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage, dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    context.end_single_time_commands(cmd);
}

VkDescriptorImageInfo Texture::descriptor_info() const {
    VkDescriptorImageInfo info{};
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.imageView = image_view_;
    info.sampler = sampler_;
    return info;
}

// ============================================================================
// Material
// ============================================================================

bool Material::load(VulkanContext& context, const std::string& directory, const std::string& material_name) {
    std::string base_path = directory + "/" + material_name;

    bool success = true;

    if (!albedo.load_tga(context, base_path + "_albedo.tga")) {
        fprintf(stderr, "Failed to load albedo texture for %s\n", material_name.c_str());
        success = false;
    }

    if (!normal.load_tga(context, base_path + "_normal.tga")) {
        fprintf(stderr, "Warning: No normal map for %s\n", material_name.c_str());
        // Create default flat normal map
        normal.create_solid(context, 128, 128, 255);
    }

    if (!roughness.load_tga(context, base_path + "_roughness.tga")) {
        fprintf(stderr, "Warning: No roughness map for %s\n", material_name.c_str());
        roughness.create_solid(context, 128, 128, 128); // 0.5 roughness
    }

    if (!metallic.load_tga(context, base_path + "_metallic.tga")) {
        fprintf(stderr, "Warning: No metallic map for %s\n", material_name.c_str());
        metallic.create_solid(context, 0, 0, 0); // Non-metallic
    }

    if (!ao.load_tga(context, base_path + "_ao.tga")) {
        fprintf(stderr, "Warning: No AO map for %s\n", material_name.c_str());
        ao.create_solid(context, 255, 255, 255); // No occlusion
    }

    return success;
}

void Material::destroy() {
    albedo.destroy();
    normal.destroy();
    roughness.destroy();
    metallic.destroy();
    ao.destroy();
}

} // namespace slam
