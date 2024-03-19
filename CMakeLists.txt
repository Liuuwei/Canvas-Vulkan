cmake_minimum_required(VERSION 3.22)

project(VulkanGame)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)
include_directories($ENV{VCPKG_PATH}/include)
include_directories(/home/liuwei/git/tinymuduo/include)


link_directories($ENV{VCPKG_PATH}/lib)
link_directories($ENV{VULKAN_SDK}/Lib)
link_directories(/home/liuwei/git/tinymuduo/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(src)