#include "Editor.h"
#include "glm/fwd.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>

Editor::Editor(uint32_t width, uint32_t height, uint32_t lineHeight) : screen_(width, height), lineHeight_(lineHeight), showLines_(height / lineHeight) {
    lines_.resize(1);
    addLineNumber(lines_[0], 1);
    wordCount_ += 5;
}

Editor::Mode Editor::mode() const {
    return mode_;
}

void Editor::enter() {
    auto& currLine = lines_[cursorPos_.y];
    auto newLine = std::string(currLine.begin() + cursorPos_.x + lineNumberOffset_, currLine.end());

    currLine.erase(currLine.begin() + cursorPos_.x + lineNumberOffset_, currLine.end());

    cursorPos_.y++;
    cursorPos_.x = 0;
    addLineNumber(newLine, cursorPos_.y + 1);
    lines_.insert(lines_.begin() + cursorPos_.y, newLine);

    wordCount_ += 5;

    moveLimit();
}

void Editor::backspace() {
    auto& currLine = lines_[cursorPos_.y];

    if (cursorPos_.x == 0 && cursorPos_.y == 0) {
        return ;
    }
    
    if (lineEmpty(currLine)) {
        lines_.erase(lines_.begin() + cursorPos_.y);
        cursorPos_.y--;
        cursorPos_.x = lines_[cursorPos_.y].size() - lineNumberOffset_;
    } else {
        delteChar();
    }

    moveLimit();
}

void Editor::insertChar(char c) {
    if (cursorPos_.y >= lines_.size()) {
        lines_.push_back({});
    }
    
    auto& currLine = lines_[cursorPos_.y];

    currLine.insert(currLine.begin() + cursorPos_.x + lineNumberOffset_, c);
    cursorPos_.x++;

    wordCount_++;

    moveLimit();
}

void Editor::insertStr(const std::string& str) {
    for (size_t i = 0; i < str.size(); i++) {
        insertChar(str[i]);
    }
}

void Editor::delteChar() {
    auto& currLine = lines_[cursorPos_.y];

    if (cursorPos_.x == 0) {
        lines_[cursorPos_.y - 1] += currLine;
        lines_.erase(lines_.begin() + cursorPos_.y);
        cursorPos_.y--;
        cursorPos_.x = lines_[cursorPos_.y].size();
    } else {
        currLine.erase(currLine.begin() + (cursorPos_.x + lineNumberOffset_ - 1));
        cursorPos_.x--;
    }

    wordCount_--;

    moveLimit();
}

void Editor::moveCursor(Editor::Direction dir) {
    switch (dir) {
    case Up:
        if (cursorPos_.y == 0) {
            return ;
        }
        cursorPos_.y--;
        cursorPos_.x = std::min(cursorPos_.x, static_cast<uint32_t>(lines_[cursorPos_.y].size() - lineNumberOffset_));
        break;
    case Down:
        if (cursorPos_.y >= lines_.size() - 1) {
            return ;
        }
        cursorPos_.y++;
        cursorPos_.x = std::min(cursorPos_.x, static_cast<uint32_t>(lines_[cursorPos_.y].size() - lineNumberOffset_));
        break;
    case Right:
        if (cursorPos_.x + lineNumberOffset_ >= lines_[cursorPos_.y].size()) {
            return ;
        }
        cursorPos_.x++;
        break;
    case Left:
        if (cursorPos_.x <= 0) {
            return ;
        }
        cursorPos_.x--;
        break;
    }

    moveLimit();
}

void Editor::moveLimit() {
    if (cursorPos_.y < limit_.up_) {
        limit_.up_ = cursorPos_.y;
        if (limit_.bottom_ - limit_.up_ > showLines_) {
            limit_.bottom_ = limit_.up_ + showLines_;
        }
    } else if (cursorPos_.y >= limit_.bottom_) {
        limit_.bottom_ = cursorPos_.y + 1;
        if (limit_.bottom_ - limit_.up_ > showLines_) {
            limit_.up_ = limit_.bottom_ - showLines_;
        }
    }

    limit_.bottom_ = std::min(limit_.bottom_, static_cast<uint32_t>(lines_.size()));
}

bool Editor::lineEmpty(const std::string& line) {
    return line.size() - lineNumberOffset_ <= 0;
}

void Editor::addLineNumber(std::string& line, uint32_t lineNumber) {
    char s[10];
    snprintf(s, 10, "%4d ", lineNumber);
    line = s;
}

void Editor::adjust(uint32_t width, uint32_t height) {
    screen_ = glm::uvec2(width, height);

    auto updateSize = static_cast<float>(height) / static_cast<float>(lineHeight_) / static_cast<float>(showLines_);
    showLines_ = height / lineHeight_;

    float upSize = cursorPos_.y - limit_.up_;
    upSize *= updateSize;

    limit_.up_ = std::max(static_cast<int>(cursorPos_.x) - static_cast<int>(upSize), static_cast<int>(0));

    limit_.bottom_ = limit_.up_ + showLines_;
    limit_.bottom_ = std::min(limit_.bottom_, static_cast<uint32_t>(lines_.size()));
}