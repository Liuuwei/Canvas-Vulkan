#pragma once

#include <chrono>

class Timer {
public:
    Timer();
    void tick();
    float delta() const;

private:
    std::chrono::time_point<std::chrono::steady_clock> prevTime_;
    std::chrono::time_point<std::chrono::steady_clock> currentTime_;
};