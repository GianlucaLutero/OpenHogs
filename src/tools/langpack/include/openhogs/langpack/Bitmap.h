// Bitmap.h
//
// Helpers for the Windows .BMP pictures used by the PC version for its
// full-screen images (briefings, newspapers, title screens): uncompressed
// 24-bit pictures, rows stored bottom to top.
//
// The language pack builder uses them to turn the PlayStation briefings
// (512 x 256, raw 15-bit) into files the PC layout expects (640 x 480 BMP).

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::langpack {

struct BitmapSize {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Reads the size from a BMP header (both the usual 40-byte header and the
// old 12-byte OS/2 header, which one game file uses). Throws
// core::FormatError if the data is not a BMP file or the size is absurd.
BitmapSize readBitmapSize(std::span<const std::byte> bmp);

// Encodes an image as an uncompressed 24-bit BMP (alpha is dropped).
std::vector<std::byte> encodeBitmap24(const formats::RgbaImage& image);

// Scales an image to a new size with bilinear filtering, which keeps text
// in the scaled briefings readable (nearest-neighbour would make letters
// uneven when scaling 256 rows to 480).
formats::RgbaImage resizeBilinear(const formats::RgbaImage& image, std::uint32_t width, std::uint32_t height);

} // namespace openhogs::langpack
