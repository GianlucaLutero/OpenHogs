// Tests for formats::decodeTim (PlayStation TIM textures).
//
// The game's textures are 4-bit and 8-bit palette images. A decoding error
// shows up as scrambled or wrongly coloured textures everywhere, so these
// tests pin down pixel order, colour conversion and transparency.

#include "openhogs/formats/TimImage.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

#include <vector>

using openhogs::core::FormatError;
using openhogs::formats::decodeTim;
using openhogs::tests::ByteBuilder;

namespace {

constexpr std::uint16_t kRed = 0x001F;    // PS1 colour: red = bits 0-4
constexpr std::uint16_t kBlue = 0x7C00;   // blue = bits 10-14
constexpr std::uint16_t kTransparent = 0x0000;

// Writes a TIM header with a one-row palette of `colors`.
ByteBuilder timWithPalette(std::uint32_t pixelMode, const std::vector<std::uint16_t>& colors) {
    ByteBuilder tim;
    tim.u32(0x10).u32(pixelMode | 0x08);  // magic, mode + "has palette"
    const auto paletteBytes = static_cast<std::uint32_t>(12 + colors.size() * 2);
    tim.u32(paletteBytes).u16(0).u16(0).u16(static_cast<std::uint16_t>(colors.size())).u16(1);
    for (const auto color : colors) {
        tim.u16(color);
    }
    return tim;
}

std::vector<std::uint8_t> pixel(const openhogs::formats::RgbaImage& image, std::size_t index) {
    const auto first = image.pixels.begin() + static_cast<std::ptrdiff_t>(index * 4);
    return {first, first + 4};
}

} // namespace

// 4-bit images store two pixels per byte with the LOW nibble first, and the
// stored width counts 16-bit words (4 pixels each). Either detail wrong
// would scramble every texture.
TEST_CASE("decodeTim reads 4-bit images low nibble first", "[formats][TimImage]") {
    auto tim = timWithPalette(0, {kTransparent, kRed, kBlue});
    tim.u32(12 + 2).u16(0).u16(0).u16(1).u16(1);  // 1 word wide = 4 pixels, 1 row
    tim.u8(0x21).u8(0x00);                        // pixels: 1, 2, 0, 0

    const auto image = decodeTim(tim.bytes());

    CHECK(image.width == 4);
    CHECK(image.height == 1);
    CHECK(pixel(image, 0) == std::vector<std::uint8_t>{255, 0, 0, 255});  // red
    CHECK(pixel(image, 1) == std::vector<std::uint8_t>{0, 0, 255, 255});  // blue
}

// PS1 rule used by the game for cut-out sprites (trees, fences): colour
// 0x0000 is transparent. Without it, cut-outs would get black borders.
TEST_CASE("decodeTim makes colour 0x0000 transparent", "[formats][TimImage]") {
    auto tim = timWithPalette(1, {kTransparent, kRed});
    tim.u32(12 + 2).u16(0).u16(0).u16(1).u16(1);  // 1 word = 2 pixels (8-bit)
    tim.u8(0).u8(1);

    const auto image = decodeTim(tim.bytes());

    REQUIRE(image.width == 2);
    CHECK(pixel(image, 0)[3] == 0);
    CHECK(pixel(image, 1)[3] == 255);
}

// A pixel referring to a colour the palette does not have means a corrupt
// file; reading past the palette would produce random colours or crash.
TEST_CASE("decodeTim rejects pixels outside the palette", "[formats][TimImage]") {
    auto tim = timWithPalette(1, {kRed});
    tim.u32(12 + 2).u16(0).u16(0).u16(1).u16(1);
    tim.u8(0).u8(7);  // colour 7 does not exist

    CHECK_THROWS_AS(decodeTim(tim.bytes()), FormatError);
}

// Archives also contain non-image entries; feeding one to the decoder by
// mistake must fail cleanly instead of producing a garbage image.
TEST_CASE("decodeTim rejects files that are not TIM images", "[formats][TimImage]") {
    ByteBuilder notTim;
    notTim.raw("RIFF....WAVE");
    CHECK_THROWS_AS(decodeTim(notTim.bytes()), FormatError);
}
