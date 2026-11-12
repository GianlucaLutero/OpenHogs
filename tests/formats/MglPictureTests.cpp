// Tests for formats::decompressMgl and decodeMglPicture: the LZ scheme of
// the game's .mgl pictures (menu pictures on PC; title screens, newspapers
// and cards on the PlayStation).
//
// Each command below is one the executable's routine knows; getting one
// wrong shifts every following byte and ruins the whole picture. The
// decoder was also checked to give byte for byte the uncompressed .BIN
// copies the PlayStation disc keeps of some cards.

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/MglPicture.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

using namespace openhogs;

namespace {

std::vector<std::byte> bytes(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    for (const int value : values) out.push_back(static_cast<std::byte>(value));
    return out;
}

} // namespace

TEST_CASE("Literal runs and repeats", "[formats][mgl]") {
    // 00xxxxxx copies x bytes; 0110nnnn repeats the last byte n + 3 times;
    // a zero byte ends the data.
    CHECK(formats::decompressMgl(bytes({0x03, 1, 2, 3, 0x00})) == bytes({1, 2, 3}));
    CHECK(formats::decompressMgl(bytes({0x01, 9, 0x61, 0x00})) == bytes({9, 9, 9, 9, 9}));
    // 0111nnnn repeats the last 16-bit value n + 2 times.
    CHECK(formats::decompressMgl(bytes({0x02, 0x34, 0x12, 0x70, 0x00})) ==
          bytes({0x34, 0x12, 0x34, 0x12, 0x34, 0x12}));
}

TEST_CASE("Gradients continue the step of the last values", "[formats][mgl]") {
    // 0100nnnn: n + 3 bytes going on by the difference of the last two
    // (wrapping at 256); 0101nnnn: the same for 16-bit values.
    CHECK(formats::decompressMgl(bytes({0x02, 10, 12, 0x40, 0x00})) == bytes({10, 12, 14, 16, 18}));
    CHECK(formats::decompressMgl(bytes({0x02, 250, 253, 0x40, 0x00})) == bytes({250, 253, 0, 3, 6}));
    CHECK(formats::decompressMgl(bytes({0x04, 0x00, 0x01, 0x10, 0x01, 0x50, 0x00})) ==
          bytes({0x00, 0x01, 0x10, 0x01, 0x20, 0x01, 0x30, 0x01}));
}

TEST_CASE("Copies from earlier in the picture", "[formats][mgl]") {
    // 10dddddd: 3 bytes from d + 3 back; 110lllhh dd: l + 4 bytes from
    // (h:d) + 3 back; 111hhhhh dd ll: l + 5 bytes. Copies may overlap
    // what they write.
    CHECK(formats::decompressMgl(bytes({0x04, 1, 2, 3, 4, 0x81, 0x00})) == bytes({1, 2, 3, 4, 1, 2, 3}));
    CHECK(formats::decompressMgl(bytes({0x03, 7, 8, 9, 0xC8, 0x00, 0x00})) == bytes({7, 8, 9, 7, 8, 9, 7, 8, 9}));
    CHECK(formats::decompressMgl(bytes({0x03, 5, 6, 7, 0xE0, 0x00, 0x01, 0x00})) ==
          bytes({5, 6, 7, 5, 6, 7, 5, 6, 7}));
}

TEST_CASE("Damaged data is refused, not read past", "[formats][mgl]") {
    // A copy reaching before the start, a missing end marker or a
    // truncated command must fail cleanly instead of reading garbage.
    CHECK_THROWS_AS(formats::decompressMgl(bytes({0x81, 0x00})), core::FormatError);
    CHECK_THROWS_AS(formats::decompressMgl(bytes({0x02, 7, 8, 0xC4, 0x00, 0x00})), core::FormatError);
    CHECK_THROWS_AS(formats::decompressMgl(bytes({0x03, 1, 2, 3})), core::FormatError);
    CHECK_THROWS_AS(formats::decompressMgl(bytes({0x03, 1, 2})), core::FormatError);
    CHECK_THROWS_AS(formats::decompressMgl(bytes({0x03, 1, 2, 3, 0xE0, 0x00})), core::FormatError);
    CHECK_THROWS_AS(formats::decompressMgl(bytes({})), core::FormatError);
}

TEST_CASE("Magenta pixels of menu pictures become transparent", "[formats][mgl]") {
    // The PC front end draws pure magenta as see-through, which is how its
    // brass panels sit on the background.
    std::vector<std::byte> bmp;
    const auto u16 = [&](std::uint16_t v) { for (int i = 0; i < 2; ++i) bmp.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF)); };
    const auto u32 = [&](std::uint32_t v) { for (int i = 0; i < 4; ++i) bmp.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF)); };
    // A 2 x 1 24-bit BMP: one magenta pixel, one grey (rows padded to 8 bytes).
    bmp.push_back(std::byte{'B'});
    bmp.push_back(std::byte{'M'});
    u32(62); u16(0); u16(0); u32(54);
    u32(40); u32(2); u32(1); u16(1); u16(24); u32(0); u32(8); u32(0); u32(0); u32(0); u32(0);
    for (const int value : {255, 0, 255, 128, 128, 128, 0, 0}) bmp.push_back(static_cast<std::byte>(value));  // B G R

    std::vector<std::byte> mgl;
    for (std::size_t start = 0; start < bmp.size(); start += 63) {
        const std::size_t count = std::min<std::size_t>(63, bmp.size() - start);
        mgl.push_back(static_cast<std::byte>(count));
        mgl.insert(mgl.end(), bmp.begin() + static_cast<std::ptrdiff_t>(start), bmp.begin() + static_cast<std::ptrdiff_t>(start + count));
    }
    mgl.push_back(std::byte{0});

    const auto image = formats::decodeMglPicture(mgl);
    REQUIRE(image.width == 2);
    CHECK(image.pixels[3] == 0);    // magenta: transparent
    CHECK(image.pixels[7] == 255);  // grey: opaque
    CHECK(image.pixels[4] == 128);
}
