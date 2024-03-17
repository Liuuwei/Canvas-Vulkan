#pragma once

#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class Triangle : public Vertex {
public:
    ~Triangle() override {
        
    }
    VkVertexInputBindingDescription bindingDescription(uint32_t binding) const override {
        VkVertexInputBindingDescription inputBinding{};
        inputBinding.binding = binding;
        inputBinding.stride = sizeof(Point);
        inputBinding.inputRate =  VK_VERTEX_INPUT_RATE_VERTEX;

        return inputBinding;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const override {
        std::vector<VkVertexInputAttributeDescription> attributes(3);
        attributes[0].binding = binding;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[0].offset = offsetof(Point, position_);

        attributes[1].binding = binding;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = offsetof(Point, color_);

        attributes[2].binding = binding;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[2].offset = offsetof(Point, texCoord_);

        return attributes;        
    }

    VkPrimitiveTopology topology() const override {
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    std::pair<std::vector<float>, std::vector<uint32_t>> vertices(float width, float height) const override {
        auto w2 = width / 2.0f, h2 = height / 2.0f;

        std::vector<float> vertices = {
            -w2, h2, 1.0f, 1.0f, 1.0f, 
            w2, h2, 1.0f, 1.0f, 1.0f, 
            -w2, -h2, 1.0f, 1.0f, 1.0f,             
        };

        std::vector<uint32_t> indices = {
            0, 1, 2
        };

        return {vertices, indices};
    }

    struct Point {
        glm::vec2 position_;
        glm::vec3 color_;
        glm::vec2 texCoord_;
    };

private:

};