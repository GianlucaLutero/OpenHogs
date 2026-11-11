// TextLayout.h
//
// Turns text into positioned characters of a bitmap font, and fills in the
// placeholders of the game's message strings.
//
// Text is UTF-8, as everywhere in OpenHogs. It is converted to the game's
// character codes (formats::encodeGameText) to find each character in the
// font, so accented letters of the translations ("MODALITÀ") work as long
// as the font has them. Characters the game cannot show become '?', so a
// stray character in a translation never stops the game.
//
// Nothing here draws: the renderer takes the quads and draws them.

#pragma once

#include "openhogs/formats/BitmapFont.h"

#include <string>
#include <string_view>
#include <vector>

namespace openhogs::ui {

// One character to draw: where it comes from in the atlas and where it goes
// on screen (top-left corner, in the same units as the layout origin).
struct GlyphQuad {
    formats::GlyphRect source;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

// Character codes for `text`, with '?' for anything the game cannot show.
std::vector<std::uint8_t> encodeForDisplay(std::string_view text);

// Width of `text` drawn with `font` at `scale` (1 = the font's pixels).
float measureText(const formats::BitmapFont& font, std::string_view text, float scale = 1.0f);

// The quads of `text` on one line, starting at (x, y) = its top-left
// corner. Spaces and codes the font lacks advance by font.spaceWidth.
std::vector<GlyphQuad> layoutText(const formats::BitmapFont& font, std::string_view text, float x, float y,
                                  float scale = 1.0f);

// Fills the placeholders of the game's messages, e.g.
//     "VICTORY TO >S!!"      with {"BRITISH"}  -> "VICTORY TO BRITISH!!"
//     "MISSION >2N : >S"     numbers {3}, strings {"THE BRIDGE"}
//                                              -> "MISSION 03 : THE BRIDGE"
// ">S" takes the next string, ">N" the next number; a digit between them
// (">2N") pads the number with zeros to that many digits. Placeholders
// without a value are left out, so a missing name never shows as ">S".
std::string fillPlaceholders(std::string_view text, const std::vector<std::string>& strings,
                             const std::vector<int>& numbers = {});

} // namespace openhogs::ui
