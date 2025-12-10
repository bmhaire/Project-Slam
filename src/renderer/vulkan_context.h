/**
 * Slam Engine - Vulkan Context
 *
 * Core Vulkan setup: instance, device, swapchain, command buffers
 */

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <string>

namespace slam {

class Window;

// Queue family indices
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    bool is_complete() const {
        return graphics_family.has_value() && present_family.has_value();
    }
};

// Swapchain support details
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct VulkanContextConfig {
    bool enable_validation = true;
    bool enable_vsync = true;
    uint32_t max_frames_in_flight = 2;
};

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // Non-copyable
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // Initialize Vulkan
    bool init(Window& window, const VulkanContextConfig& config = {});

    // Cleanup
    void shutdown();

    // Swapchain management
    void recreate_swapchain();

    // Frame management
    bool begin_frame(uint32_t& image_index);
    bool begin_frame(uint32_t& image_index, bool start_render_pass);  // Deferred rendering support
    void end_frame(uint32_t image_index);
    void end_frame(uint32_t image_index, bool end_render_pass);  // Deferred rendering support

    // Wait for device idle (use before shutdown or swapchain recreation)
    void wait_idle();

    // Getters
    VkInstance instance() const { return instance_; }
    VkPhysicalDevice physical_device() const { return physical_device_; }
    VkDevice device() const { return device_; }
    VkQueue graphics_queue() const { return graphics_queue_; }
    VkQueue present_queue() const { return present_queue_; }
    VkSurfaceKHR surface() const { return surface_; }
    VkSwapchainKHR swapchain() const { return swapchain_; }
    VkFormat swapchain_format() const { return swapchain_format_; }
    VkExtent2D swapchain_extent() const { return swapchain_extent_; }
    const std::vector<VkImageView>& swapchain_image_views() const { return swapchain_image_views_; }
    VkRenderPass render_pass() const { return render_pass_; }
    const std::vector<VkFramebuffer>& framebuffers() const { return framebuffers_; }
    VkCommandPool command_pool() const { return command_pool_; }
    VkCommandBuffer current_command_buffer() const { return command_buffers_[current_frame_]; }
    VkFramebuffer current_framebuffer() const { return framebuffers_[current_image_index_]; }
    uint32_t current_frame() const { return current_frame_; }
    uint32_t current_image_index() const { return current_image_index_; }
    uint32_t image_count() const { return static_cast<uint32_t>(swapchain_images_.size()); }

    // Shader helpers
    std::vector<char> load_shader(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    // Memory helpers
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

    // Command buffer helpers
    VkCommandBuffer begin_single_time_commands();
    void end_single_time_commands(VkCommandBuffer command_buffer);

    // Buffer creation helper
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer& buffer, VkDeviceMemory& buffer_memory);

    void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

private:
    // Initialization helpers
    bool create_instance();
    bool setup_debug_messenger();
    bool create_surface();
    bool pick_physical_device();
    bool create_logical_device();
    bool create_swapchain();
    bool create_image_views();
    bool create_render_pass();
    bool create_framebuffers();
    bool create_command_pool();
    bool create_command_buffers();
    bool create_sync_objects();

    // Cleanup helpers
    void cleanup_swapchain();

    // Query helpers
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device);
    bool is_device_suitable(VkPhysicalDevice device);
    bool check_device_extension_support(VkPhysicalDevice device);

    // Swapchain helpers
    VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    // Validation layer helpers
    bool check_validation_layer_support();
    std::vector<const char*> get_required_extensions();

    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);

    // Configuration
    VulkanContextConfig config_;
    Window* window_ = nullptr;

    // Core Vulkan objects
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;

    // Queues
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    QueueFamilyIndices queue_families_;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;

    // Render pass and framebuffers
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    // Command pool and buffers
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers_;

    // Synchronization
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    std::vector<VkFence> images_in_flight_;  // Track which fence each swapchain image is using

    uint32_t current_frame_ = 0;
    uint32_t current_image_index_ = 0;

    // Required device extensions
    const std::vector<const char*> device_extensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset" // Required for MoltenVK
    };

    // Validation layers
    const std::vector<const char*> validation_layers_ = {
        "VK_LAYER_KHRONOS_validation"
    };
};

} // namespace slam
