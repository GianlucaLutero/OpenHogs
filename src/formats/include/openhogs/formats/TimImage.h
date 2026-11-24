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
#include <functional>
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

// Changes one 16-bit palette colour (PlayStation format: bits 0-4 red,
// 5-9 green, 10-14 blue) before the image is decoded; `index` is the
// colour's place in the palette. The original recolours map textures this
// way, palette entry by palette entry (see scene::gradeColour).
using PaletteFilter = std::function<std::uint16_t(std::uint16_t colour, std::size_t index)>;

// Same as decodeTim(data), with every colour of the first palette row
// passed through `filter`. Direct-colour images are left as they are.
RgbaImage decodeTim(std::span<const std::byte> data, const PaletteFilter& filter);

} // namespace openhogs::formats
