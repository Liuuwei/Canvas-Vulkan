cmake_minimum_required(VERSION 3.22)

project(VulkanGame)

set(CMAKE_CXX_COMPILER $ENV{MSYS2}/ucrt64/bin/clang++.exe)
set(CMAKE_CXX_STANDARD 20)

include_directories(include)
include_directories($ENV{VCPKG_INCLUDE})
include_directories($ENV{VULKAN_SDK}/Include)

link_directories($ENV{VCPKG_LIB})
link_directories($ENV{VULKAN_SDK}/Lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(src)