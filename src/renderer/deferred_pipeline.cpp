/**
 * Slam Engine - Deferred Pipeline Implementation
 */

#include "deferred_pipeline.h"
#include "vulkan_context.h"
#include "mesh.h"
#include <array>
#include <cstdio>
#include <cstring>

namespace slam {

DeferredPipeline::~DeferredPipeline() {
    destroy();
}

bool DeferredPipeline::init(VulkanContext& context, uint32_t width, uint32_t height) {
    context_ = &context;
    width_ = width;
    height_ = height;

    // Initialize G-buffer
    if (!gbuffer_.init(context, width, height)) {
        fprintf(stderr, "Failed to initialize G-buffer\n");
        return false;
    }

    // Initialize light manager
    if (!lights_.init(context)) {
        fprintf(stderr, "Failed to initialize light manager\n");
        return false;
    }

    // Initialize shadow maps
    if (!shadows_.init(context)) {
        fprintf(stderr, "Failed to initialize shadow maps\n");
        return false;
    }

    // Create full-screen quad
    float quad_vertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };

    context.create_buffer(sizeof(quad_vertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        quad_vertex_buffer_, quad_vertex_memory_);

    void* data;
    vkMapMemory(context.device(), quad_vertex_memory_, 0, sizeof(quad_vertices), 0, &data);
    memcpy(data, quad_vertices, sizeof(quad_vertices));
    vkUnmapMemory(context.device(), quad_vertex_memory_);

    // Create pipelines
    if (!create_descriptor_sets()) {
        fprintf(stderr, "Failed to create descriptor sets\n");
        return false;
    }

    if (!create_geometry_pipeline()) {
        fprintf(stderr, "Failed to create geometry pipeline\n");
        return false;
    }

    if (!create_lighting_pipeline()) {
        fprintf(stderr, "Failed to create lighting pipeline\n");
        return false;
    }

    if (!create_shadow_pipeline()) {
        fprintf(stderr, "Failed to create shadow pipeline\n");
        return false;
    }

    // Initial descriptor update
    if (!update_descriptor_sets()) {
        fprintf(stderr, "Failed to update descriptor sets\n");
        return false;
    }

    return true;
}

void DeferredPipeline::destroy() {
    if (!context_ || !context_->device()) return;

    VkDevice device = context_->device();
    vkDeviceWaitIdle(device);

    // Destroy quad buffer
    if (quad_vertex_buffer_) {
        vkDestroyBuffer(device, quad_vertex_buffer_, nullptr);
        quad_vertex_buffer_ = VK_NULL_HANDLE;
    }
    if (quad_vertex_memory_) {
        vkFreeMemory(device, quad_vertex_memory_, nullptr);
        quad_vertex_memory_ = VK_NULL_HANDLE;
    }

    // Destroy pipelines
    if (geometry_pipeline_) {
        vkDestroyPipeline(device, geometry_pipeline_, nullptr);
        geometry_pipeline_ = VK_NULL_HANDLE;
    }
    if (geometry_layout_) {
        vkDestroyPipelineLayout(device, geometry_layout_, nullptr);
        geometry_layout_ = VK_NULL_HANDLE;
    }

    if (lighting_pipeline_) {
        vkDestroyPipeline(device, lighting_pipeline_, nullptr);
        lighting_pipeline_ = VK_NULL_HANDLE;
    }
    if (lighting_layout_) {
        vkDestroyPipelineLayout(device, lighting_layout_, nullptr);
        lighting_layout_ = VK_NULL_HANDLE;
    }

    if (shadow_pipeline_) {
        vkDestroyPipeline(device, shadow_pipeline_, nullptr);
        shadow_pipeline_ = VK_NULL_HANDLE;
    }
    if (shadow_layout_) {
        vkDestroyPipelineLayout(device, shadow_layout_, nullptr);
        shadow_layout_ = VK_NULL_HANDLE;
    }

    // Destroy descriptors
    if (lighting_descriptor_pool_) {
        vkDestroyDescriptorPool(device, lighting_descriptor_pool_, nullptr);
        lighting_descriptor_pool_ = VK_NULL_HANDLE;
    }
    if (lighting_descriptor_layout_) {
        vkDestroyDescriptorSetLayout(device, lighting_descriptor_layout_, nullptr);
        lighting_descriptor_layout_ = VK_NULL_HANDLE;
    }

    // Destroy subsystems
    shadows_.destroy();
    lights_.destroy();
    gbuffer_.destroy();

    context_ = nullptr;
}

bool DeferredPipeline::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (!gbuffer_.resize(width, height)) {
        return false;
    }

    return update_descriptor_sets();
}

