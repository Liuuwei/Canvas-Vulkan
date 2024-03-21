#include "Timer.h"
#include <chrono>

Timer::Timer() : prevTime_(std::chrono::high_resolution_clock::now()), currentTime_(std::chrono::high_resolution_clock::now()) {

}

void Timer::tick() {
    prevTime_ = currentTime_;
    currentTime_ = std::chrono::high_resolution_clock::now();
}

unsigned long long Timer::deltaMilliseconds() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime_ - prevTime_).count();
}

unsigned long long Timer::nowMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}