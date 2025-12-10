/**
 * Slam Engine - Input Manager Implementation
 */

#include "input_manager.h"
#include "window.h"

namespace slam {

InputManager::InputManager() {
    keys_current_.fill(false);
    keys_previous_.fill(false);
    mouse_current_.fill(false);
    mouse_previous_.fill(false);
}

void InputManager::connect(Window& window) {
    window.set_key_callback([this](int key, int scancode, int action, int mods) {
        on_key(key, scancode, action, mods);
    });

    window.set_mouse_button_callback([this](int button, int action, int mods) {
        on_mouse_button(button, action, mods);
    });

    window.set_mouse_move_callback([this](double x, double y) {
        on_mouse_move(x, y);
    });

    window.set_scroll_callback([this](double x, double y) {
        on_scroll(x, y);
    });
}

void InputManager::update() {
    // Copy current state to previous
    keys_previous_ = keys_current_;
    mouse_previous_ = mouse_current_;
    mouse_pos_previous_ = mouse_pos_;

    // Reset scroll delta (accumulates between frames)
    scroll_delta_ = scroll_accumulated_;
    scroll_accumulated_ = vec2(0, 0);

    // Mouse delta is calculated in on_mouse_move, reset here
    mouse_delta_ = vec2(0, 0);
}

bool InputManager::is_key_down(int key) const {
    if (key < 0 || key >= SLAM_KEY_COUNT) return false;
    return keys_current_[key];
}

bool InputManager::is_key_pressed(int key) const {
    if (key < 0 || key >= SLAM_KEY_COUNT) return false;
    return keys_current_[key] && !keys_previous_[key];
}

bool InputManager::is_key_released(int key) const {
    if (key < 0 || key >= SLAM_KEY_COUNT) return false;
    return !keys_current_[key] && keys_previous_[key];
}

bool InputManager::is_mouse_down(int button) const {
    if (button < 0 || button >= SLAM_MOUSE_BUTTON_COUNT) return false;
    return mouse_current_[button];
}

bool InputManager::is_mouse_pressed(int button) const {
    if (button < 0 || button >= SLAM_MOUSE_BUTTON_COUNT) return false;
    return mouse_current_[button] && !mouse_previous_[button];
}

bool InputManager::is_mouse_released(int button) const {
    if (button < 0 || button >= SLAM_MOUSE_BUTTON_COUNT) return false;
    return !mouse_current_[button] && mouse_previous_[button];
}

void InputManager::on_key(int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key < 0 || key >= SLAM_KEY_COUNT) return;

    if (action == SLAM_PRESS) {
        keys_current_[key] = true;
    } else if (action == SLAM_RELEASE) {
        keys_current_[key] = false;
    }
    // SLAM_REPEAT - key stays down, no state change needed
}

void InputManager::on_mouse_button(int button, int action, int mods) {
    (void)mods;

    if (button < 0 || button >= SLAM_MOUSE_BUTTON_COUNT) return;

    if (action == SLAM_PRESS) {
        mouse_current_[button] = true;
    } else if (action == SLAM_RELEASE) {
        mouse_current_[button] = false;
    }
}

void InputManager::on_mouse_move(double x, double y) {
    vec2 new_pos(static_cast<float>(x), static_cast<float>(y));

    if (first_mouse_) {
        mouse_pos_previous_ = new_pos;
        first_mouse_ = false;
    }

    // Accumulate delta (may get multiple callbacks per frame)
    mouse_delta_ += (new_pos - mouse_pos_) * mouse_sensitivity_;
    mouse_pos_ = new_pos;
}

void InputManager::on_scroll(double x, double y) {
    scroll_accumulated_ += vec2(static_cast<float>(x), static_cast<float>(y));
}

} // namespace slam
