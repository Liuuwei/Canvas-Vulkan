#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "Plane.h"
#include "Image.h"
#include "Vertex.h"
#include "vulkan/vulkan_core.h"

#include <ft2build.h>
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
        // x += character.offsetX_;
        // y += character.offsetY_;
        // x += character.width_ / 2.0f;
        // y -= character.height_ / 2.0f;

        auto width = character.width_, height = character.height_;
        
        auto index = character.index_;

        auto t = Plane::vertices(x, y, width, height, color);
        std::vector<Point> points;
        for (size_t i = 0; i < t.first.size(); i++) {
            points.emplace_back(Point(t.first[i], index));
        }

        return {points, t.second};
    }

    typedef std::pair<std::vector<Point>, std::vector<uint32_t>> PairPointIndex;
    static PairPointIndex mergeVertices(PairPointIndex p1, PairPointIndex p2) {
        auto size = p1.first.size();

        for (size_t i = 0; i < p2.first.size(); i++) {
            p1.first.push_back(p2.first[i]);
        }
        for (size_t i = 0; i < p2.second.size(); i++) {
            p1.second.push_back(p2.second[i] + size);
        }

        return p1;
    }

    static PairPointIndex generateText(float x, float y, const std::string& text, const std::unordered_map<char, Character>& dictionary) {
        PairPointIndex pointAndIndex;
        for (auto& c : text) {
            glm::vec2 center;
            center.x = x + dictionary.at(c).offsetX_ + dictionary.at(c).width_ / 2.0f;
            center.y = y + dictionary.at(c).offsetY_ - dictionary.at(c).height_ / 2.0f;
            auto t = Font::vertices(center.x, center.y, dictionary.at(c), dictionary.at(c).color_);
            
            pointAndIndex = Font::mergeVertices(pointAndIndex, t);

            x += dictionary.at(c).advance_;
        }

        return pointAndIndex;
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