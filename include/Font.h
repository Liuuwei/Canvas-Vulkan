#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

#include "Plane.h"
#include "Image.h"
#include "Vertex.h"
#include "vulkan/vulkan_core.h"
#include "Timer.h"

#include <ft2build.h>
#include <utility>
#include FT_FREETYPE_H

class Font : public Plane {  
public:
    Font(const std::string& path, uint32_t size);

    VkVertexInputBindingDescription bindingDescription(uint32_t binding) const override {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = binding;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(Point);

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescription(uint32_t binding) const override {
        auto attributes = Plane::attributeDescription(binding);

        VkVertexInputAttributeDescription attributeDescription{};
        attributeDescription.binding = binding;
        attributeDescription.location = attributes.size();
        attributeDescription.format = VK_FORMAT_R32_UINT;
        attributeDescription.offset = offsetof(Point, index_);
        attributes.push_back(attributeDescription);

        return attributes;
    }

    struct Point {
        Point(float x, float y, float r, float g, float b, float u, float v, uint32_t index = 0) :
            position_(x, y), color_(r, g, b), texCoord_(u, v), index_(index) {}

        Point(glm::vec2 position, glm::vec3 color, glm::vec2 texCoord, uint32_t index = 0) : 
            position_(position), color_(color), texCoord_(texCoord), index_(index) {}

        Point(Plane::Point point, uint32_t index) : 
            position_(point.position_), color_(point.color_), texCoord_(point.texCoord_), index_(index) {}

        Point() {}

        glm::vec2 position_{};
        glm::vec3 color_{};
        glm::vec2 texCoord_{};
        uint32_t index_{};
    };

    struct Character {
        Character() {}
        char char_{};
        // left top point
        int offsetX_{};
        int offsetY_{};
        uint32_t width_{};
        uint32_t height_{};
        int advance_{};
        uint32_t index_{};
        glm::vec3 color_ = glm::vec3(0.0f, 0.0f, 0.0f);
        std::shared_ptr<Image> image_;
    };

    static std::pair<std::vector<Point>, std::vector<uint32_t>> vertices(float x, float y, const Character& character, glm::vec3 color) {
        auto width = character.width_, height = character.height_;
        
        auto index = character.index_;

        auto w2 = width / 2.0f, h2 = height / 2.0f;

        std::vector<Font::Point> vertices = {
            {x - w2, y + h2, color.x, color.y, color.z, 0.0f, 0.0f, index}, 
            {x + w2, y + h2, color.x, color.y, color.z,  1.0f, 0.0f, index}, 
            {x - w2, y - h2, color.x, color.y, color.z, 0.0f, 1.0f, index}, 
            {x + w2, y - h2, color.x, color.y, color.z, 1.0f, 1.0f, index}, 
        };

        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 1, 3
        };

        return {vertices, indices};
    }

    #ifndef defVertices
    #define defVertices(vertices, indices, x, y, character, color) \
    { \
        auto w2 = character.width_ / 2.0f, h2 = character.height_ / 2.0f; \
        auto index = character.index_; \
      \
        vertices = { \
            {x - w2, y + h2, color.x, color.y, color.z, 0.0f, 0.0f, index}, \
            {x + w2, y + h2, color.x, color.y, color.z,  1.0f, 0.0f, index}, \
            {x - w2, y - h2, color.x, color.y, color.z, 0.0f, 1.0f, index}, \
            {x + w2, y - h2, color.x, color.y, color.z, 1.0f, 1.0f, index}, \
        }; \
      \
        indices = { \
            0, 1, 2, 2, 1, 3 \
        }; \
    }
    #endif

    // typedef std::pair<std::vector<Point>, std::vector<uint32_t>> PairPointIndex;
    // static PairPointIndex merge(PairPointIndex p1, PairPointIndex p2) {
    //     auto size = p1.first.size();

    //     std::for_each(p2.second.begin(), p2.second.end(), [size](auto& v) {
    //         v += size;
    //     });

    //     p1.first.insert(p1.first.end(), p2.first.begin(), p2.first.end());
    //     p1.second.insert(p1.second.end(), p2.second.begin(), p2.second.end());

    //     return p1;
    // }

    #ifndef mergeVertices
    #define mergeVertices(p1, p2) \
    { \
            auto size = p1.first.size(); \
            std::for_each(p2.second.begin(), p2.second.end(), [size](auto& v) { \
                v += size; \
            }); \
                \
            p1.first.insert(p1.first.end(), p2.first.begin(), p2.first.end()); \
            p1.second.insert(p1.second.end(), p2.second.begin(), p2.second.end()); \
    }
    #endif

    static std::pair<std::vector<Font::Point>, std::vector<uint32_t>> generateTextLine(float x, float y, const std::string& line, const std::unordered_map<char, Character>& dictionary) {
        auto s = Timer::nowMilliseconds();
        std::pair<std::vector<Font::Point>, std::vector<uint32_t>> result;
        unsigned long long v = 0, m = 0;
        bool isSpace_ = false;
        for (size_t i = 0; i < line.size(); i++) {
            glm::vec2 center;
            center.x = x + dictionary.at(line[i]).offsetX_ + dictionary.at(line[i]).width_ / 2.0f;
            center.y = y + dictionary.at(line[i]).offsetY_ - dictionary.at(line[i]).height_ / 2.0f;
            // auto s = Timer::nowMilliseconds();
            auto t = Font::vertices(center.x, center.y, dictionary.at(line[i]), dictionary.at(line[i]).color_);
            // auto e = Timer::nowMilliseconds();
            // v += e - s;

            
            // s = Timer::nowMilliseconds();
            mergeVertices(result, t);
            // e = Timer::nowMilliseconds();
            // m += e - s;

            x += dictionary.at(line[i]).advance_;
        }
        // std::cout << std::format("[once] vertices ms: {}, merge ms: {}\n", v, m);

        return result;
    }

    static std::pair<std::vector<Font::Point>, std::vector<uint32_t>> generateTextLines(float x, float y, const std::vector<std::string>& lines, const std::unordered_map<char, Character>& dictionary, uint32_t lineWidth) {
        std::pair<std::vector<Font::Point>, std::vector<uint32_t>> result;

        unsigned long long g = 0, m = 0;
        for (auto& line : lines) {
            // auto s = Timer::nowMilliseconds();
            auto pointAndIndex = Font::generateTextLine(x, y, line, dictionary);
            // auto e = Timer::nowMilliseconds();
            // g += e - s;
            
            mergeVertices(result, pointAndIndex);
            // s = Timer::nowMilliseconds();
            // m += s - e;

            y -= lineWidth;
        }

        // std::cout << std::format("generate ms: {}, merge ms: {}\n", g, m);

        return result;
    }

    void loadChar(char c);
    void renderMode(FT_Render_Mode mode);
    FT_Bitmap bitmap() { return face_->glyph->bitmap; }
    FT_GlyphSlot glyph() { return face_->glyph; }
private:
    void check(FT_Error error);

    FT_Library libarry_;
    FT_Face face_;
};