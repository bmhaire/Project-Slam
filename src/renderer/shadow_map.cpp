/**
 * Slam Engine - Shadow Map Implementation
 */

#include "shadow_map.h"
#include "vulkan_context.h"
#include <cstdio>
#include <cmath>

namespace slam {

ShadowMapArray::~ShadowMapArray() {
    destroy();
}

bool ShadowMapArray::init(VulkanContext& context, uint32_t resolution, uint32_t max_lights) {
    context_ = &context;
    resolution_ = resolution;
    max_lights_ = max_lights;

    if (!create_cubemap_array()) {
        fprintf(stderr, "Failed to create shadow cubemap array\n");
        return false;
    }

    if (!create_sampler()) {
        fprintf(stderr, "Failed to create shadow sampler\n");
        return false;
    }

    if (!create_render_pass()) {
        fprintf(stderr, "Failed to create shadow render pass\n");
        return false;
    }

    if (!create_framebuffers()) {
        fprintf(stderr, "Failed to create shadow framebuffers\n");
        return false;
    }

    return true;
}

void ShadowMapArray::destroy() {
    if (!context_ || !context_->device()) return;

    VkDevice device = context_->device();

    for (auto fb : framebuffers_) {
        if (fb) vkDestroyFramebuffer(device, fb, nullptr);
    }
    framebuffers_.clear();

    for (auto view : face_views_) {
        if (view) vkDestroyImageView(device, view, nullptr);
    }
    face_views_.clear();

    if (render_pass_) {
        vkDestroyRenderPass(device, render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }

    if (sampler_) {
        vkDestroySampler(device, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }

    if (array_view_) {
        vkDestroyImageView(device, array_view_, nullptr);
        array_view_ = VK_NULL_HANDLE;
    }

    if (cubemap_array_) {
        vkDestroyImage(device, cubemap_array_, nullptr);
        cubemap_array_ = VK_NULL_HANDLE;
    }

    if (cubemap_memory_) {
        vkFreeMemory(device, cubemap_memory_, nullptr);
        cubemap_memory_ = VK_NULL_HANDLE;
    }

    context_ = nullptr;
}

bool ShadowMapArray::create_cubemap_array() {
    // Create cubemap array image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = resolution_;
    image_info.extent.height = resolution_;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6 * max_lights_;  // 6 faces per light
    image_info.format = VK_FORMAT_D32_SFLOAT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(context_->device(), &image_info, nullptr, &cubemap_array_) != VK_SUCCESS) {
        return false;
    }

    // Allocate memory
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(context_->device(), cubemap_array_, &mem_req);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = context_->find_memory_type(
        mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(context_->device(), &alloc_info, nullptr, &cubemap_memory_) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(context_->device(), cubemap_array_, cubemap_memory_, 0);

    // Create array view for sampling
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = cubemap_array_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    view_info.format = VK_FORMAT_D32_SFLOAT;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 6 * max_lights_;

    if (vkCreateImageView(context_->device(), &view_info, nullptr, &array_view_) != VK_SUCCESS) {
        return false;
    }

    // Create per-face views for rendering
    face_views_.resize(6 * max_lights_);
    for (uint32_t i = 0; i < 6 * max_lights_; i++) {
        VkImageViewCreateInfo face_view_info{};
        face_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        face_view_info.image = cubemap_array_;
        face_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        face_view_info.format = VK_FORMAT_D32_SFLOAT;
        face_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        face_view_info.subresourceRange.baseMipLevel = 0;
        face_view_info.subresourceRange.levelCount = 1;
        face_view_info.subresourceRange.baseArrayLayer = i;
        face_view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context_->device(), &face_view_info, nullptr, &face_views_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool ShadowMapArray::create_render_pass() {
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_ref{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &depth_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    return vkCreateRenderPass(context_->device(), &render_pass_info, nullptr, &render_pass_) == VK_SUCCESS;
}

bool ShadowMapArray::create_framebuffers() {
    framebuffers_.resize(6 * max_lights_);

    for (uint32_t i = 0; i < 6 * max_lights_; i++) {
        VkFramebufferCreateInfo fb_info{};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass_;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &face_views_[i];
        fb_info.width = resolution_;
        fb_info.height = resolution_;
        fb_info.layers = 1;

        if (vkCreateFramebuffer(context_->device(), &fb_info, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool ShadowMapArray::create_sampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    // Disable comparison sampler for MoltenVK compatibility
    // Shadow comparison will be done manually in the shader
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    return vkCreateSampler(context_->device(), &sampler_info, nullptr, &sampler_) == VK_SUCCESS;
}

VkFramebuffer ShadowMapArray::framebuffer(uint32_t light_index, uint32_t face) const {
    uint32_t idx = light_index * 6 + face;
    if (idx < framebuffers_.size()) {
        return framebuffers_[idx];
    }
    return VK_NULL_HANDLE;
}

mat4 ShadowMapArray::get_face_view(const vec3& light_pos, uint32_t face) {
    // Cubemap face directions
    static const vec3 targets[] = {
        vec3(1.0f, 0.0f, 0.0f),   // +X
        vec3(-1.0f, 0.0f, 0.0f),  // -X
        vec3(0.0f, 1.0f, 0.0f),   // +Y
        vec3(0.0f, -1.0f, 0.0f),  // -Y
        vec3(0.0f, 0.0f, 1.0f),   // +Z
        vec3(0.0f, 0.0f, -1.0f)   // -Z
    };

    static const vec3 ups[] = {
        vec3(0.0f, -1.0f, 0.0f),  // +X
        vec3(0.0f, -1.0f, 0.0f),  // -X
        vec3(0.0f, 0.0f, 1.0f),   // +Y
        vec3(0.0f, 0.0f, -1.0f),  // -Y
        vec3(0.0f, -1.0f, 0.0f),  // +Z
        vec3(0.0f, -1.0f, 0.0f)   // -Z
    };

    return look_at(light_pos, light_pos + targets[face], ups[face]);
}

mat4 ShadowMapArray::get_projection(float near_plane, float far_plane) {
    // 90 degree FOV for cubemap faces
    return perspective(M_PI / 2.0f, 1.0f, near_plane, far_plane);
}

VkDescriptorImageInfo ShadowMapArray::descriptor_info() const {
    return {sampler_, array_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

} // namespace slam
