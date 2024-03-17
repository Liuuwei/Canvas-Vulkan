#include "Timer.h"
#include <chrono>

Timer::Timer() : prevTime_(std::chrono::high_resolution_clock::now()), currentTime_(std::chrono::high_resolution_clock::now()) {

}

void Timer::tick() {
    prevTime_ = currentTime_;
    currentTime_ = std::chrono::high_resolution_clock::now();
}

float Timer::delta() const {
    return std::chrono::duration<float, std::chrono::seconds::period>(currentTime_ - prevTime_).count();
}