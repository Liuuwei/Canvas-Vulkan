#include "Vulkan.h"
#include <exception>
#include <iostream>

int main() {
    try {
        Vulkan vulkan("Game", 800, 600);
        vulkan.run();
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }
}