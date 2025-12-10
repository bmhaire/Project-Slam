/**
 * Slam Engine - Timer Utility
 *
 * High-resolution timing for frame delta and profiling
 */

#pragma once

#include <chrono>

namespace slam {

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double>;

    Timer() : start_time_(Clock::now()), last_time_(start_time_) {}

    // Reset the timer
    void reset() {
        start_time_ = Clock::now();
        last_time_ = start_time_;
    }

    // Get time elapsed since construction or last reset (in seconds)
    double elapsed() const {
        return Duration(Clock::now() - start_time_).count();
    }

    // Get time elapsed since last call to delta() (in seconds)
    double delta() {
        TimePoint now = Clock::now();
        double dt = Duration(now - last_time_).count();
        last_time_ = now;
        return dt;
    }

    // Get time elapsed since last call to delta() without updating last_time_
    double peek_delta() const {
        return Duration(Clock::now() - last_time_).count();
    }

private:
    TimePoint start_time_;
    TimePoint last_time_;
};

// Frame timing helper
class FrameTimer {
public:
    FrameTimer() : frame_count_(0), fps_(0.0), frame_time_(0.0), accumulated_time_(0.0) {}

    // Call at the start of each frame
    void begin_frame() {
        frame_delta_ = timer_.delta();
        accumulated_time_ += frame_delta_;
        frame_count_++;

        // Update FPS every second
        if (accumulated_time_ >= 1.0) {
            fps_ = static_cast<double>(frame_count_) / accumulated_time_;
            frame_time_ = accumulated_time_ / static_cast<double>(frame_count_);
            frame_count_ = 0;
            accumulated_time_ = 0.0;
        }
    }

    // Get delta time for current frame (in seconds)
    double delta_time() const { return frame_delta_; }

    // Get delta time as float (commonly used in game code)
    float delta_time_f() const { return static_cast<float>(frame_delta_); }

    // Get frames per second (updated every second)
    double fps() const { return fps_; }

    // Get average frame time in milliseconds (updated every second)
    double frame_time_ms() const { return frame_time_ * 1000.0; }

    // Get total elapsed time since construction
    double total_time() const { return timer_.elapsed(); }

private:
    Timer timer_;
    double frame_delta_ = 0.0;
    double fps_;
    double frame_time_;
    double accumulated_time_;
    uint64_t frame_count_;
};

// Scoped timer for profiling
class ScopedTimer {
public:
    ScopedTimer(const char* name) : name_(name) {
        start_ = Timer::Clock::now();
    }

    ~ScopedTimer() {
        auto end = Timer::Clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start_).count();
        // Could log this somewhere, for now just compute it
        (void)ms;
        (void)name_;
    }

private:
    const char* name_;
    Timer::TimePoint start_;
};

} // namespace slam
