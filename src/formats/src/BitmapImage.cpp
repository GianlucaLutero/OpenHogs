// BitmapImage.cpp - see BitmapImage.h for an overview.

#include "openhogs/formats/BitmapImage.h"

#include "openhogs/core/BinaryReader.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace openhogs::formats {

namespace {

constexpr std::uint32_t kUncompressed = 0;  // BI_RGB

struct Header {
    std::uint32_t pixelOffset = 0;
    std::uint32_t headerSize = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;  // negative: rows stored top-down
    std::uint16_t bitsPerPixel = 0;
    std::uint32_t compression = 0;
    std::uint32_t coloursUsed = 0;
};

Header readHeader(core::BinaryReader& reader) {
    if (reader.readFixedString(2) != "BM") {
        throw core::FormatError("BMP: missing 'BM' signature");
    }
    Header header;
    reader.skip(8);  // file size, reserved
    header.pixelOffset = reader.readU32();
    header.headerSize = reader.readU32();
    header.width = reader.readI32();
    header.height = reader.readI32();
    reader.skip(2);  // planes
    header.bitsPerPixel = reader.readU16();
    header.compression = reader.readU32();
    reader.skip(12);  // image size, resolution
    header.coloursUsed = reader.readU32();
    return header;
}

// Palette entries are stored blue, green, red, unused.
std::vector<std::array<std::uint8_t, 3>> readPalette(core::BinaryReader& reader, const Header& header) {
    const std::uint32_t count = header.coloursUsed != 0 ? header.coloursUsed : (1u << header.bitsPerPixel);
    reader.seek(14 + header.headerSize);
    std::vector<std::array<std::uint8_t, 3>> palette(count);
    for (auto& colour : palette) {
        const std::uint8_t blue = reader.readU8();
        const std::uint8_t green = reader.readU8();
        const std::uint8_t red = reader.readU8();
        reader.skip(1);
        colour = {red, green, blue};
    }
    return palette;
}

} // namespace

RgbaImage decodeBitmap(std::span<const std::byte> data, BitmapTransparency transparency) {
    core::BinaryReader reader(data);
    const Header header = readHeader(reader);

    const int bits = header.bitsPerPixel;
    if (header.compression != kUncompressed || (bits != 4 && bits != 8 && bits != 24)) {
        throw core::FormatError("BMP: unsupported variant (" + std::to_string(bits) + " bits, compression " +
                                std::to_string(header.compression) + ")");
    }
    if (header.width <= 0 || header.height == 0) {
        throw core::FormatError("BMP: invalid size");
    }

    const bool palettized = bits != 24;
    const auto palette = palettized ? readPalette(reader, header) : std::vector<std::array<std::uint8_t, 3>>{};

    RgbaImage image;
    image.width = static_cast<std::uint32_t>(header.width);
    const bool topDown = header.height < 0;
    image.height = static_cast<std::uint32_t>(topDown ? -header.height : header.height);
    image.pixels.resize(std::size_t{image.width} * image.height * 4);

    // Every row is padded to a multiple of 4 bytes.
    const std::size_t rowBytes = ((std::size_t{image.width} * static_cast<std::size_t>(bits) + 31) / 32) * 4;

    for (std::uint32_t row = 0; row < image.height; ++row) {
        const std::uint32_t stored = topDown ? row : image.height - 1 - row;
        reader.seek(header.pixelOffset + stored * rowBytes);
        const auto bytes = reader.readBytes(rowBytes);

        for (std::uint32_t x = 0; x < image.width; ++x) {
            std::uint8_t* out = &image.pixels[(std::size_t{row} * image.width + x) * 4];
            if (!palettized) {
                const std::size_t at = std::size_t{x} * 3;
                out[0] = static_cast<std::uint8_t>(bytes[at + 2]);
                out[1] = static_cast<std::uint8_t>(bytes[at + 1]);
                out[2] = static_cast<std::uint8_t>(bytes[at]);
                out[3] = 255;
                continue;
            }
            // 4-bit pictures hold two pixels per byte, the left one in the
            // high nibble.
            const auto byte = static_cast<std::uint8_t>(bytes[bits == 8 ? x : x / 2]);
            const std::uint8_t index = bits == 8 ? byte : ((x % 2 == 0) ? byte >> 4 : byte & 0x0F);
            if (index >= palette.size()) {
                throw core::FormatError("BMP: pixel uses colour " + std::to_string(index) + " outside the palette");
            }
            out[0] = palette[index][0];
            out[1] = palette[index][1];
            out[2] = palette[index][2];
            const bool transparent = transparency == BitmapTransparency::PaletteIndex0 && index == 0;
            out[3] = transparent ? 0 : 255;
        }
    }
    return image;
}

} // namespace openhogs::formats
