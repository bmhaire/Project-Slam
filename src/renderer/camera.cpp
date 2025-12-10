/**
 * Slam Engine - Camera Implementation
 */

#include "camera.h"
#include "input/input_manager.h"

namespace slam {

Camera::Camera() : position_(0, 2, 5) {}

void Camera::set_position(const vec3& position) {
    position_ = position;
}

void Camera::set_rotation(float pitch, float yaw) {
    pitch_ = clamp(pitch, MIN_PITCH, MAX_PITCH);
    yaw_ = yaw;
}

void Camera::set_aspect_ratio(float aspect) {
    aspect_ratio_ = aspect;
}

void Camera::set_fov(float fov_degrees) {
    fov_ = fov_degrees * DEG_TO_RAD;
}

void Camera::set_near_far(float near_plane, float far_plane) {
    near_plane_ = near_plane;
    far_plane_ = far_plane;
}

void Camera::update(InputManager& input, float dt) {
    // Mouse look
    vec2 mouse_delta = input.mouse_delta();
    yaw_ += mouse_delta.x * mouse_sensitivity_;
    pitch_ -= mouse_delta.y * mouse_sensitivity_;
    pitch_ = clamp(pitch_, MIN_PITCH, MAX_PITCH);

    // Movement
    vec3 move_dir(0);

    vec3 fwd = forward();
    vec3 rgt = right();

    // In fly mode, use camera forward; otherwise, project forward onto horizontal plane
    if (!fly_mode_) {
        fwd.y = 0;
        fwd = normalize(fwd);
    }

    if (input.is_key_down(SLAM_KEY_W)) {
        move_dir += fwd;
    }
    if (input.is_key_down(SLAM_KEY_S)) {
        move_dir -= fwd;
    }
    if (input.is_key_down(SLAM_KEY_D)) {
        move_dir += rgt;
    }
    if (input.is_key_down(SLAM_KEY_A)) {
        move_dir -= rgt;
    }

    // Vertical movement in fly mode
    if (fly_mode_) {
        if (input.is_key_down(SLAM_KEY_SPACE)) {
            move_dir.y += 1.0f;
        }
        if (input.is_key_down(SLAM_KEY_LEFT_CONTROL) || input.is_key_down(SLAM_KEY_C)) {
            move_dir.y -= 1.0f;
        }
    }

    // Normalize and apply speed
    if (move_dir.length_squared() > 0.001f) {
        move_dir = normalize(move_dir);

        float speed = move_speed_;
        if (input.is_key_down(SLAM_KEY_LEFT_SHIFT)) {
            speed = sprint_speed_;
        }

        position_ += move_dir * speed * dt;
    }
}

vec3 Camera::forward() const {
    // Calculate forward vector from pitch and yaw
    float cos_pitch = std::cos(pitch_);
    return vec3(
        std::sin(yaw_) * cos_pitch,
        std::sin(pitch_),
        -std::cos(yaw_) * cos_pitch
    );
}

vec3 Camera::right() const {
    return normalize(cross(forward(), vec3(0, 1, 0)));
}

vec3 Camera::up() const {
    return normalize(cross(right(), forward()));
}

mat4 Camera::get_view_matrix() const {
    return look_at(position_, position_ + forward(), vec3(0, 1, 0));
}

mat4 Camera::get_projection_matrix() const {
    return perspective(fov_, aspect_ratio_, near_plane_, far_plane_);
}

} // namespace slam
