#include "Font.h"
#include "freetype/config/integer-types.h"
#include "freetype/freetype.h"
#include "freetype/fttypes.h"

#include <stdexcept>

Font::Font(const std::string& path) {
    check(FT_Init_FreeType(&libarry_));
    
    check(FT_New_Face(libarry_, path.c_str(), 0, &face_));
    
    // check(FT_Set_Char_Size(face_, 0, 16 * 64, 300, 300));

    check(FT_Set_Pixel_Sizes(face_, 0, 48));
}

void Font::loadChar(char c) {
    if (FT_Load_Char(face_, c, FT_LOAD_RENDER)) {
        throw std::runtime_error("faield to load char");
    }
}


void Font::renderMode(FT_Render_Mode mode) {
    check(FT_Render_Glyph(face_->glyph, mode));
}

FT_Bitmap Font::bitmap() {
    return face_->glyph->bitmap;
}

FT_UInt Font::getCharIndex(FT_ULong charcode) {
    auto index =  FT_Get_Char_Index(face_, charcode);
    if (index == 0) {
        throw std::runtime_error("FT failed to get char index");
    }
    return index;
}

void Font::loadGlyph(FT_UInt index, FT_Int32 flags) {
    check(FT_Load_Glyph(face_, index, flags));
}

void Font::check(FT_Error error) {
    if (error) {
        char msg[128];
        sprintf_s(msg, 128, "Fate: %s: line is %s", __FILE__, __LINE__);
        throw std::runtime_error(msg);
    }
}