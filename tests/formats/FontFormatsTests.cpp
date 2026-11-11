// Tests for formats::decodeBitmap (.BMP pictures) and formats::BitmapFont
// (FEText .tab + .BMP fonts).
//
// Every text of the menus and the HUD is drawn from these pictures. A
// mistake in row order, nibble order or transparency garbles or boxes in
// every letter; a wrong atlas origin shows the neighbouring letter instead
// of the right one.

#include "openhogs/formats/BitmapFont.h"
#include "openhogs/formats/BitmapImage.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;

namespace {

using Colour = std::array<std::uint8_t, 3>;

// A BMP file with `bits` per pixel, a palette (for 4 and 8 bits) and the
// given rows of already-packed pixel bytes, stored as they appear in the
// file (the first row given is the first row stored). `rowBytes` must
// include the padding to 4 bytes.
std::vector<std::byte> makeBitmap(int width, int height, std::uint16_t bits, const std::vector<Colour>& palette,
                                  const std::vector<std::vector<std::uint8_t>>& storedRows,
                                  std::uint32_t compression = 0) {
    const auto pixelOffset = static_cast<std::uint32_t>(14 + 40 + palette.size() * 4);
    ByteBuilder bmp;
    bmp.raw("BM").u32(0).u32(0).u32(pixelOffset);
    bmp.u32(40).i32(width).i32(height).u16(1).u16(bits).u32(compression);
    bmp.u32(0).u32(2834).u32(2834).u32(static_cast<std::uint32_t>(palette.size())).u32(0);
    for (const auto& colour : palette) {
        bmp.u8(colour[2]).u8(colour[1]).u8(colour[0]).u8(0);  // stored blue, green, red
    }
    for (const auto& row : storedRows) {
        for (const auto byte : row) bmp.u8(byte);
    }
    return bmp.bytes();
}

std::array<std::uint8_t, 4> pixelAt(const RgbaImage& image, std::uint32_t x, std::uint32_t y) {
    const auto at = (std::size_t{y} * image.width + x) * 4;
    return {image.pixels[at], image.pixels[at + 1], image.pixels[at + 2], image.pixels[at + 3]};
}

const Colour kBlack{0, 0, 0};
const Colour kWhite{255, 255, 255};
const Colour kRed{200, 10, 20};

} // namespace

TEST_CASE("4-bit bitmaps are stored bottom-up with the left pixel in the high nibble", "[formats][bmp]") {
    // The fonts are 4-bit BMPs: getting either the row order or the nibble
    // order wrong mirrors every letter.
    // Stored rows: bottom row first. Bottom = (white, red), top = (red, black).
    const auto file = makeBitmap(2, 2, 4, {kBlack, kWhite, kRed}, {{0x12, 0, 0, 0}, {0x20, 0, 0, 0}});
    const auto image = decodeBitmap(file);

    REQUIRE(image.width == 2);
    REQUIRE(image.height == 2);
    CHECK(pixelAt(image, 0, 0) == std::array<std::uint8_t, 4>{200, 10, 20, 255});
    CHECK(pixelAt(image, 1, 0) == std::array<std::uint8_t, 4>{0, 0, 0, 255});
    CHECK(pixelAt(image, 0, 1) == std::array<std::uint8_t, 4>{255, 255, 255, 255});
    CHECK(pixelAt(image, 1, 1) == std::array<std::uint8_t, 4>{200, 10, 20, 255});
}

TEST_CASE("Palette entry 0 can be made transparent", "[formats][bmp]") {
    // The fonts' background is palette entry 0 (black in some, magenta in
    // others); it must vanish, while black letter outlines (another entry)
    // must stay.
    const std::vector<Colour> palette{kBlack, kBlack};
    const auto file = makeBitmap(2, 1, 8, palette, {{0, 1, 0, 0}});

    const auto transparent = decodeBitmap(file, BitmapTransparency::PaletteIndex0);
    CHECK(pixelAt(transparent, 0, 0)[3] == 0);
    CHECK(pixelAt(transparent, 1, 0)[3] == 255);

    const auto opaque = decodeBitmap(file);
    CHECK(pixelAt(opaque, 0, 0)[3] == 255);
}

TEST_CASE("24-bit bitmaps with a negative height are stored top-down", "[formats][bmp]") {
    // Both row orders exist in the wild; replacement pictures from a mod
    // tool may well be top-down.
    const auto file = makeBitmap(1, -2, 24, {}, {{30, 20, 10, 0}, {3, 2, 1, 0}});
    const auto image = decodeBitmap(file);
    REQUIRE(image.height == 2);
    CHECK(pixelAt(image, 0, 0) == std::array<std::uint8_t, 4>{10, 20, 30, 255});
    CHECK(pixelAt(image, 0, 1) == std::array<std::uint8_t, 4>{1, 2, 3, 255});
}

TEST_CASE("Compressed bitmaps are rejected instead of decoded as garbage", "[formats][bmp]") {
    // RLE pictures are not used by the game; a replacement saved that way
    // must give a clear error.
    const auto file = makeBitmap(2, 1, 8, {kBlack}, {{0, 0, 0, 0}}, 1);
    CHECK_THROWS_AS(decodeBitmap(file), FormatError);
}

TEST_CASE("Font positions are moved from video memory to the picture's corner", "[formats][font]") {
    // The .tab stores where the PlayStation kept the picture in video
    // memory (e.g. x = 960). Without the shift no letter would be found.
    ByteBuilder table;
    table.i16(0).i16(0).i16(0).i16(0);         // code 0: nothing
    table.i16(0).i16(0).i16(0).i16(0);         // code 1: space
    table.i16(960).i16(320).i16(2).i16(3);     // code 2
    table.i16(962).i16(321).i16(1).i16(2);     // code 3
    const auto bitmap = makeBitmap(4, 4, 8, {kBlack, kWhite}, {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}});

    const auto font = loadBitmapFont(table.bytes(), bitmap);
    REQUIRE(font.glyphs.size() == 4);
    CHECK(font.glyph(2).x == 0);
    CHECK(font.glyph(2).y == 0);
    CHECK(font.glyph(3).x == 2);
    CHECK(font.glyph(3).y == 1);
    CHECK(font.glyph(1).empty());
    CHECK(font.glyph(200).empty());  // beyond the table
    CHECK(font.lineHeight == 3);
    CHECK(font.spaceWidth >= 1);
}

TEST_CASE("A font whose letters lie outside its picture is rejected", "[formats][font]") {
    // A mismatched .tab and .BMP (e.g. from two different mods) would read
    // outside the texture; better to say so when loading.
    ByteBuilder table;
    table.i16(960).i16(320).i16(2).i16(2);
    table.i16(964).i16(320).i16(2).i16(2);  // ends at x = 6, picture is 4 wide
    const auto bitmap = makeBitmap(4, 2, 8, {kBlack}, {{0, 0, 0, 0}, {0, 0, 0, 0}});
    CHECK_THROWS_AS(loadBitmapFont(table.bytes(), bitmap), FormatError);
}

TEST_CASE("A font table must be made of whole entries", "[formats][font]") {
    // A truncated .tab would shift every following letter.
    const std::vector<std::byte> broken(12);
    CHECK_THROWS_AS(parseFontTable(broken), FormatError);
}
