/**
 * Slam Engine - Camera System
 *
 * First-person camera with mouse look and keyboard movement
 */

#pragma once

#include "utils/math.h"

namespace slam {

class InputManager;

class Camera {
public:
    Camera();

    // Setup
    void set_position(const vec3& position);
    void set_rotation(float pitch, float yaw);
    void set_aspect_ratio(float aspect);
    void set_fov(float fov_degrees);
    void set_near_far(float near_plane, float far_plane);

    // Update camera from input
    void update(InputManager& input, float dt);

    // Calculate matrices
    mat4 get_view_matrix() const;
    mat4 get_projection_matrix() const;

    // Getters
    vec3 position() const { return position_; }
    vec3 forward() const;
    vec3 right() const;
    vec3 up() const;
    float pitch() const { return pitch_; }
    float yaw() const { return yaw_; }

    // Movement settings
    void set_move_speed(float speed) { move_speed_ = speed; }
    void set_sprint_speed(float speed) { sprint_speed_ = speed; }
    void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity_ = sensitivity; }

    // Fly mode (no gravity, free movement)
    void set_fly_mode(bool enabled) { fly_mode_ = enabled; }
    bool is_fly_mode() const { return fly_mode_; }

private:
    // Position and orientation
    vec3 position_;
    float pitch_ = 0.0f; // Up/down rotation (radians)
    float yaw_ = 0.0f;   // Left/right rotation (radians)

    // Projection parameters
    float fov_ = 70.0f * DEG_TO_RAD;
    float aspect_ratio_ = 16.0f / 9.0f;
    float near_plane_ = 0.1f;
    float far_plane_ = 1000.0f;

    // Movement parameters
    float move_speed_ = 5.0f;
    float sprint_speed_ = 10.0f;
    float mouse_sensitivity_ = 1.0f;
    bool fly_mode_ = true;

    // Pitch limits
    static constexpr float MAX_PITCH = 89.0f * DEG_TO_RAD;
    static constexpr float MIN_PITCH = -89.0f * DEG_TO_RAD;
};

} // namespace slam
