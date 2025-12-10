/**
 * Slam Engine - G-Buffer Implementation
 */

#include "gbuffer.h"
#include "vulkan_context.h"
#include <array>
#include <cstdio>

namespace slam {

GBuffer::~GBuffer() {
    destroy();
}

bool GBuffer::init(VulkanContext& context, uint32_t width, uint32_t height) {
    context_ = &context;
    width_ = width;
    height_ = height;

    // Create attachments
    if (!create_attachment(position_, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) {
        fprintf(stderr, "Failed to create position attachment\n");
        return false;
    }

    if (!create_attachment(normal_, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) {
        fprintf(stderr, "Failed to create normal attachment\n");
        return false;
    }

    if (!create_attachment(albedo_, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) {
        fprintf(stderr, "Failed to create albedo attachment\n");
        return false;
    }

    if (!create_attachment(material_, VK_FORMAT_R8G8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) {
        fprintf(stderr, "Failed to create material attachment\n");
        return false;
    }

    if (!create_attachment(depth_, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT)) {
        fprintf(stderr, "Failed to create depth attachment\n");
        return false;
    }

    if (!create_sampler()) {
        fprintf(stderr, "Failed to create G-buffer sampler\n");
        return false;
    }

    if (!create_render_pass()) {
        fprintf(stderr, "Failed to create G-buffer render pass\n");
        return false;
    }

    if (!create_framebuffer()) {
        fprintf(stderr, "Failed to create G-buffer framebuffer\n");
        return false;
    }

    return true;
}

void GBuffer::destroy() {
    if (!context_ || !context_->device()) return;

    VkDevice device = context_->device();

    if (framebuffer_) {
        vkDestroyFramebuffer(device, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }

    if (render_pass_) {
        vkDestroyRenderPass(device, render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }

    if (sampler_) {
        vkDestroySampler(device, sampler_, nullptr);
        sampler_ = VK_NULL_HANDLE;
    }

    if (depth_sampler_) {
        vkDestroySampler(device, depth_sampler_, nullptr);
        depth_sampler_ = VK_NULL_HANDLE;
    }

    destroy_attachment(position_);
    destroy_attachment(normal_);
    destroy_attachment(albedo_);
    destroy_attachment(material_);
    destroy_attachment(depth_);

    context_ = nullptr;
}

bool GBuffer::resize(uint32_t width, uint32_t height) {
    if (!context_) return false;

    vkDeviceWaitIdle(context_->device());

    // Destroy old framebuffer
    if (framebuffer_) {
        vkDestroyFramebuffer(context_->device(), framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }

    // Destroy old attachments
    destroy_attachment(position_);
    destroy_attachment(normal_);
    destroy_attachment(albedo_);
    destroy_attachment(material_);
    destroy_attachment(depth_);

    width_ = width;
    height_ = height;

    // Recreate attachments
    if (!create_attachment(position_, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) return false;

    if (!create_attachment(normal_, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) return false;

    if (!create_attachment(albedo_, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) return false;

    if (!create_attachment(material_, VK_FORMAT_R8G8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT)) return false;

    if (!create_attachment(depth_, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT)) return false;

    return create_framebuffer();
}

bool GBuffer::create_attachment(GBufferAttachment& attachment, VkFormat format,
                                VkImageUsageFlags usage, VkImageAspectFlags aspect) {
    attachment.format = format;

    // Create image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width_;
    image_info.extent.height = height_;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(context_->device(), &image_info, nullptr, &attachment.image) != VK_SUCCESS) {
        return false;
    }

    // Allocate memory
    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(context_->device(), attachment.image, &mem_req);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = context_->find_memory_type(
        mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(context_->device(), &alloc_info, nullptr, &attachment.memory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(context_->device(), attachment.image, attachment.memory, 0);

    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = attachment.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(context_->device(), &view_info, nullptr, &attachment.view) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void GBuffer::destroy_attachment(GBufferAttachment& attachment) {
    if (!context_ || !context_->device()) return;

    if (attachment.view) {
        vkDestroyImageView(context_->device(), attachment.view, nullptr);
        attachment.view = VK_NULL_HANDLE;
    }
    if (attachment.image) {
        vkDestroyImage(context_->device(), attachment.image, nullptr);
        attachment.image = VK_NULL_HANDLE;
    }
    if (attachment.memory) {
        vkFreeMemory(context_->device(), attachment.memory, nullptr);
        attachment.memory = VK_NULL_HANDLE;
    }
}

bool GBuffer::create_render_pass() {
    // Attachment descriptions
    std::array<VkAttachmentDescription, 5> attachments{};

    // Position
    attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Normal
    attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Albedo
    attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Material
    attachments[3].format = VK_FORMAT_R8G8_UNORM;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth
    attachments[4].format = VK_FORMAT_D32_SFLOAT;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    // Color attachment references
    std::array<VkAttachmentReference, 4> color_refs{};
    color_refs[0] = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    color_refs[1] = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    color_refs[2] = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    color_refs[3] = {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    // Depth attachment reference
    VkAttachmentReference depth_ref{4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(color_refs.size());
    subpass.pColorAttachments = color_refs.data();
    subpass.pDepthStencilAttachment = &depth_ref;

    // Dependencies
    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create render pass
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    return vkCreateRenderPass(context_->device(), &render_pass_info, nullptr, &render_pass_) == VK_SUCCESS;
}

bool GBuffer::create_framebuffer() {
    std::array<VkImageView, 5> attachments = {
        position_.view,
        normal_.view,
        albedo_.view,
        material_.view,
        depth_.view
    };

    VkFramebufferCreateInfo fb_info{};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = render_pass_;
    fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    fb_info.pAttachments = attachments.data();
    fb_info.width = width_;
    fb_info.height = height_;
    fb_info.layers = 1;

    return vkCreateFramebuffer(context_->device(), &fb_info, nullptr, &framebuffer_) == VK_SUCCESS;
}

bool GBuffer::create_sampler() {
    // Color attachment sampler
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_NEAREST;
    sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

    if (vkCreateSampler(context_->device(), &sampler_info, nullptr, &sampler_) != VK_SUCCESS) {
        return false;
    }

    // Depth sampler - configured for depth texture sampling
    VkSamplerCreateInfo depth_sampler_info{};
    depth_sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    depth_sampler_info.magFilter = VK_FILTER_NEAREST;
    depth_sampler_info.minFilter = VK_FILTER_NEAREST;
    depth_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    depth_sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depth_sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depth_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    depth_sampler_info.mipLodBias = 0.0f;
    depth_sampler_info.anisotropyEnable = VK_FALSE;
    depth_sampler_info.maxAnisotropy = 1.0f;
    depth_sampler_info.compareEnable = VK_FALSE;  // We're reading raw depth values
    depth_sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    depth_sampler_info.minLod = 0.0f;
    depth_sampler_info.maxLod = 0.0f;
    depth_sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;  // Use 1.0 for depth border

    return vkCreateSampler(context_->device(), &depth_sampler_info, nullptr, &depth_sampler_) == VK_SUCCESS;
}

VkDescriptorImageInfo GBuffer::position_descriptor() const {
    return {sampler_, position_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

VkDescriptorImageInfo GBuffer::normal_descriptor() const {
    return {sampler_, normal_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

VkDescriptorImageInfo GBuffer::albedo_descriptor() const {
    return {sampler_, albedo_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

VkDescriptorImageInfo GBuffer::material_descriptor() const {
    return {sampler_, material_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

VkDescriptorImageInfo GBuffer::depth_descriptor() const {
    return {depth_sampler_, depth_.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
}

} // namespace slam
