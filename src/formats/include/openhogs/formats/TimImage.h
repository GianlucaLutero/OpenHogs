// TimImage.h
//
// Decoder for PlayStation TIM images, the texture format of the game
// (the PC version kept the PS1 textures). See docs/formats/tim.md.
//
// A TIM file has a header, an optional colour palette ("CLUT") and the
// pixel block. Pixels are 4 or 8 bit palette indices, or direct 16/24 bit
// colours. The game uses 4 and 8 bit images with a palette.
//
// The decoder produces plain 8-bit RGBA, ready to be saved as PNG or
// uploaded to the GPU.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

struct RgbaImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels;  // width * height * 4 bytes, rows top to bottom
};

// Decodes a TIM file. Throws core::FormatError if it is not a valid TIM.
//
// Transparency follows the PlayStation rule: the 16-bit colour 0x0000
// (black with the "semi-transparent" bit off) is fully transparent; every
// other colour is opaque.
RgbaImage decodeTim(std::span<const std::byte> data);

} // namespace openhogs::formats
