// Bitmap.cpp - see Bitmap.h for an overview.

#include "openhogs/langpack/Bitmap.h"

#include "openhogs/core/BinaryReader.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace openhogs::langpack {

namespace {

constexpr std::uint32_t kFileHeaderSize = 14;
constexpr std::uint32_t kInfoHeaderSize = 40;  // BITMAPINFOHEADER
constexpr std::uint32_t kCoreHeaderSize = 12;  // old OS/2 BITMAPCOREHEADER

// Larger sizes mean a misread header; refusing them avoids allocating
// gigabytes for a picture that cannot exist.
constexpr std::uint32_t kMaxDimension = 16384;

void appendU16(std::vector<std::byte>& out, std::uint16_t value) {
    out.push_back(static_cast<std::byte>(value & 0xFF));
    out.push_back(static_cast<std::byte>(value >> 8));
}

void appendU32(std::vector<std::byte>& out, std::uint32_t value) {
    appendU16(out, static_cast<std::uint16_t>(value & 0xFFFF));
    appendU16(out, static_cast<std::uint16_t>(value >> 16));
}

} // namespace

BitmapSize readBitmapSize(std::span<const std::byte> bmp) {
    core::BinaryReader reader(bmp);
    if (reader.readU8() != 'B' || reader.readU8() != 'M') {
        throw core::FormatError("Not a BMP file");
    }
    reader.seek(kFileHeaderSize);
    const std::uint32_t headerSize = reader.readU32();

    BitmapSize size;
    if (headerSize == kCoreHeaderSize) {
        // Old OS/2 header with 16-bit sizes. The PC game ships one picture
        // like this: Language/Tims/Briefing/level24.BMP.
        size.width = reader.readU16();
        size.height = reader.readU16();
    } else {
        size.width = static_cast<std::uint32_t>(std::abs(reader.readI32()));
        // A negative height means "rows top to bottom"; only the size matters here.
        size.height = static_cast<std::uint32_t>(std::abs(reader.readI32()));
    }

    if (size.width == 0 || size.height == 0 || size.width > kMaxDimension || size.height > kMaxDimension) {
        throw core::FormatError("BMP: unsupported picture size " + std::to_string(size.width) + "x" +
                                std::to_string(size.height));
    }
    return size;
}

std::vector<std::byte> encodeBitmap24(const formats::RgbaImage& image) {
    // Each row is padded to a multiple of 4 bytes.
    const std::uint32_t rowBytes = (image.width * 3 + 3) & ~3u;
    const std::uint32_t pixelBytes = rowBytes * image.height;

    std::vector<std::byte> out;
    out.reserve(kFileHeaderSize + kInfoHeaderSize + pixelBytes);

    // File header.
    out.push_back(std::byte{'B'});
    out.push_back(std::byte{'M'});
    appendU32(out, kFileHeaderSize + kInfoHeaderSize + pixelBytes);
    appendU32(out, 0);                                  // reserved
    appendU32(out, kFileHeaderSize + kInfoHeaderSize);  // offset of the pixels

    // Info header.
    appendU32(out, kInfoHeaderSize);
    appendU32(out, image.width);
    appendU32(out, image.height);  // positive: rows bottom to top, like the game's files
    appendU16(out, 1);             // planes
    appendU16(out, 24);            // bits per pixel
    appendU32(out, 0);             // no compression
    appendU32(out, pixelBytes);
    appendU32(out, 2835);          // 72 DPI, horizontal
    appendU32(out, 2835);          // 72 DPI, vertical
    appendU32(out, 0);             // colours in palette
    appendU32(out, 0);             // important colours

    // Pixels: bottom row first, each pixel as blue, green, red.
    for (std::uint32_t row = image.height; row-- > 0;) {
        const std::uint8_t* source = image.pixels.data() + std::size_t{row} * image.width * 4;
        for (std::uint32_t x = 0; x < image.width; ++x) {
            out.push_back(static_cast<std::byte>(source[x * 4 + 2]));
            out.push_back(static_cast<std::byte>(source[x * 4 + 1]));
            out.push_back(static_cast<std::byte>(source[x * 4 + 0]));
        }
        for (std::uint32_t pad = image.width * 3; pad < rowBytes; ++pad) {
            out.push_back(std::byte{0});
        }
    }
    return out;
}

formats::RgbaImage resizeBilinear(const formats::RgbaImage& image, std::uint32_t width, std::uint32_t height) {
    formats::RgbaImage result;
    result.width = width;
    result.height = height;
    result.pixels.resize(std::size_t{width} * height * 4);

    const auto pixel = [&](std::uint32_t x, std::uint32_t y, std::uint32_t channel) {
        return static_cast<float>(image.pixels[(std::size_t{y} * image.width + x) * 4 + channel]);
    };

    for (std::uint32_t y = 0; y < height; ++y) {
        // Map the centre of the destination pixel into the source image.
        const float sourceY = std::clamp((static_cast<float>(y) + 0.5f) * static_cast<float>(image.height) /
                                                 static_cast<float>(height) - 0.5f,
                                         0.0f, static_cast<float>(image.height - 1));
        const auto y0 = static_cast<std::uint32_t>(sourceY);
        const std::uint32_t y1 = std::min(y0 + 1, image.height - 1);
        const float fy = sourceY - static_cast<float>(y0);

        for (std::uint32_t x = 0; x < width; ++x) {
            const float sourceX = std::clamp((static_cast<float>(x) + 0.5f) * static_cast<float>(image.width) /
                                                     static_cast<float>(width) - 0.5f,
                                             0.0f, static_cast<float>(image.width - 1));
            const auto x0 = static_cast<std::uint32_t>(sourceX);
            const std::uint32_t x1 = std::min(x0 + 1, image.width - 1);
            const float fx = sourceX - static_cast<float>(x0);

            for (std::uint32_t c = 0; c < 4; ++c) {
                const float top = pixel(x0, y0, c) * (1 - fx) + pixel(x1, y0, c) * fx;
                const float bottom = pixel(x0, y1, c) * (1 - fx) + pixel(x1, y1, c) * fx;
                const float value = top * (1 - fy) + bottom * fy;
                result.pixels[(std::size_t{y} * width + x) * 4 + c] =
                    static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 255.0f)));
            }
        }
    }
    return result;
}

} // namespace openhogs::langpack