bool DeferredPipeline::create_descriptor_sets() {
    // Lighting pass descriptors
    // Bindings: 0=position, 1=normal, 2=albedo, 3=material, 4=depth
    //           5=lights, 6=uniforms, 7=clusters, 8=light_indices, 9=shadow_maps
    std::array<VkDescriptorSetLayoutBinding, 10> bindings{};

    // G-buffer textures
    for (int i = 0; i < 5; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    // Light buffer
    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Light uniforms
    bindings[6].binding = 6;
    bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Cluster buffer
    bindings[7].binding = 7;
    bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Light index buffer
    bindings[8].binding = 8;
    bindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Shadow maps
    bindings[9].binding = 9;
    bindings[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[9].descriptorCount = 1;
    bindings[9].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(context_->device(), &layout_info, nullptr,
            &lighting_descriptor_layout_) != VK_SUCCESS) {
        return false;
    }

    // Create descriptor pool
    std::array<VkDescriptorPoolSize, 3> pool_sizes{};
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = 6;  // 5 G-buffer + 1 shadow
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[1].descriptorCount = 3;  // lights, clusters, indices
    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;

    if (vkCreateDescriptorPool(context_->device(), &pool_info, nullptr,
            &lighting_descriptor_pool_) != VK_SUCCESS) {
        return false;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = lighting_descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &lighting_descriptor_layout_;

    return vkAllocateDescriptorSets(context_->device(), &alloc_info,
            &lighting_descriptor_set_) == VK_SUCCESS;
}

bool DeferredPipeline::update_descriptor_sets() {
    std::array<VkWriteDescriptorSet, 10> writes{};

    // G-buffer textures
    VkDescriptorImageInfo position_info = gbuffer_.position_descriptor();
    VkDescriptorImageInfo normal_info = gbuffer_.normal_descriptor();
    VkDescriptorImageInfo albedo_info = gbuffer_.albedo_descriptor();
    VkDescriptorImageInfo material_info = gbuffer_.material_descriptor();
    VkDescriptorImageInfo depth_info = gbuffer_.depth_descriptor();

    for (int i = 0; i < 5; i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = lighting_descriptor_set_;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
    writes[0].pImageInfo = &position_info;
    writes[1].pImageInfo = &normal_info;
    writes[2].pImageInfo = &albedo_info;
    writes[3].pImageInfo = &material_info;
    writes[4].pImageInfo = &depth_info;

    // Light buffers
    VkDescriptorBufferInfo light_info = lights_.light_buffer_info();
    VkDescriptorBufferInfo uniform_info = lights_.uniform_buffer_info();
    VkDescriptorBufferInfo cluster_info = lights_.cluster_buffer_info();
    VkDescriptorBufferInfo index_info = lights_.light_index_buffer_info();

    writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[5].dstSet = lighting_descriptor_set_;
    writes[5].dstBinding = 5;
    writes[5].descriptorCount = 1;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[5].pBufferInfo = &light_info;

    writes[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[6].dstSet = lighting_descriptor_set_;
    writes[6].dstBinding = 6;
    writes[6].descriptorCount = 1;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[6].pBufferInfo = &uniform_info;

    writes[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[7].dstSet = lighting_descriptor_set_;
    writes[7].dstBinding = 7;
    writes[7].descriptorCount = 1;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[7].pBufferInfo = &cluster_info;

    writes[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[8].dstSet = lighting_descriptor_set_;
    writes[8].dstBinding = 8;
    writes[8].descriptorCount = 1;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[8].pBufferInfo = &index_info;

    // Shadow maps
    VkDescriptorImageInfo shadow_info = shadows_.descriptor_info();
    writes[9].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[9].dstSet = lighting_descriptor_set_;
    writes[9].dstBinding = 9;
    writes[9].descriptorCount = 1;
    writes[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[9].pImageInfo = &shadow_info;

    vkUpdateDescriptorSets(context_->device(), static_cast<uint32_t>(writes.size()),
                          writes.data(), 0, nullptr);

    return true;
}

bool DeferredPipeline::create_geometry_pipeline() {
    // Load shaders
    auto vert_code = context_->load_shader("shaders/gbuffer.vert.spv");
    auto frag_code = context_->load_shader("shaders/gbuffer.frag.spv");

    if (vert_code.empty() || frag_code.empty()) {
        fprintf(stderr, "Failed to load G-buffer shaders\n");
        return false;
    }

    VkShaderModule vert_module = context_->create_shader_module(vert_code);
    VkShaderModule frag_module = context_->create_shader_module(frag_code);

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_module;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_module;
    shader_stages[1].pName = "main";

    // Vertex input
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 4> attributes{};
    attributes[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)};
    attributes[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)};
    attributes[2] = {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)};
    attributes[3] = {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)};

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // 4 color attachments for G-buffer
    std::array<VkPipelineColorBlendAttachmentState, 4> blend_attachments{};
    for (auto& att : blend_attachments) {
        att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        att.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = static_cast<uint32_t>(blend_attachments.size());
    color_blend.pAttachments = blend_attachments.data();

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // Push constants
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(GeometryPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(context_->device(), &layout_info, nullptr,
            &geometry_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(context_->device(), vert_module, nullptr);
        vkDestroyShaderModule(context_->device(), frag_module, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = geometry_layout_;
    pipeline_info.renderPass = gbuffer_.render_pass();
    pipeline_info.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(context_->device(), VK_NULL_HANDLE,
        1, &pipeline_info, nullptr, &geometry_pipeline_);

    vkDestroyShaderModule(context_->device(), vert_module, nullptr);
    vkDestroyShaderModule(context_->device(), frag_module, nullptr);

    return result == VK_SUCCESS;
}

bool DeferredPipeline::create_lighting_pipeline() {
    // Load shaders
    auto vert_code = context_->load_shader("shaders/lighting.vert.spv");
    auto frag_code = context_->load_shader("shaders/lighting.frag.spv");

    if (vert_code.empty() || frag_code.empty()) {
        fprintf(stderr, "Failed to load lighting shaders\n");
        return false;
    }

    VkShaderModule vert_module = context_->create_shader_module(vert_code);
    VkShaderModule frag_module = context_->create_shader_module(frag_code);

    VkPipelineShaderStageCreateInfo shader_stages[2]{};
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vert_module;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = frag_module;
    shader_stages[1].pName = "main";

    // Vertex input for full-screen quad
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(float) * 4;  // pos.xy, uv.xy
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributes{};
    attributes[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
    attributes[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2};

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments = &blend_attachment;

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // Push constants
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(LightingPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &lighting_descriptor_layout_;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(context_->device(), &layout_info, nullptr,
            &lighting_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(context_->device(), vert_module, nullptr);
        vkDestroyShaderModule(context_->device(), frag_module, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = lighting_layout_;
    pipeline_info.renderPass = context_->render_pass();
    pipeline_info.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(context_->device(), VK_NULL_HANDLE,
        1, &pipeline_info, nullptr, &lighting_pipeline_);

    vkDestroyShaderModule(context_->device(), vert_module, nullptr);
    vkDestroyShaderModule(context_->device(), frag_module, nullptr);

    return result == VK_SUCCESS;
}

bool DeferredPipeline::create_shadow_pipeline() {
    // Load shadow shader
    auto vert_code = context_->load_shader("shaders/shadow.vert.spv");

    if (vert_code.empty()) {
        fprintf(stderr, "Failed to load shadow shader\n");
        return false;
    }

    VkShaderModule vert_module = context_->create_shader_module(vert_code);

    VkPipelineShaderStageCreateInfo shader_stage{};
    shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage.module = vert_module;
    shader_stage.pName = "main";

    // Vertex input
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute{};
    attribute.binding = 0;
    attribute.location = 0;
    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute.offset = offsetof(Vertex, position);

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = 1;
    vertex_input.pVertexAttributeDescriptions = &attribute;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;  // Front-face culling for shadows
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor = 1.75f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 0;

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // Push constants
    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(ShadowPushConstants);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(context_->device(), &layout_info, nullptr,
            &shadow_layout_) != VK_SUCCESS) {
        vkDestroyShaderModule(context_->device(), vert_module, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 1;
    pipeline_info.pStages = &shader_stage;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = shadow_layout_;
    pipeline_info.renderPass = shadows_.render_pass();
    pipeline_info.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(context_->device(), VK_NULL_HANDLE,
        1, &pipeline_info, nullptr, &shadow_pipeline_);

    vkDestroyShaderModule(context_->device(), vert_module, nullptr);

    return result == VK_SUCCESS;
}

void DeferredPipeline::set_view_projection(const mat4& view, const mat4& proj) {
    view_matrix_ = view;
    proj_matrix_ = proj;
}

void DeferredPipeline::begin_geometry_pass(VkCommandBuffer cmd) {
    std::array<VkClearValue, 5> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};  // Position
    clear_values[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};  // Normal
    clear_values[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}};  // Albedo
    clear_values[3].color = {{0.5f, 0.0f, 0.0f, 0.0f}};  // Material
    clear_values[4].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = gbuffer_.render_pass();
    begin_info.framebuffer = gbuffer_.framebuffer();
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent = {gbuffer_.width(), gbuffer_.height()};
    begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, geometry_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(gbuffer_.width());
    viewport.height = static_cast<float>(gbuffer_.height());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {gbuffer_.width(), gbuffer_.height()};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void DeferredPipeline::end_geometry_pass(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
}

void DeferredPipeline::draw_mesh(VkCommandBuffer cmd, const Mesh& mesh, const mat4& model) {
    // Skip meshes with invalid buffers
    if (mesh.vertex_buffer() == VK_NULL_HANDLE || mesh.index_count() == 0) {
        return;
    }

    GeometryPushConstants push{};
    push.model = model;
    push.view = view_matrix_;
    push.projection = proj_matrix_;

    vkCmdPushConstants(cmd, geometry_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(GeometryPushConstants), &push);

    VkBuffer vertex_buffers[] = {mesh.vertex_buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(cmd, mesh.index_buffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, mesh.index_count(), 1, 0, 0, 0);
}

void DeferredPipeline::begin_lighting_pass(VkCommandBuffer cmd, VkFramebuffer target_framebuffer,
                                          VkRenderPass target_render_pass, uint32_t width, uint32_t height) {
    VkClearValue clear_value{};
    clear_value.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = target_render_pass;
    begin_info.framebuffer = target_framebuffer;
    begin_info.renderArea.offset = {0, 0};
    begin_info.renderArea.extent = {width, height};
    begin_info.clearValueCount = 1;
    begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void DeferredPipeline::end_lighting_pass(VkCommandBuffer cmd) {
    vkCmdEndRenderPass(cmd);
}

void DeferredPipeline::render_lighting(VkCommandBuffer cmd, const vec3& camera_pos,
                                       float near_plane, float far_plane) {
    // Bind descriptor set
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lighting_layout_,
                           0, 1, &lighting_descriptor_set_, 0, nullptr);

    // Push constants
    mat4 view_proj = proj_matrix_ * view_matrix_;
    mat4 inv_view_proj = view_proj.inverse();

    LightingPushConstants push{};
    push.inv_view_proj = inv_view_proj;
    push.camera_pos = vec4(camera_pos.x, camera_pos.y, camera_pos.z, 0.0f);
    push.screen_size = vec4(static_cast<float>(width_), static_cast<float>(height_),
                           near_plane, far_plane);

    vkCmdPushConstants(cmd, lighting_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(LightingPushConstants), &push);

    // Draw full-screen quad
    VkBuffer vertex_buffers[] = {quad_vertex_buffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void DeferredPipeline::render_shadows(VkCommandBuffer cmd, const std::vector<Mesh*>& meshes,
                                     const std::vector<mat4>& transforms) {
    uint32_t resolution = shadows_.resolution();
    float near_plane = 0.1f;

    // For each shadow-casting light
    int num_shadow_lights = std::min(static_cast<int>(lights_.lights().size()),
                                    static_cast<int>(MAX_SHADOW_CASTERS));

    for (int light_idx = 0; light_idx < num_shadow_lights; light_idx++) {
        const PointLight& light = lights_.lights()[light_idx];
        float far_plane = light.radius;

        mat4 proj = ShadowMapArray::get_projection(near_plane, far_plane);

        // Render each cubemap face
        for (uint32_t face = 0; face < 6; face++) {
            mat4 view = ShadowMapArray::get_face_view(light.position, face);

            VkClearValue clear_value{};
            clear_value.depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            begin_info.renderPass = shadows_.render_pass();
            begin_info.framebuffer = shadows_.framebuffer(light_idx, face);
            begin_info.renderArea.offset = {0, 0};
            begin_info.renderArea.extent = {resolution, resolution};
            begin_info.clearValueCount = 1;
            begin_info.pClearValues = &clear_value;

            vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline_);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(resolution);
            viewport.height = static_cast<float>(resolution);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = {resolution, resolution};
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Draw all meshes
            for (size_t i = 0; i < meshes.size(); i++) {
                // Skip null meshes or meshes with invalid buffers
                if (!meshes[i] || meshes[i]->vertex_buffer() == VK_NULL_HANDLE) {
                    continue;
                }

                ShadowPushConstants push{};
                push.light_space_matrix = proj * view * transforms[i];
                push.light_pos = vec4(light.position.x, light.position.y,
                                     light.position.z, far_plane);

                vkCmdPushConstants(cmd, shadow_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                                   0, sizeof(ShadowPushConstants), &push);

                VkBuffer vertex_buffers[] = {meshes[i]->vertex_buffer()};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
                vkCmdBindIndexBuffer(cmd, meshes[i]->index_buffer(), 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, meshes[i]->index_count(), 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(cmd);
        }
    }
}

} // namespace slam
