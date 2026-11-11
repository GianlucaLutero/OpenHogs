// BitmapFont.h
//
// The bitmap fonts of the PC version (FEText/): a .tab file that says
// where each character is, and a palettized .BMP picture holding all the
// characters (the "atlas"). See docs/formats/fonts.md.
//
// The .tab file is a list of 8-byte entries, one per character code of
// the game's text encoding (see GameText.h), so entry 0x22 is 'A':
//
//     int16 x, y          position of the character in video memory
//     int16 width, height size in pixels (0 x 0: the code has no character)
//
// The positions are where the PlayStation version kept the picture in its
// video memory, not positions inside the .BMP. The picture starts at the
// smallest x and y used by any character, so subtracting that corner turns
// them into atlas coordinates. (Checked on all the fonts of the game: the
// characters then line up exactly with the picture.)
//
// Codes without a character include the space (code 0x01): the game
// advances by a fixed amount instead, see BitmapFont::spaceWidth.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

// Where one character is in the atlas, in pixels.
struct GlyphRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool empty() const { return width <= 0 || height <= 0; }
};

struct BitmapFont {
    RgbaImage atlas;                // palette entry 0 is transparent
    std::vector<GlyphRect> glyphs;  // indexed by character code
    int lineHeight = 0;             // tallest character
    int spaceWidth = 0;             // advance of a space and of unknown codes

    // The character for a code, or an empty rectangle.
    GlyphRect glyph(std::uint8_t code) const;
};

// Reads a .tab file as it is stored (video memory positions). Throws
// core::FormatError if the size is not a whole number of entries.
std::vector<GlyphRect> parseFontTable(std::span<const std::byte> data);

// Builds a font from its .tab and .BMP files. Throws core::FormatError if
// a character lies outside the picture.
BitmapFont loadBitmapFont(std::span<const std::byte> table, std::span<const std::byte> bitmap);

} // namespace openhogs::formats
