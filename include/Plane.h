#pragma once

#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>

class Plane : public Vertex {
public:
    VkVertexInputBindingDescription bindingDescription(uint32_t binding) const override {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = binding;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(Point);

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const override {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        attributeDescriptions[0].binding = binding;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].offset = offsetof(Point, position_);

        attributeDescriptions[1].binding = binding;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].offset = offsetof(Point, color_);

        attributeDescriptions[2].binding = binding;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].offset = offsetof(Point, texCoord_);

        return attributeDescriptions;
    }

    VkPrimitiveTopology topology() const override { 
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    struct Point {
        glm::vec2 position_;
        glm::vec3 color_;
        glm::vec2 texCoord_;
    };

    std::pair<std::vector<float>, std::vector<uint32_t>> vertices(float width, float height) const override {
        auto w2 = width / 2.0f, h2 = height / 2.0f;

        std::vector<float> vertices = {
            -w2, h2, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 
            w2, h2, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 
            -w2, -h2, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 
            w2, -h2, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 
        };

        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 1, 3
        };

        return {vertices, indices};
    }

private:

};