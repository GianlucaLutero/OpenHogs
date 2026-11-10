// PngWriter.cpp - see PngWriter.h for an overview.
//
// A PNG file is a signature followed by "chunks":
//   IHDR  image size and pixel format
//   IDAT  the pixels, wrapped in a zlib stream
//   IEND  end marker
// Each chunk is: length (big-endian), 4-letter type, data, CRC-32 of type+data.

#include "openhogs/extractor/PngWriter.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace openhogs::extractor {

namespace {

void appendU32BigEndian(std::vector<std::byte>& out, std::uint32_t value) {
    out.push_back(static_cast<std::byte>((value >> 24) & 0xFF));
    out.push_back(static_cast<std::byte>((value >> 16) & 0xFF));
    out.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::byte>(value & 0xFF));
}

void appendChunk(std::vector<std::byte>& out, std::string_view type, std::span<const std::byte> data) {
    appendU32BigEndian(out, static_cast<std::uint32_t>(data.size()));

    // The CRC covers the type and the data, so build them together.
    std::vector<std::byte> typeAndData;
    for (const char c : type) {
        typeAndData.push_back(static_cast<std::byte>(c));
    }
    typeAndData.insert(typeAndData.end(), data.begin(), data.end());

    out.insert(out.end(), typeAndData.begin(), typeAndData.end());
    appendU32BigEndian(out, crc32(typeAndData));
}

// Wraps raw bytes in a zlib stream made of uncompressed ("stored") blocks.
std::vector<std::byte> zlibStore(std::span<const std::byte> raw) {
    constexpr std::size_t kMaxStoredBlock = 65535;  // limit of the 16-bit length field

    std::vector<std::byte> out;
    out.push_back(std::byte{0x78});  // zlib header: deflate, 32 KiB window
    out.push_back(std::byte{0x01});  // no preset dictionary, fastest level

    std::size_t position = 0;
    do {
        const std::size_t blockSize = std::min(kMaxStoredBlock, raw.size() - position);
        const bool isLast = position + blockSize == raw.size();

        out.push_back(static_cast<std::byte>(isLast ? 1 : 0));  // BFINAL bit, type 00 = stored
        const auto length = static_cast<std::uint16_t>(blockSize);
        const auto lengthComplement = static_cast<std::uint16_t>(~length);
        out.push_back(static_cast<std::byte>(length & 0xFF));  // little-endian here
        out.push_back(static_cast<std::byte>(length >> 8));
        out.push_back(static_cast<std::byte>(lengthComplement & 0xFF));
        out.push_back(static_cast<std::byte>(lengthComplement >> 8));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(position),
                   raw.begin() + static_cast<std::ptrdiff_t>(position + blockSize));

        position += blockSize;
    } while (position < raw.size());

    appendU32BigEndian(out, adler32(raw));
    return out;
}

} // namespace

std::uint32_t crc32(std::span<const std::byte> data) {
    // Standard CRC-32 (polynomial 0xEDB88320), table built once on first use.
    static const std::array<std::uint32_t, 256> table = [] {
        std::array<std::uint32_t, 256> values{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int bit = 0; bit < 8; ++bit) {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            values[n] = c;
        }
        return values;
    }();

    std::uint32_t crc = 0xFFFFFFFFu;
    for (const std::byte b : data) {
        crc = table[(crc ^ std::to_integer<std::uint32_t>(b)) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

std::uint32_t adler32(std::span<const std::byte> data) {
    constexpr std::uint32_t kModulo = 65521;
    std::uint32_t a = 1;
    std::uint32_t b = 0;
    for (const std::byte value : data) {
        a = (a + std::to_integer<std::uint32_t>(value)) % kModulo;
        b = (b + a) % kModulo;
    }
    return (b << 16) | a;
}

std::vector<std::byte> encodePng(const formats::RgbaImage& image) {
    std::vector<std::byte> png;

    // 1. Signature.
    constexpr std::array<std::uint8_t, 8> kSignature{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    for (const auto b : kSignature) {
        png.push_back(static_cast<std::byte>(b));
    }

    // 2. IHDR: width, height, 8 bits per channel, colour type 6 (RGBA),
    //    default compression, filter and no interlacing.
    std::vector<std::byte> header;
    appendU32BigEndian(header, image.width);
    appendU32BigEndian(header, image.height);
    constexpr std::array<std::uint8_t, 5> kFormat{8, 6, 0, 0, 0};
    for (const std::uint8_t value : kFormat) {
        header.push_back(static_cast<std::byte>(value));
    }
    appendChunk(png, "IHDR", header);

    // 3. IDAT: every row starts with a filter byte (0 = none).
    const std::size_t rowBytes = std::size_t{image.width} * 4;
    std::vector<std::byte> raw;
    raw.reserve((rowBytes + 1) * image.height);
    for (std::uint32_t y = 0; y < image.height; ++y) {
        raw.push_back(std::byte{0});
        const auto* row = image.pixels.data() + y * rowBytes;
        for (std::size_t i = 0; i < rowBytes; ++i) {
            raw.push_back(static_cast<std::byte>(row[i]));
        }
    }
    appendChunk(png, "IDAT", zlibStore(raw));

    // 4. IEND.
    appendChunk(png, "IEND", {});
    return png;
}

} // namespace openhogs::extractor
