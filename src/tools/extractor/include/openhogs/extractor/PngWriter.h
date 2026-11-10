// PngWriter.h
//
// Minimal PNG encoder used to export textures.
//
// It writes valid, uncompressed PNG files (the image data is stored in
// "stored" deflate blocks). Files are larger than with real compression,
// but this keeps the project free of external libraries such as zlib, and
// every image viewer and editor can open them.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::extractor {

// Encodes an RGBA image as a PNG file in memory.
std::vector<std::byte> encodePng(const formats::RgbaImage& image);

// Checksums required by the PNG format, exposed for testing.
std::uint32_t crc32(std::span<const std::byte> data);
std::uint32_t adler32(std::span<const std::byte> data);

} // namespace openhogs::extractor
