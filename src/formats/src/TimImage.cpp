// TimImage.cpp - see TimImage.h for an overview.

#include "openhogs/formats/TimImage.h"

#include "openhogs/core/BinaryReader.h"

#include <array>

namespace openhogs::formats {

namespace {

constexpr std::uint32_t kTimMagic = 0x10;
constexpr std::uint32_t kPixelModeMask = 0x07;
constexpr std::uint32_t kHasPaletteFlag = 0x08;

enum class PixelMode : std::uint32_t { Indexed4 = 0, Indexed8 = 1, Direct16 = 2, Direct24 = 3 };

using Rgba = std::array<std::uint8_t, 4>;

// Expands a 5-bit channel to 8 bits so that 31 becomes 255 (not 248).
std::uint8_t expand5To8(std::uint32_t value) {
    return static_cast<std::uint8_t>((value << 3) | (value >> 2));
}

// PlayStation 16-bit colour: bits 0-4 red, 5-9 green, 10-14 blue,
// bit 15 "semi-transparent". 0x0000 means "fully transparent".
Rgba convertColor16(std::uint16_t color) {
    const std::uint8_t alpha = (color == 0) ? 0 : 255;
    return {expand5To8(color & 0x1F), expand5To8((color >> 5) & 0x1F),
            expand5To8((color >> 10) & 0x1F), alpha};
}

// Reads the palette block. Only the first row is used: the other rows are
// alternative palettes the game picks at runtime (not needed for export).
std::vector<Rgba> readPalette(core::BinaryReader& reader) {
    const std::size_t blockStart = reader.position();
    const std::uint32_t blockLength = reader.readU32();
    reader.skip(4);  // position in PS1 video memory, irrelevant on PC
    const std::uint16_t colorsPerRow = reader.readU16();
    const std::uint16_t rows = reader.readU16();
    if (colorsPerRow == 0 || rows == 0) {
        throw core::FormatError("TIM: empty colour palette");
    }

    std::vector<Rgba> palette;
    palette.reserve(colorsPerRow);
    for (std::uint16_t i = 0; i < colorsPerRow; ++i) {
        palette.push_back(convertColor16(reader.readU16()));
    }

    // Jump over the remaining rows using the declared block length.
    reader.seek(blockStart + blockLength);
    return palette;
}

const Rgba& paletteColor(const std::vector<Rgba>& palette, std::size_t index) {
    if (index >= palette.size()) {
        throw core::FormatError("TIM: pixel uses colour " + std::to_string(index) +
                                " but the palette has only " + std::to_string(palette.size()));
    }
    return palette[index];
}

} // namespace

RgbaImage decodeTim(std::span<const std::byte> data) {
    core::BinaryReader reader(data);

    if (reader.readU32() != kTimMagic) {
        throw core::FormatError("Not a TIM image: wrong magic number");
    }
    const std::uint32_t flags = reader.readU32();
    const auto mode = static_cast<PixelMode>(flags & kPixelModeMask);
    if ((flags & kPixelModeMask) > static_cast<std::uint32_t>(PixelMode::Direct24)) {
        throw core::FormatError("TIM: unsupported pixel mode " + std::to_string(flags & kPixelModeMask));
    }

    const bool indexed = mode == PixelMode::Indexed4 || mode == PixelMode::Indexed8;
    if (indexed && (flags & kHasPaletteFlag) == 0) {
        throw core::FormatError("TIM: indexed image without a colour palette");
    }

    std::vector<Rgba> palette;
    if (flags & kHasPaletteFlag) {
        palette = readPalette(reader);
    }

    // Pixel block header. The stored width counts 16-bit words, not pixels.
    reader.skip(4 + 4);  // block length + position in video memory (both unused)
    const std::uint32_t widthInWords = reader.readU16();
    const std::uint32_t height = reader.readU16();

    RgbaImage image;
    image.height = height;
    switch (mode) {
        case PixelMode::Indexed4: image.width = widthInWords * 4; break;
        case PixelMode::Indexed8: image.width = widthInWords * 2; break;
        case PixelMode::Direct16: image.width = widthInWords; break;
        case PixelMode::Direct24: image.width = widthInWords * 2 / 3; break;
    }
    image.pixels.reserve(std::size_t{image.width} * image.height * 4);

    auto append = [&image](const Rgba& color) {
        image.pixels.insert(image.pixels.end(), color.begin(), color.end());
    };

    // Each row occupies exactly widthInWords * 2 bytes in the file.
    const std::size_t rowBytes = std::size_t{widthInWords} * 2;
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::size_t rowStart = reader.position();
        std::uint32_t x = 0;

        while (x < image.width) {
            switch (mode) {
                case PixelMode::Indexed4: {
                    // Two pixels per byte, low nibble first.
                    const std::uint8_t pair = reader.readU8();
                    append(paletteColor(palette, pair & 0x0F));
                    append(paletteColor(palette, pair >> 4));
                    x += 2;
                    break;
                }
                case PixelMode::Indexed8:
                    append(paletteColor(palette, reader.readU8()));
                    x += 1;
                    break;
                case PixelMode::Direct16:
                    append(convertColor16(reader.readU16()));
                    x += 1;
                    break;
                case PixelMode::Direct24: {
                    const std::uint8_t r = reader.readU8();
                    const std::uint8_t g = reader.readU8();
                    const std::uint8_t b = reader.readU8();
                    append({r, g, b, 255});
                    x += 1;
                    break;
                }
            }
        }
        // 24-bit rows can end with a padding byte: always realign.
        reader.seek(rowStart + rowBytes);
    }
    return image;
}

} // namespace openhogs::formats
