/**
 * Slam Engine - Window Management Implementation
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window.h"
#include <cstdio>

namespace slam {

Window::Window() = default;

Window::~Window() {
    shutdown();
}

bool Window::init(const WindowConfig& config) {
    // Set error callback before any GLFW calls
    glfwSetErrorCallback(glfw_error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    // Check Vulkan support
    if (!glfwVulkanSupported()) {
        fprintf(stderr, "Vulkan not supported by GLFW\n");
        glfwTerminate();
        return false;
    }

    // Window hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // Get monitor for fullscreen
    GLFWmonitor* monitor = nullptr;
    int win_width = config.width;
    int win_height = config.height;

    if (config.fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        win_width = mode->width;
        win_height = mode->height;
    }

    // Create window
    window_ = glfwCreateWindow(win_width, win_height, config.title.c_str(), monitor, nullptr);
    if (!window_) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    // Store this pointer for callbacks
    glfwSetWindowUserPointer(window_, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window_, glfw_framebuffer_size_callback);
    glfwSetKeyCallback(window_, glfw_key_callback);
    glfwSetMouseButtonCallback(window_, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window_, glfw_cursor_pos_callback);
    glfwSetScrollCallback(window_, glfw_scroll_callback);

    // Get initial sizes
    glfwGetWindowSize(window_, &width_, &height_);
    glfwGetFramebufferSize(window_, &fb_width_, &fb_height_);

    printf("Window created: %dx%d (framebuffer: %dx%d)\n", width_, height_, fb_width_, fb_height_);

    return true;
}

void Window::shutdown() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Window::poll_events() {
    glfwPollEvents();
}

bool Window::should_close() const {
    return window_ && glfwWindowShouldClose(window_);
}

void Window::request_close() {
    if (window_) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

bool Window::was_resized() {
    bool result = resized_;
    resized_ = false;
    return result;
}

void Window::set_mouse_captured(bool captured) {
    if (window_) {
        mouse_captured_ = captured;
        glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        // Enable raw mouse motion if available
        if (captured && glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    }
}

const char** Window::get_required_vulkan_extensions(uint32_t* count) {
    return glfwGetRequiredInstanceExtensions(count);
}

// Static callback implementations

void Window::glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void Window::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->fb_width_ = width;
        self->fb_height_ = height;
        glfwGetWindowSize(window, &self->width_, &self->height_);
        self->resized_ = true;

        if (self->resize_callback_) {
            self->resize_callback_(width, height);
        }
    }
}

void Window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->key_callback_) {
        self->key_callback_(key, scancode, action, mods);
    }
}

void Window::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->mouse_button_callback_) {
        self->mouse_button_callback_(button, action, mods);
    }
}

void Window::glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->mouse_move_callback_) {
        self->mouse_move_callback_(x, y);
    }
}

void Window::glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset) {
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self && self->scroll_callback_) {
        self->scroll_callback_(x_offset, y_offset);
    }
}

} // namespace slam
