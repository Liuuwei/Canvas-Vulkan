cmake_minimum_required(VERSION 3.22)

project(VulkanGame)

set(CMAKE_C_COMPILER $ENV{LLVM}/bin/clang.exe)
set(CMAKE_CXX_COMPILER $ENV{LLVM}/bin/clang++.exe)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories($ENV{VCPKG_INCLUDE})
include_directories($ENV{VULKAN_SDK}/Include)

link_directories($ENV{VCPKG_LIB})
link_directories($ENV{VULKAN_SDK}/Lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(src)