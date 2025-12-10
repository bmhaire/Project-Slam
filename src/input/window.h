/**
 * Slam Engine - Window Management
 *
 * GLFW window wrapper for cross-platform windowing and input
 */

#pragma once

#include <string>
#include <functional>

// Forward declare GLFW types to avoid including GLFW in header
struct GLFWwindow;

namespace slam {

struct WindowConfig {
    std::string title = "Slam Engine";
    int width = 2560;
    int height = 1440;
    bool fullscreen = false;
    bool resizable = true;
    bool vsync = true;
};

class Window {
public:
    // Callbacks
    using ResizeCallback = std::function<void(int width, int height)>;
    using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
    using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
    using MouseMoveCallback = std::function<void(double x, double y)>;
    using ScrollCallback = std::function<void(double x_offset, double y_offset)>;

    Window();
    ~Window();

    // Non-copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Initialize window
    bool init(const WindowConfig& config);

    // Shutdown window
    void shutdown();

    // Poll events
    void poll_events();

    // Check if window should close
    bool should_close() const;

    // Request window close
    void request_close();

    // Get window dimensions
    int width() const { return width_; }
    int height() const { return height_; }
    float aspect_ratio() const { return static_cast<float>(width_) / static_cast<float>(height_); }

    // Get framebuffer dimensions (may differ from window size on HiDPI)
    int framebuffer_width() const { return fb_width_; }
    int framebuffer_height() const { return fb_height_; }

    // Check if window was resized (clears flag after check)
    bool was_resized();

    // Mouse capture mode
    void set_mouse_captured(bool captured);
    bool is_mouse_captured() const { return mouse_captured_; }

    // Get raw GLFW window handle (for Vulkan surface creation)
    GLFWwindow* handle() const { return window_; }

    // Set callbacks
    void set_resize_callback(ResizeCallback callback) { resize_callback_ = callback; }
    void set_key_callback(KeyCallback callback) { key_callback_ = callback; }
    void set_mouse_button_callback(MouseButtonCallback callback) { mouse_button_callback_ = callback; }
    void set_mouse_move_callback(MouseMoveCallback callback) { mouse_move_callback_ = callback; }
    void set_scroll_callback(ScrollCallback callback) { scroll_callback_ = callback; }

    // Get required Vulkan extensions for surface creation
    static const char** get_required_vulkan_extensions(uint32_t* count);

private:
    // GLFW callbacks (static trampolines)
    static void glfw_error_callback(int error, const char* description);
    static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y);
    static void glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset);

    GLFWwindow* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int fb_width_ = 0;
    int fb_height_ = 0;
    bool resized_ = false;
    bool mouse_captured_ = false;

    // Callbacks
    ResizeCallback resize_callback_;
    KeyCallback key_callback_;
    MouseButtonCallback mouse_button_callback_;
    MouseMoveCallback mouse_move_callback_;
    ScrollCallback scroll_callback_;
};

} // namespace slam
