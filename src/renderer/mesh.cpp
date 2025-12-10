/**
 * Slam Engine - Mesh System Implementation
 */

#include "mesh.h"
#include "vulkan_context.h"
#include <cstring>

namespace slam {

Mesh::Mesh() = default;

Mesh::~Mesh() {
    destroy();
}

Mesh::Mesh(Mesh&& other) noexcept
    : context_(other.context_)
    , vertex_buffer_(other.vertex_buffer_)
    , vertex_buffer_memory_(other.vertex_buffer_memory_)
    , index_buffer_(other.index_buffer_)
    , index_buffer_memory_(other.index_buffer_memory_)
    , vertex_count_(other.vertex_count_)
    , index_count_(other.index_count_) {
    other.context_ = nullptr;
    other.vertex_buffer_ = VK_NULL_HANDLE;
    other.vertex_buffer_memory_ = VK_NULL_HANDLE;
    other.index_buffer_ = VK_NULL_HANDLE;
    other.index_buffer_memory_ = VK_NULL_HANDLE;
    other.vertex_count_ = 0;
    other.index_count_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        context_ = other.context_;
        vertex_buffer_ = other.vertex_buffer_;
        vertex_buffer_memory_ = other.vertex_buffer_memory_;
        index_buffer_ = other.index_buffer_;
        index_buffer_memory_ = other.index_buffer_memory_;
        vertex_count_ = other.vertex_count_;
        index_count_ = other.index_count_;
        other.context_ = nullptr;
        other.vertex_buffer_ = VK_NULL_HANDLE;
        other.vertex_buffer_memory_ = VK_NULL_HANDLE;
        other.index_buffer_ = VK_NULL_HANDLE;
        other.index_buffer_memory_ = VK_NULL_HANDLE;
        other.vertex_count_ = 0;
        other.index_count_ = 0;
    }
    return *this;
}

bool Mesh::create(VulkanContext& context,
                  const std::vector<Vertex>& vertices,
                  const std::vector<uint32_t>& indices) {
    // Validate input - empty meshes are not allowed
    if (vertices.empty()) {
        fprintf(stderr, "Mesh::create() called with empty vertices\n");
        return false;
    }
    if (indices.empty()) {
        fprintf(stderr, "Mesh::create() called with empty indices\n");
        return false;
    }

    context_ = &context;
    vertex_count_ = static_cast<uint32_t>(vertices.size());
    index_count_ = static_cast<uint32_t>(indices.size());

    // Create vertex buffer
    VkDeviceSize vertex_size = sizeof(Vertex) * vertices.size();

    // Create staging buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    context_->create_buffer(vertex_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    // Copy vertex data
    void* data;
    vkMapMemory(context_->device(), staging_buffer_memory, 0, vertex_size, 0, &data);
    memcpy(data, vertices.data(), vertex_size);
    vkUnmapMemory(context_->device(), staging_buffer_memory);

    // Create device-local vertex buffer
    context_->create_buffer(vertex_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertex_buffer_, vertex_buffer_memory_);

    // Copy to device
    context_->copy_buffer(staging_buffer, vertex_buffer_, vertex_size);

    // Cleanup staging buffer
    vkDestroyBuffer(context_->device(), staging_buffer, nullptr);
    vkFreeMemory(context_->device(), staging_buffer_memory, nullptr);

    // Create index buffer
    VkDeviceSize index_size = sizeof(uint32_t) * indices.size();

    context_->create_buffer(index_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging_buffer, staging_buffer_memory);

    vkMapMemory(context_->device(), staging_buffer_memory, 0, index_size, 0, &data);
    memcpy(data, indices.data(), index_size);
    vkUnmapMemory(context_->device(), staging_buffer_memory);

    context_->create_buffer(index_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer_, index_buffer_memory_);

    context_->copy_buffer(staging_buffer, index_buffer_, index_size);

    vkDestroyBuffer(context_->device(), staging_buffer, nullptr);
    vkFreeMemory(context_->device(), staging_buffer_memory, nullptr);

    return true;
}

void Mesh::destroy() {
    if (context_ && context_->device()) {
        if (index_buffer_) {
            vkDestroyBuffer(context_->device(), index_buffer_, nullptr);
            index_buffer_ = VK_NULL_HANDLE;
        }
        if (index_buffer_memory_) {
            vkFreeMemory(context_->device(), index_buffer_memory_, nullptr);
            index_buffer_memory_ = VK_NULL_HANDLE;
        }
        if (vertex_buffer_) {
            vkDestroyBuffer(context_->device(), vertex_buffer_, nullptr);
            vertex_buffer_ = VK_NULL_HANDLE;
        }
        if (vertex_buffer_memory_) {
            vkFreeMemory(context_->device(), vertex_buffer_memory_, nullptr);
            vertex_buffer_memory_ = VK_NULL_HANDLE;
        }
    }
}

