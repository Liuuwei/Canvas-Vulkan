#pragma once

#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <glm/glm.hpp>
#include <cstdint>

class Block : public Vertex {
public:
    ~Block() {}

    VkVertexInputBindingDescription bindingDescription(uint32_t binding) const override {
        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding = binding;
        inputBinding.stride = sizeof(Point);
        inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return inputBinding;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const override {
        std::vector<VkVertexInputAttributeDescription> attributes(3);
        attributes[0].binding = binding;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Point, position_);

        attributes[1].binding = binding;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = offsetof(Point, color_);

        attributes[2].binding = binding;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[2].offset = offsetof(Point, texCoord_);

        return attributes;
    }

    VkPrimitiveTopology topology() const override {
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    struct Point {
        glm::vec3 position_;
        glm::vec3 color_;
        glm::vec3 texCoord_;
    };

private:

};