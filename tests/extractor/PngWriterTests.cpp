// Tests for the PNG encoder used to export textures.

#include "openhogs/extractor/PngWriter.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <vector>

using namespace openhogs::extractor;

namespace {

std::vector<std::byte> toBytes(std::string_view text) {
    std::vector<std::byte> bytes;
    for (const char c : text) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    return bytes;
}

} // namespace

// Image viewers reject a PNG whose checksums are wrong. Both checksums are
// compared with the published reference values of each algorithm.
TEST_CASE("PNG checksums match their reference values", "[extractor][PngWriter]") {
    CHECK(crc32(toBytes("123456789")) == 0xCBF43926u);    // CRC-32 check value
    CHECK(adler32(toBytes("Wikipedia")) == 0x11E60398u);  // Adler-32 reference example
}

// The whole file is compared with one produced by an independent encoder
// (Python's zlib + struct, stored/uncompressed mode) for a 2x1 image with
// an opaque red and a transparent blue pixel. This checks signature, chunk
// layout, zlib wrapping and checksums at once.
TEST_CASE("encodePng produces a valid PNG file", "[extractor][PngWriter]") {
    openhogs::formats::RgbaImage image;
    image.width = 2;
    image.height = 1;
    image.pixels = {255, 0, 0, 255, 0, 0, 255, 0};

    const std::vector<std::uint8_t> expected{
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48,
        0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
        0x00, 0xF4, 0x22, 0x7F, 0x8A, 0x00, 0x00, 0x00, 0x14, 0x49, 0x44, 0x41, 0x54, 0x78,
        0x01, 0x01, 0x09, 0x00, 0xF6, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
        0x00, 0x0E, 0xFA, 0x02, 0xFE, 0x56, 0xA2, 0x0E, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x49,
        0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

    const auto png = encodePng(image);
    REQUIRE(png.size() == expected.size());
    for (std::size_t i = 0; i < png.size(); ++i) {
        INFO("byte " << i);
        CHECK(std::to_integer<std::uint8_t>(png[i]) == expected[i]);
    }
}