void Mesh::bind(VkCommandBuffer command_buffer) {
    VkBuffer buffers[] = {vertex_buffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::draw(VkCommandBuffer command_buffer) {
    vkCmdDrawIndexed(command_buffer, index_count_, 1, 0, 0, 0);
}

void Mesh::draw(VkCommandBuffer command_buffer, uint32_t count, uint32_t first_index) {
    vkCmdDrawIndexed(command_buffer, count, 1, first_index, 0, 0);
}

Mesh Mesh::create_cube(VulkanContext& context, float size) {
    float h = size * 0.5f;

    // Colors for each face
    vec3 colors[] = {
        vec3(1, 0, 0), // Red - front
        vec3(0, 1, 0), // Green - back
        vec3(0, 0, 1), // Blue - right
        vec3(1, 1, 0), // Yellow - left
        vec3(1, 0, 1), // Magenta - top
        vec3(0, 1, 1), // Cyan - bottom
    };

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Front face (Z+)
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(-h, -h, h), colors[0], vec3(0, 0, 1), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(h, -h, h), colors[0], vec3(0, 0, 1), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, h, h), colors[0], vec3(0, 0, 1), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, h, h), colors[0], vec3(0, 0, 1), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Back face (Z-)
    base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(h, -h, -h), colors[1], vec3(0, 0, -1), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(-h, -h, -h), colors[1], vec3(0, 0, -1), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, h, -h), colors[1], vec3(0, 0, -1), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, h, -h), colors[1], vec3(0, 0, -1), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Right face (X+)
    base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(h, -h, h), colors[2], vec3(1, 0, 0), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(h, -h, -h), colors[2], vec3(1, 0, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, h, -h), colors[2], vec3(1, 0, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, h, h), colors[2], vec3(1, 0, 0), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Left face (X-)
    base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(-h, -h, -h), colors[3], vec3(-1, 0, 0), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(-h, -h, h), colors[3], vec3(-1, 0, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, h, h), colors[3], vec3(-1, 0, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, h, -h), colors[3], vec3(-1, 0, 0), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Top face (Y+)
    base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(-h, h, h), colors[4], vec3(0, 1, 0), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(h, h, h), colors[4], vec3(0, 1, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, h, -h), colors[4], vec3(0, 1, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, h, -h), colors[4], vec3(0, 1, 0), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Bottom face (Y-)
    base = static_cast<uint32_t>(vertices.size());
    vertices.push_back(Vertex(vec3(-h, -h, -h), colors[5], vec3(0, -1, 0), vec2(0, 1)));
    vertices.push_back(Vertex(vec3(h, -h, -h), colors[5], vec3(0, -1, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, -h, h), colors[5], vec3(0, -1, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, -h, h), colors[5], vec3(0, -1, 0), vec2(0, 0)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    Mesh mesh;
    mesh.create(context, vertices, indices);
    return mesh;
}

Mesh Mesh::create_cube(VulkanContext& context, float size, const vec3& color) {
    float h = size * 0.5f;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Front face
    uint32_t base = 0;
    vertices.push_back(Vertex(vec3(-h, -h, h), color, vec3(0, 0, 1), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(h, -h, h), color, vec3(0, 0, 1), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, h, h), color, vec3(0, 0, 1), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, h, h), color, vec3(0, 0, 1), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Back face
    base = 4;
    vertices.push_back(Vertex(vec3(h, -h, -h), color, vec3(0, 0, -1), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(-h, -h, -h), color, vec3(0, 0, -1), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, h, -h), color, vec3(0, 0, -1), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, h, -h), color, vec3(0, 0, -1), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Right face
    base = 8;
    vertices.push_back(Vertex(vec3(h, -h, h), color, vec3(1, 0, 0), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(h, -h, -h), color, vec3(1, 0, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, h, -h), color, vec3(1, 0, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(h, h, h), color, vec3(1, 0, 0), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Left face
    base = 12;
    vertices.push_back(Vertex(vec3(-h, -h, -h), color, vec3(-1, 0, 0), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(-h, -h, h), color, vec3(-1, 0, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(-h, h, h), color, vec3(-1, 0, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, h, -h), color, vec3(-1, 0, 0), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Top face
    base = 16;
    vertices.push_back(Vertex(vec3(-h, h, h), color, vec3(0, 1, 0), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(h, h, h), color, vec3(0, 1, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, h, -h), color, vec3(0, 1, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, h, -h), color, vec3(0, 1, 0), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    // Bottom face
    base = 20;
    vertices.push_back(Vertex(vec3(-h, -h, -h), color, vec3(0, -1, 0), vec2(0, 0)));
    vertices.push_back(Vertex(vec3(h, -h, -h), color, vec3(0, -1, 0), vec2(1, 0)));
    vertices.push_back(Vertex(vec3(h, -h, h), color, vec3(0, -1, 0), vec2(1, 1)));
    vertices.push_back(Vertex(vec3(-h, -h, h), color, vec3(0, -1, 0), vec2(0, 1)));
    indices.insert(indices.end(), {base, base + 1, base + 2, base + 2, base + 3, base});

    Mesh mesh;
    mesh.create(context, vertices, indices);
    return mesh;
}

Mesh Mesh::create_plane(VulkanContext& context, float size) {
    float h = size * 0.5f;

    std::vector<Vertex> vertices = {
        Vertex(vec3(-h, 0, -h), vec3(0.5f), vec3(0, 1, 0), vec2(0, 0)),
        Vertex(vec3(h, 0, -h), vec3(0.5f), vec3(0, 1, 0), vec2(1, 0)),
        Vertex(vec3(h, 0, h), vec3(0.5f), vec3(0, 1, 0), vec2(1, 1)),
        Vertex(vec3(-h, 0, h), vec3(0.5f), vec3(0, 1, 0), vec2(0, 1)),
    };

    std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

    Mesh mesh;
    mesh.create(context, vertices, indices);
    return mesh;
}

} // namespace slam
