// RawImage.cpp - see RawImage.h for an overview.

#include "openhogs/formats/RawImage.h"

#include "openhogs/core/BinaryReader.h"

#include <string>

namespace openhogs::formats {

namespace {

// Expands a 5-bit channel to 8 bits so that 31 becomes 255.
std::uint8_t expand5To8(std::uint32_t value) {
    return static_cast<std::uint8_t>((value << 3) | (value >> 2));
}

} // namespace

RgbaImage decodeRaw15(std::span<const std::byte> data, std::uint32_t width, std::uint32_t height) {
    const std::size_t pixelCount = std::size_t{width} * height;
    if (data.size() < pixelCount * 2) {
        throw core::FormatError("Raw picture: expected " + std::to_string(pixelCount * 2) + " bytes for " +
                                std::to_string(width) + "x" + std::to_string(height) + ", got " +
                                std::to_string(data.size()));
    }

    RgbaImage image;
    image.width = width;
    image.height = height;
    image.pixels.reserve(pixelCount * 4);

    core::BinaryReader reader(data);
    for (std::size_t i = 0; i < pixelCount; ++i) {
        // Bits 0-4 red, 5-9 green, 10-14 blue (bit 15 is ignored).
        const std::uint16_t colour = reader.readU16();
        image.pixels.push_back(expand5To8(colour & 0x1F));
        image.pixels.push_back(expand5To8((colour >> 5) & 0x1F));
        image.pixels.push_back(expand5To8((colour >> 10) & 0x1F));
        image.pixels.push_back(255);
    }
    return image;
}

} // namespace openhogs::formats
