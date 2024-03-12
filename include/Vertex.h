#pragma once

#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vector>

class Vertex {
public:
    virtual VkVertexInputBindingDescription bindingDescription(uint32_t binding) const = 0;
    virtual std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const = 0;
    virtual VkPrimitiveTopology topology() const;
private:
};