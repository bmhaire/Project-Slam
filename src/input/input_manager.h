/**
 * Slam Engine - Input Manager
 *
 * Keyboard and mouse state tracking
 */

#pragma once

#include "utils/math.h"
#include <array>

// GLFW key codes
#define SLAM_KEY_UNKNOWN            -1
#define SLAM_KEY_SPACE              32
#define SLAM_KEY_APOSTROPHE         39
#define SLAM_KEY_COMMA              44
#define SLAM_KEY_MINUS              45
#define SLAM_KEY_PERIOD             46
#define SLAM_KEY_SLASH              47
#define SLAM_KEY_0                  48
#define SLAM_KEY_1                  49
#define SLAM_KEY_2                  50
#define SLAM_KEY_3                  51
#define SLAM_KEY_4                  52
#define SLAM_KEY_5                  53
#define SLAM_KEY_6                  54
#define SLAM_KEY_7                  55
#define SLAM_KEY_8                  56
#define SLAM_KEY_9                  57
#define SLAM_KEY_SEMICOLON          59
#define SLAM_KEY_EQUAL              61
#define SLAM_KEY_A                  65
#define SLAM_KEY_B                  66
#define SLAM_KEY_C                  67
#define SLAM_KEY_D                  68
#define SLAM_KEY_E                  69
#define SLAM_KEY_F                  70
#define SLAM_KEY_G                  71
#define SLAM_KEY_H                  72
#define SLAM_KEY_I                  73
#define SLAM_KEY_J                  74
#define SLAM_KEY_K                  75
#define SLAM_KEY_L                  76
#define SLAM_KEY_M                  77
#define SLAM_KEY_N                  78
#define SLAM_KEY_O                  79
#define SLAM_KEY_P                  80
#define SLAM_KEY_Q                  81
#define SLAM_KEY_R                  82
#define SLAM_KEY_S                  83
#define SLAM_KEY_T                  84
#define SLAM_KEY_U                  85
#define SLAM_KEY_V                  86
#define SLAM_KEY_W                  87
#define SLAM_KEY_X                  88
#define SLAM_KEY_Y                  89
#define SLAM_KEY_Z                  90
#define SLAM_KEY_LEFT_BRACKET       91
#define SLAM_KEY_BACKSLASH          92
#define SLAM_KEY_RIGHT_BRACKET      93
#define SLAM_KEY_GRAVE_ACCENT       96
#define SLAM_KEY_ESCAPE             256
#define SLAM_KEY_ENTER              257
#define SLAM_KEY_TAB                258
#define SLAM_KEY_BACKSPACE          259
#define SLAM_KEY_INSERT             260
#define SLAM_KEY_DELETE             261
#define SLAM_KEY_RIGHT              262
#define SLAM_KEY_LEFT               263
#define SLAM_KEY_DOWN               264
#define SLAM_KEY_UP                 265
#define SLAM_KEY_PAGE_UP            266
#define SLAM_KEY_PAGE_DOWN          267
#define SLAM_KEY_HOME               268
#define SLAM_KEY_END                269
#define SLAM_KEY_CAPS_LOCK          280
#define SLAM_KEY_SCROLL_LOCK        281
#define SLAM_KEY_NUM_LOCK           282
#define SLAM_KEY_PRINT_SCREEN       283
#define SLAM_KEY_PAUSE              284
#define SLAM_KEY_F1                 290
#define SLAM_KEY_F2                 291
#define SLAM_KEY_F3                 292
#define SLAM_KEY_F4                 293
#define SLAM_KEY_F5                 294
#define SLAM_KEY_F6                 295
#define SLAM_KEY_F7                 296
#define SLAM_KEY_F8                 297
#define SLAM_KEY_F9                 298
#define SLAM_KEY_F10                299
#define SLAM_KEY_F11                300
#define SLAM_KEY_F12                301
#define SLAM_KEY_LEFT_SHIFT         340
#define SLAM_KEY_LEFT_CONTROL       341
#define SLAM_KEY_LEFT_ALT           342
#define SLAM_KEY_LEFT_SUPER         343
#define SLAM_KEY_RIGHT_SHIFT        344
#define SLAM_KEY_RIGHT_CONTROL      345
#define SLAM_KEY_RIGHT_ALT          346
#define SLAM_KEY_RIGHT_SUPER        347

#define SLAM_KEY_LAST               SLAM_KEY_RIGHT_SUPER
#define SLAM_KEY_COUNT              512

// Mouse buttons
#define SLAM_MOUSE_BUTTON_1         0
#define SLAM_MOUSE_BUTTON_2         1
#define SLAM_MOUSE_BUTTON_3         2
#define SLAM_MOUSE_BUTTON_4         3
#define SLAM_MOUSE_BUTTON_5         4
#define SLAM_MOUSE_BUTTON_LEFT      SLAM_MOUSE_BUTTON_1
#define SLAM_MOUSE_BUTTON_RIGHT     SLAM_MOUSE_BUTTON_2
#define SLAM_MOUSE_BUTTON_MIDDLE    SLAM_MOUSE_BUTTON_3
#define SLAM_MOUSE_BUTTON_COUNT     8

// Actions
#define SLAM_RELEASE                0
#define SLAM_PRESS                  1
#define SLAM_REPEAT                 2

namespace slam {

class Window;

class InputManager {
public:
    InputManager();

    // Connect to window for callbacks
    void connect(Window& window);

    // Update (call at end of frame to update previous state)
    void update();

    // Keyboard state
    bool is_key_down(int key) const;
    bool is_key_pressed(int key) const;  // Just pressed this frame
    bool is_key_released(int key) const; // Just released this frame

    // Mouse buttons
    bool is_mouse_down(int button) const;
    bool is_mouse_pressed(int button) const;
    bool is_mouse_released(int button) const;

    // Mouse position
    vec2 mouse_position() const { return mouse_pos_; }
    vec2 mouse_delta() const { return mouse_delta_; }

    // Scroll
    vec2 scroll_delta() const { return scroll_delta_; }

    // Sensitivity
    void set_mouse_sensitivity(float sensitivity) { mouse_sensitivity_ = sensitivity; }
    float mouse_sensitivity() const { return mouse_sensitivity_; }

private:
    void on_key(int key, int scancode, int action, int mods);
    void on_mouse_button(int button, int action, int mods);
    void on_mouse_move(double x, double y);
    void on_scroll(double x, double y);

    std::array<bool, SLAM_KEY_COUNT> keys_current_{};
    std::array<bool, SLAM_KEY_COUNT> keys_previous_{};

    std::array<bool, SLAM_MOUSE_BUTTON_COUNT> mouse_current_{};
    std::array<bool, SLAM_MOUSE_BUTTON_COUNT> mouse_previous_{};

    vec2 mouse_pos_{0, 0};
    vec2 mouse_pos_previous_{0, 0};
    vec2 mouse_delta_{0, 0};

    vec2 scroll_delta_{0, 0};
    vec2 scroll_accumulated_{0, 0};

    float mouse_sensitivity_ = 0.002f;
    bool first_mouse_ = true;
};

} // namespace slam
