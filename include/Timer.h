#pragma once

#include <chrono>

class Timer {
public:
    Timer();

    void tick();
    unsigned long long deltaMilliseconds() const;
    static unsigned long long nowMilliseconds();
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> prevTime_;
    std::chrono::time_point<std::chrono::high_resolution_clock> currentTime_;
};