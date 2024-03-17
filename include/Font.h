#pragma once

#include "freetype/fttypes.h"
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H

class Font {
public:
    Font(const std::string& path);

    void loadChar(char c);
    void renderMode(FT_Render_Mode mode);
    FT_Bitmap bitmap();
public:
    FT_UInt getCharIndex(FT_ULong c);
    void loadGlyph(FT_UInt index, FT_Int32 flags = FT_LOAD_DEFAULT);
    void check(FT_Error error);

    FT_Library libarry_;
    FT_Face face_;
    
};