add_library(MyVulkan Vulkan.cpp)

target_link_libraries(MyVulkan vulkan-1 glfw3dll)

add_executable(Main main.cpp)
target_link_libraries(Main MyVulkan vulkan-1 glfw3dll)