/**
 * Slam Engine - Entry Point
 *
 * A 4-player networked deathmatch game engine
 * Target: macOS M-series, 60fps @ 1440p
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "utils/math.h"
#include "utils/timer.h"
#include "input/window.h"
#include "input/input_manager.h"
#include "renderer/vulkan_context.h"
#include "renderer/pipeline.h"
#include "renderer/mesh.h"
#include "renderer/camera.h"

namespace slam {

struct EngineConfig {
    int window_width = 2560;
    int window_height = 1440;
    bool fullscreen = false;
    bool vsync = true;
    bool enable_validation = true;

    // Network settings
    bool is_host = false;
    const char* connect_address = nullptr;
    int port = 7777;

    // Map settings
    unsigned int map_seed = 0;
};

class Engine {
public:
    bool initialize(const EngineConfig& config) {
        config_ = config;

        printf("Slam Engine v0.1.0\n");
        printf("Initializing...\n");
        printf("  Resolution: %dx%d\n", config_.window_width, config_.window_height);
        printf("  Mode: %s\n", config_.is_host ? "Host" : (config_.connect_address ? "Client" : "Local"));

        // Create window
        WindowConfig window_config;
        window_config.title = "Slam Engine";
        window_config.width = config_.window_width;
        window_config.height = config_.window_height;
        window_config.fullscreen = config_.fullscreen;
        window_config.vsync = config_.vsync;

        if (!window_.init(window_config)) {
            fprintf(stderr, "Failed to create window\n");
            return false;
        }

        // Connect input manager
        input_.connect(window_);

        // Initialize Vulkan
        VulkanContextConfig vk_config;
        vk_config.enable_validation = config_.enable_validation;
        vk_config.enable_vsync = config_.vsync;

        if (!vulkan_.init(window_, vk_config)) {
            fprintf(stderr, "Failed to initialize Vulkan\n");
            return false;
        }

        // Create graphics pipeline
        if (!pipeline_.create(vulkan_, "shaders/basic.vert.spv", "shaders/basic.frag.spv")) {
            fprintf(stderr, "Failed to create graphics pipeline\n");
            return false;
        }

        // Create test mesh (cube)
        cube_ = Mesh::create_cube(vulkan_, 1.0f);

        // Setup camera
        camera_.set_position(vec3(0, 2, 5));
        camera_.set_aspect_ratio(window_.aspect_ratio());
        camera_.set_fov(70.0f);
        camera_.set_fly_mode(true);

        // Capture mouse for FPS controls
        window_.set_mouse_captured(true);

        printf("Initialization complete!\n");
        printf("\nControls:\n");
        printf("  WASD - Move\n");
        printf("  Mouse - Look around\n");
        printf("  Space/Ctrl - Move up/down (fly mode)\n");
        printf("  Shift - Sprint\n");
        printf("  ESC - Exit\n\n");

        running_ = true;
        return true;
    }

    void run() {
        printf("Starting main loop...\n");

        while (running_ && !window_.should_close()) {
            // Begin frame timing
            frame_timer_.begin_frame();
            float dt = frame_timer_.delta_time_f();

            // Poll input
            window_.poll_events();

            // Handle escape key
            if (input_.is_key_pressed(SLAM_KEY_ESCAPE)) {
                running_ = false;
                continue;
            }

            // Toggle mouse capture with Tab
            if (input_.is_key_pressed(SLAM_KEY_TAB)) {
                window_.set_mouse_captured(!window_.is_mouse_captured());
            }

            // Update camera
            if (window_.is_mouse_captured()) {
                camera_.update(input_, dt);
            }

            // Handle window resize
            if (window_.was_resized()) {
                camera_.set_aspect_ratio(window_.aspect_ratio());
                // Vulkan context handles swapchain recreation internally
            }

            // Render
            render();

            // Update input state (must be after all input checks)
            input_.update();

            // Print FPS every second
            static double last_fps_time = 0;
            if (frame_timer_.total_time() - last_fps_time >= 1.0) {
                printf("FPS: %.1f (%.2fms)\n", frame_timer_.fps(), frame_timer_.frame_time_ms());
                last_fps_time = frame_timer_.total_time();
            }
        }

        printf("Main loop ended.\n");
    }

    void render() {
        // Begin frame
        uint32_t image_index;
        if (!vulkan_.begin_frame(image_index)) {
            return; // Swapchain recreation in progress
        }

        VkCommandBuffer cmd = vulkan_.current_command_buffer();

        // Bind pipeline
        pipeline_.bind(cmd);

        // Get matrices
        mat4 view = camera_.get_view_matrix();
        mat4 proj = camera_.get_projection_matrix();

        // Draw multiple cubes
        cube_.bind(cmd);

        // Draw a grid of cubes
        for (int x = -2; x <= 2; x++) {
            for (int z = -2; z <= 2; z++) {
                mat4 model = translate(vec3(x * 3.0f, 0, z * 3.0f));

                // Rotate cubes slightly based on time
                float time = static_cast<float>(frame_timer_.total_time());
                model = rotate(model, time * 0.5f + x * 0.1f, vec3(0, 1, 0));

                // Push MVP matrices
                PushConstants constants;
                memcpy(constants.model, model.data(), sizeof(float) * 16);
                memcpy(constants.view, view.data(), sizeof(float) * 16);
                memcpy(constants.projection, proj.data(), sizeof(float) * 16);

                pipeline_.push_constants(cmd, constants);
                cube_.draw(cmd);
            }
        }

        // End frame
        vulkan_.end_frame(image_index);
    }

    void shutdown() {
        printf("Shutting down...\n");

        // Wait for GPU to finish
        vulkan_.wait_idle();

        // Cleanup in reverse order
        cube_.destroy();
        pipeline_.destroy();
        vulkan_.shutdown();
        window_.shutdown();

        printf("Shutdown complete.\n");
    }

private:
    EngineConfig config_;
    bool running_ = false;

    Window window_;
    InputManager input_;
    VulkanContext vulkan_;
    Pipeline pipeline_;
    Mesh cube_;
    Camera camera_;
    FrameTimer frame_timer_;
};

} // namespace slam

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --host              Start as game host/server\n");
    printf("  --connect <ip>      Connect to a host\n");
    printf("  --port <port>       Network port (default: 7777)\n");
    printf("  --seed <number>     Map generation seed\n");
    printf("  --windowed          Run in windowed mode (1920x1080)\n");
    printf("  --no-validation     Disable Vulkan validation layers\n");
    printf("  --no-vsync          Disable VSync\n");
    printf("  --help              Show this help message\n");
}

int main(int argc, char* argv[]) {
    slam::EngineConfig config;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--host") == 0) {
            config.is_host = true;
        }
        else if (strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
            config.connect_address = argv[++i];
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config.port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            config.map_seed = static_cast<unsigned int>(atoi(argv[++i]));
        }
        else if (strcmp(argv[i], "--windowed") == 0) {
            config.fullscreen = false;
            config.window_width = 1920;
            config.window_height = 1080;
        }
        else if (strcmp(argv[i], "--no-validation") == 0) {
            config.enable_validation = false;
        }
        else if (strcmp(argv[i], "--no-vsync") == 0) {
            config.vsync = false;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // Create and run engine
    slam::Engine engine;

    if (!engine.initialize(config)) {
        printf("Failed to initialize engine!\n");
        return 1;
    }

    engine.run();
    engine.shutdown();

    return 0;
}
