#pragma once

#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <iostream>

class Line : public Vertex {
public:
    VkVertexInputBindingDescription bindingDescription(uint32_t binding) const override {
        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = binding;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(Point);

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const override {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
        int location = 0;

        attributeDescriptions[0].binding = binding;
        attributeDescriptions[0].location = location++;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Point, position_);

        attributeDescriptions[1].binding = binding;
        attributeDescriptions[1].location = location++;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Point, color_);

        attributeDescriptions[2].binding = binding;
        attributeDescriptions[2].location = location++;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Point, texCoord_);

        return attributeDescriptions;
    }

    VkPrimitiveTopology topology() const override {
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    
    struct Point {
        glm::vec2 position_;
        glm::vec4 color_;
        glm::vec2 texCoord_;
    };

    std::pair<std::vector<Point>, std::vector<uint32_t>> initVertices(int width, int height) {
        std::vector<Point> vertices;
        std::vector<uint32_t> indices;

        int a = 0;
        for (int i = -width / 2; i <= width / 2; i++) {
            for (int j = -height / 2; j <= height / 2; j++) {
                Point point{};
                point.position_ = glm::vec2(i, j);
                point.color_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                vertices.push_back(point);
                indices.push_back(a++);
            }
        }

        return {vertices, indices};
    }
private:

};