#pragma once

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

class Editor {
public:
    enum Mode {
        Command, 
        Input, 
    };

    enum Direction {
        Up = GLFW_KEY_UP, 
        Down = GLFW_KEY_DOWN, 
        Right = GLFW_KEY_RIGHT, 
        Left = GLFW_KEY_LEFT, 
    };

    Editor(uint32_t width, uint32_t height, uint32_t lineHeight);

    Mode mode() const;
    void enter();
    void backspace();
    void insertChar(char c);
    void insertStr(const std::string& str);
    void delteChar();
    void moveCursor(Direction dir);
    void moveLimit();
    bool lineEmpty(const std::string& line);
    void addLineNumber(std::string& line, uint32_t lineNumber);
    void adjust(uint32_t width, uint32_t height);
public:
    struct Limit {
        uint32_t up_ = 0;
        uint32_t bottom_ = 1;
    };

    Mode mode_ = Command;
    uint32_t lineHeight_;
    uint32_t currLine_ = 0;
    std::vector<std::string> lines_;
    glm::uvec2 cursorPos_ = {0, 0};
    glm::uvec2 screen_;
    uint32_t showLines_;
    Limit limit_{};
    uint32_t lineNumberOffset_ = 5;
    unsigned long long wordCount_ = 0;
};