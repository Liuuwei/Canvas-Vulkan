#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

#include <string>

class ShaderModule {
public:
    ShaderModule(VkDevice device, const std::string& path);

    VkShaderModule get() const { return shader_; }
private:
    VkDevice device_;
    VkShaderModule shader_;
};