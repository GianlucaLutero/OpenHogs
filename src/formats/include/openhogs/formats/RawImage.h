// RawImage.h
//
// Decoder for raw 15-bit PlayStation pictures: pixels stored one after the
// other as 16-bit colours, with no header at all. The PlayStation version
// uses them for full-screen pictures such as the mission briefings
// (BRIEF/<LANGUAGE>/LEVEL*.BIN: 512 x 256 pixels, 262,144 bytes).
//
// Because there is no header, the caller must know the size.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace openhogs::formats {

// Decodes `width` x `height` pixels (top row first). Colour conversion is
// the same as for TIM images, but every pixel is opaque: these pictures
// are backgrounds, not cut-outs. Throws core::FormatError if the data is
// smaller than width * height * 2 bytes.
RgbaImage decodeRaw15(std::span<const std::byte> data, std::uint32_t width, std::uint32_t height);

} // namespace openhogs::formats
