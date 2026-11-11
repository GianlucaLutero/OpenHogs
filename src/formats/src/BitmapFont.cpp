// BitmapFont.cpp - see BitmapFont.h for an overview.

#include "openhogs/formats/BitmapFont.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/BitmapImage.h"

#include <algorithm>
#include <climits>
#include <string>

namespace openhogs::formats {

namespace {

constexpr std::size_t kEntrySize = 8;

// The original's space width is not stored in the files; a third of the
// line height looks right with every font (docs/open-decisions.md).
constexpr int kSpaceWidthDivisor = 3;

} // namespace

GlyphRect BitmapFont::glyph(std::uint8_t code) const {
    return code < glyphs.size() ? glyphs[code] : GlyphRect{};
}

std::vector<GlyphRect> parseFontTable(std::span<const std::byte> data) {
    if (data.size() % kEntrySize != 0) {
        throw core::FormatError("Font table: size " + std::to_string(data.size()) + " is not a multiple of 8");
    }
    core::BinaryReader reader(data);
    std::vector<GlyphRect> glyphs(data.size() / kEntrySize);
    for (auto& glyph : glyphs) {
        glyph.x = reader.readI16();
        glyph.y = reader.readI16();
        glyph.width = reader.readI16();
        glyph.height = reader.readI16();
    }
    return glyphs;
}

BitmapFont loadBitmapFont(std::span<const std::byte> table, std::span<const std::byte> bitmap) {
    BitmapFont font;
    font.glyphs = parseFontTable(table);
    font.atlas = decodeBitmap(bitmap, BitmapTransparency::PaletteIndex0);

    // Move from video memory positions to atlas positions.
    int originX = INT_MAX;
    int originY = INT_MAX;
    for (const auto& glyph : font.glyphs) {
        if (!glyph.empty()) {
            originX = std::min(originX, glyph.x);
            originY = std::min(originY, glyph.y);
        }
    }
    for (auto& glyph : font.glyphs) {
        if (glyph.empty()) {
            glyph = GlyphRect{};
            continue;
        }
        glyph.x -= originX;
        glyph.y -= originY;
        if (glyph.x + glyph.width > static_cast<int>(font.atlas.width) ||
            glyph.y + glyph.height > static_cast<int>(font.atlas.height)) {
            throw core::FormatError("Font: a character lies outside the " + std::to_string(font.atlas.width) + "x" +
                                    std::to_string(font.atlas.height) + " picture");
        }
        font.lineHeight = std::max(font.lineHeight, glyph.height);
    }
    font.spaceWidth = std::max(1, font.lineHeight / kSpaceWidthDivisor);
    return font;
}

} // namespace openhogs::formats
