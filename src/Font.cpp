#include "Font.h"
#include "freetype/freetype.h"
#include "freetype/fttypes.h"

#include <cstdint>
#include <stdexcept>

Font::Font(const std::string& path, uint32_t size) {
    check(FT_Init_FreeType(&libarry_));
    
    check(FT_New_Face(libarry_, path.c_str(), 0, &face_));
    
    check(FT_Set_Pixel_Sizes(face_, 0, size));
}

void Font::loadChar(char c) {
    if (FT_Load_Char(face_, c, FT_LOAD_RENDER)) {
        throw std::runtime_error("faield to load char");
    }
}


void Font::renderMode(FT_Render_Mode mode) {
    check(FT_Render_Glyph(face_->glyph, mode));
}

void Font::check(FT_Error error) {
    if (error) {
        char msg[128];
        sprintf_s(msg, 128, "Fate: %s: line is %s", __FILE__, __LINE__);
        throw std::runtime_error(msg);
    }
}