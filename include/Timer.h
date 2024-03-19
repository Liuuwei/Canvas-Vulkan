#pragma once

#include <chrono>

class Timer {
public:
    Timer();
    void tick();
    float delta() const;

private:
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> prevTime_;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> currentTime_;
};