/**
 * Slam Engine - Entry Point
 *
 * A 4-player networked deathmatch game engine
 * Target: macOS M-series, 60fps @ 1440p
 *
 * This is the fly-through demo integrating Phases 1-4:
 * - Vulkan deferred rendering
 * - Procedurally generated maps
 * - PBR lighting with shadows
 * - First-person fly camera
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "utils/math.h"
#include "utils/timer.h"
#include "input/window.h"
#include "input/input_manager.h"
#include "renderer/vulkan_context.h"
#include "renderer/pipeline.h"
#include "renderer/mesh.h"
#include "renderer/camera.h"
#include "renderer/deferred_pipeline.h"
#include "game/map_generator.h"
#include "game/map_mesh.h"

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
    unsigned int map_seed = 12345;
    int map_size = 128;  // Smaller for demo (128x128 instead of 1024)
};

class Engine {
public:
    bool initialize(const EngineConfig& config) {
        config_ = config;

        printf("Slam Engine v0.1.0 - Fly-Through Demo\n");
        printf("Initializing...\n");
        printf("  Resolution: %dx%d\n", config_.window_width, config_.window_height);
        printf("  Map Seed: %u\n", config_.map_seed);

        // Create window
        WindowConfig window_config;
        window_config.title = "Slam Engine - Fly-Through Demo";
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

        // Initialize deferred pipeline
        printf("  Initializing deferred renderer...\n");
        if (!deferred_.init(vulkan_, window_.width(), window_.height())) {
            fprintf(stderr, "Failed to create deferred pipeline\n");
            // Fall back to basic pipeline for now
            use_deferred_ = false;
            if (!pipeline_.create(vulkan_, "shaders/basic.vert.spv", "shaders/basic.frag.spv")) {
                fprintf(stderr, "Failed to create basic pipeline\n");
                return false;
            }
        } else {
            use_deferred_ = true;
        }

        // Generate procedural map
        printf("  Generating procedural map (%dx%d)...\n", config_.map_size, config_.map_size);
        map_generator_ = std::make_unique<MapGenerator>(config_.map_seed);
        map_generator_->set_fill_ratio(0.45f);
        map_generator_->set_smoothing_iterations(5);
        map_generator_->set_min_room_size(30);
        map_generator_->generate(config_.map_size, config_.map_size);

        printf("    Rooms found: %d\n", map_generator_->room_count());
        printf("    Spawn points: %d\n", map_generator_->spawn_count());
        printf("    Props placed: %d\n", map_generator_->prop_count());

        // Generate map mesh
        printf("  Generating map geometry...\n");
        MapMeshConfig mesh_config;
        mesh_config.wall_height = 4.0f;
        mesh_config.uv_scale = 0.25f;
        mesh_config.smooth_walls = true;

        map_mesh_ = std::make_unique<MapMesh>();
        map_mesh_->generate(vulkan_, *map_generator_, mesh_config);

        printf("    Floor vertices: %u\n", map_mesh_->floor_mesh().vertex_count());
        printf("    Wall vertices: %u\n", map_mesh_->wall_mesh().vertex_count());

        // Generate prop meshes
        printf("  Generating props...\n");
        column_mesh_ = PropMeshGenerator::generate_column(vulkan_, 0.3f, 4.0f);
        crate_mesh_ = PropMeshGenerator::generate_crate(vulkan_, 0.8f);
        barrel_mesh_ = PropMeshGenerator::generate_barrel(vulkan_, 0.4f, 1.2f);

        // Setup lights
        setup_lights();

        // Setup camera at first spawn point
        if (!map_generator_->spawns().empty()) {
            const SpawnPoint& spawn = map_generator_->spawns()[0];
            camera_.set_position(spawn.position + vec3(0, 1.6f, 0));  // Eye height
            camera_.set_yaw(spawn.rotation);
        } else {
            camera_.set_position(vec3(0, 5, 0));
        }
        camera_.set_aspect_ratio(window_.aspect_ratio());
        camera_.set_fov(70.0f);
        camera_.set_fly_mode(true);
        camera_.set_move_speed(8.0f);

        // Capture mouse for FPS controls
        window_.set_mouse_captured(true);

        printf("Initialization complete!\n");
        printf("\nControls:\n");
        printf("  WASD      - Move\n");
        printf("  Mouse     - Look around\n");
        printf("  Space     - Move up (fly mode)\n");
        printf("  Ctrl      - Move down (fly mode)\n");
        printf("  Shift     - Sprint\n");
        printf("  Tab       - Toggle mouse capture\n");
        printf("  L         - Toggle lights animation\n");
        printf("  ESC       - Exit\n\n");

        running_ = true;
        return true;
    }

    void setup_lights() {
        auto& lights = deferred_.lights();
        lights.set_ambient(vec3(0.02f, 0.02f, 0.03f), 1.0f);

        // Place lights in rooms
        for (const Room& room : map_generator_->rooms()) {
            // Add a main light in room center
            vec3 room_center = map_generator_->cell_to_world(
                static_cast<int>(room.center.x),
                static_cast<int>(room.center.y)
            );
            room_center.y = 3.0f;  // Near ceiling

            // Vary light color based on room
            float hue = static_cast<float>(lights.light_count()) * 0.618034f;
            hue = hue - std::floor(hue);
            vec3 color = hsv_to_rgb(hue, 0.3f, 1.0f);

            PointLight main_light(room_center, 15.0f, color, 2.0f);
            lights.add_light(main_light);

            // Add extra lights for larger rooms
            if (room.area > 200) {
                // Corner lights
                float offset = std::min(room.width, room.height) * 0.3f;
                vec3 corners[] = {
                    room_center + vec3(-offset, 0, -offset),
                    room_center + vec3(offset, 0, -offset),
                    room_center + vec3(-offset, 0, offset),
                    room_center + vec3(offset, 0, offset)
                };

                for (const vec3& corner : corners) {
                    PointLight corner_light(corner, 8.0f, vec3(1.0f, 0.9f, 0.7f), 1.0f);
                    lights.add_light(corner_light);
                }
            }
        }

        printf("    Lights placed: %d\n", lights.light_count());
    }

    vec3 hsv_to_rgb(float h, float s, float v) {
        float c = v * s;
        float x = c * (1 - std::abs(std::fmod(h * 6, 2) - 1));
        float m = v - c;

        vec3 rgb;
        if (h < 1.0f/6) rgb = vec3(c, x, 0);
        else if (h < 2.0f/6) rgb = vec3(x, c, 0);
        else if (h < 3.0f/6) rgb = vec3(0, c, x);
        else if (h < 4.0f/6) rgb = vec3(0, x, c);
        else if (h < 5.0f/6) rgb = vec3(x, 0, c);
        else rgb = vec3(c, 0, x);

        return rgb + vec3(m);
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

            // Toggle light animation with L
            if (input_.is_key_pressed(SLAM_KEY_L)) {
                animate_lights_ = !animate_lights_;
                printf("Light animation: %s\n", animate_lights_ ? "ON" : "OFF");
            }

            // Update camera
            if (window_.is_mouse_captured()) {
                camera_.update(input_, dt);
            }

            // Handle window resize
            if (window_.was_resized()) {
                camera_.set_aspect_ratio(window_.aspect_ratio());
                if (use_deferred_) {
                    deferred_.resize(window_.width(), window_.height());
                }
            }

            // Update lights
            update_lights(dt);

            // Render
            if (use_deferred_) {
                render_deferred();
            } else {
                render_basic();
            }

            // Update input state (must be after all input checks)
            input_.update();

            // Print FPS every second
            static double last_fps_time = 0;
            if (frame_timer_.total_time() - last_fps_time >= 1.0) {
                printf("FPS: %.1f (%.2fms) | Pos: (%.1f, %.1f, %.1f)\n",
                    frame_timer_.fps(), frame_timer_.frame_time_ms(),
                    camera_.position().x, camera_.position().y, camera_.position().z);
                last_fps_time = frame_timer_.total_time();
            }
        }

        printf("Main loop ended.\n");
    }

    void update_lights(float dt) {
        if (!animate_lights_) return;

        auto& lights = deferred_.lights();
        float time = static_cast<float>(frame_timer_.total_time());

        // Animate light positions slightly
        for (int i = 0; i < lights.light_count(); i++) {
            const PointLight& original = lights.lights()[i];

            // Gentle bobbing motion
            float offset_y = std::sin(time * 2.0f + i * 0.5f) * 0.2f;
            vec3 new_pos = original.position;
            new_pos.y += offset_y;

            lights.set_light_position(i, new_pos);

            // Subtle color pulse
            float pulse = 0.9f + 0.1f * std::sin(time * 3.0f + i * 0.7f);
            lights.set_light_intensity(i, original.intensity * pulse);
        }
    }

    void render_deferred() {
        // Begin frame
        uint32_t image_index;
        if (!vulkan_.begin_frame(image_index)) {
            return; // Swapchain recreation in progress
        }

        VkCommandBuffer cmd = vulkan_.current_command_buffer();

        // Get matrices
        mat4 view = camera_.get_view_matrix();
        mat4 proj = camera_.get_projection_matrix();

        // Update light data
        auto& lights = deferred_.lights();
        lights.upload(camera_.position());
        lights.update_clusters(view, proj, 0.1f, 100.0f);

        // Set view/projection for geometry pass
        deferred_.set_view_projection(view, proj);

        // ---- Geometry Pass ----
        deferred_.begin_geometry_pass(cmd);

        // Draw floor
        if (map_mesh_->has_floor()) {
            deferred_.draw_mesh(cmd, map_mesh_->floor_mesh(), mat4::identity());
        }

        // Draw walls
        if (map_mesh_->has_walls()) {
            deferred_.draw_mesh(cmd, map_mesh_->wall_mesh(), mat4::identity());
        }

        // Draw props
        for (const PropPlacement& prop : map_generator_->props()) {
            mat4 model = translate(prop.position);
            model = rotate(model, prop.rotation, vec3(0, 1, 0));
            model = scale(model, vec3(prop.scale));

            Mesh* mesh = nullptr;
            switch (prop.prop_type) {
                case 0: mesh = column_mesh_.get(); break;
                case 1: mesh = crate_mesh_.get(); break;
                case 2: mesh = barrel_mesh_.get(); break;
            }

            if (mesh) {
                deferred_.draw_mesh(cmd, *mesh, model);
            }
        }

        deferred_.end_geometry_pass(cmd);

        // ---- Lighting Pass ----
        deferred_.begin_lighting_pass(cmd, vulkan_.current_framebuffer(),
                                      vulkan_.render_pass(),
                                      window_.width(), window_.height());

        deferred_.render_lighting(cmd, camera_.position(), 0.1f, 100.0f);

        deferred_.end_lighting_pass(cmd);

        // End frame
        vulkan_.end_frame(image_index);
    }

    void render_basic() {
        // Fallback basic rendering (Phase 1 style)
        uint32_t image_index;
        if (!vulkan_.begin_frame(image_index)) {
            return;
        }

        VkCommandBuffer cmd = vulkan_.current_command_buffer();

        pipeline_.bind(cmd);

        mat4 view = camera_.get_view_matrix();
        mat4 proj = camera_.get_projection_matrix();

        // Draw floor
        if (map_mesh_->has_floor()) {
            map_mesh_->floor_mesh().bind(cmd);

            PushConstants constants;
            mat4 model = mat4::identity();
            memcpy(constants.model, model.data(), sizeof(float) * 16);
            memcpy(constants.view, view.data(), sizeof(float) * 16);
            memcpy(constants.projection, proj.data(), sizeof(float) * 16);

            pipeline_.push_constants(cmd, constants);
            map_mesh_->floor_mesh().draw(cmd);
        }

        // Draw walls
        if (map_mesh_->has_walls()) {
            map_mesh_->wall_mesh().bind(cmd);

            PushConstants constants;
            mat4 model = mat4::identity();
            memcpy(constants.model, model.data(), sizeof(float) * 16);
            memcpy(constants.view, view.data(), sizeof(float) * 16);
            memcpy(constants.projection, proj.data(), sizeof(float) * 16);

            pipeline_.push_constants(cmd, constants);
            map_mesh_->wall_mesh().draw(cmd);
        }

        vulkan_.end_frame(image_index);
    }

    void shutdown() {
        printf("Shutting down...\n");

        // Wait for GPU to finish
        vulkan_.wait_idle();

        // Cleanup in reverse order
        barrel_mesh_.reset();
        crate_mesh_.reset();
        column_mesh_.reset();
        map_mesh_.reset();
        map_generator_.reset();

        deferred_.destroy();
        pipeline_.destroy();
        vulkan_.shutdown();
        window_.shutdown();

        printf("Shutdown complete.\n");
    }

private:
    EngineConfig config_;
    bool running_ = false;
    bool use_deferred_ = true;
    bool animate_lights_ = true;

    Window window_;
    InputManager input_;
    VulkanContext vulkan_;
    Pipeline pipeline_;          // Fallback
    DeferredPipeline deferred_;  // Main renderer
    Camera camera_;
    FrameTimer frame_timer_;

    // Map
    std::unique_ptr<MapGenerator> map_generator_;
    std::unique_ptr<MapMesh> map_mesh_;

    // Props
    std::unique_ptr<Mesh> column_mesh_;
    std::unique_ptr<Mesh> crate_mesh_;
    std::unique_ptr<Mesh> barrel_mesh_;
};

} // namespace slam

void print_usage(const char* program_name) {
    printf("Slam Engine - Fly-Through Demo\n\n");
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --seed <number>     Map generation seed (default: 12345)\n");
    printf("  --size <number>     Map size (default: 128, range: 64-512)\n");
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
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            config.map_size = atoi(argv[++i]);
            if (config.map_size < 64) config.map_size = 64;
            if (config.map_size > 512) config.map_size = 512;
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
