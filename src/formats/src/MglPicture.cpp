// MglPicture.cpp - see MglPicture.h for an overview, docs/formats/mgl.md
// for the commands.

#include "openhogs/formats/MglPicture.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/BitmapImage.h"

#include <string>

namespace openhogs::formats {

namespace {

constexpr std::size_t kMaximumSize = 16 * 1024 * 1024;

[[noreturn]] void damaged(const std::string& what) { throw core::FormatError("MGL picture: " + what); }

// The decompressed bytes so far, with the checks every command needs.
class Output {
public:
    std::vector<std::byte> bytes;

    std::byte back(std::size_t distance) const {
        if (distance == 0 || distance > bytes.size()) damaged("a copy reaches before the start");
        return bytes[bytes.size() - distance];
    }
    void put(std::byte value) {
        if (bytes.size() >= kMaximumSize) damaged("the picture is too large");
        bytes.push_back(value);
    }
    // Copies `count` bytes from `distance` back, one at a time, so a copy
    // may repeat the bytes it has just written.
    void copy(std::size_t distance, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) put(back(distance));
    }
};

} // namespace

std::vector<std::byte> decompressMgl(std::span<const std::byte> mgl) {
    Output out;
    std::size_t in = 0;
    const auto next = [&]() {
        if (in >= mgl.size()) damaged("the data ends before its end marker");
        return std::to_integer<unsigned>(mgl[in++]);
    };

    // Each command is one byte, sometimes followed by one or two more; a
    // zero byte ends the picture.
    for (unsigned command = next(); command != 0; command = next()) {
        const unsigned low4 = command & 0x0F;
        if (command < 0x40) {
            // 00xxxxxx: x bytes copied as they are.
            for (unsigned i = 0; i < (command & 0x3F); ++i) out.put(static_cast<std::byte>(next()));
        } else if (command < 0x50) {
            // 0100nnnn: n + 3 bytes continuing the step between the last two.
            auto value = std::to_integer<unsigned>(out.back(1));
            const unsigned step = value - std::to_integer<unsigned>(out.back(2));
            for (unsigned i = 0; i < low4 + 3; ++i) {
                value = (value + step) & 0xFF;
                out.put(static_cast<std::byte>(value));
            }
        } else if (command < 0x60) {
            // 0101nnnn: n + 2 16-bit values continuing the step between the
            // last two (for 16-bit pixels).
            const auto word = [&](std::size_t distance) {
                return std::to_integer<unsigned>(out.back(distance)) | (std::to_integer<unsigned>(out.back(distance - 1)) << 8);
            };
            unsigned value = word(2);
            const unsigned step = value - word(4);
            for (unsigned i = 0; i < low4 + 2; ++i) {
                value = (value + step) & 0xFFFF;
                out.put(static_cast<std::byte>(value & 0xFF));
                out.put(static_cast<std::byte>(value >> 8));
            }
        } else if (command < 0x70) {
            // 0110nnnn: the last byte n + 3 more times.
            out.copy(1, low4 + 3);
        } else if (command < 0x80) {
            // 0111nnnn: the last 16-bit value n + 2 more times.
            out.copy(2, 2 * (low4 + 2));
        } else if (command < 0xC0) {
            // 10dddddd: 3 bytes from d + 3 back.
            out.copy((command & 0x3F) + 3, 3);
        } else if (command < 0xE0) {
            // 110lllhh dddddddd: l + 4 bytes from (h:d) + 3 back.
            const unsigned distance = ((command & 0x03) << 8) | next();
            out.copy(distance + 3, ((command >> 2) & 0x07) + 4);
        } else {
            // 111hhhhh dddddddd llllllll: l + 5 bytes from (h:d) + 3 back.
            const unsigned distance = ((command & 0x1F) << 8) | next();
            const unsigned count = next() + 5;
            out.copy(distance + 3, count);
        }
    }
    return std::move(out.bytes);
}

RgbaImage decodeMglPicture(std::span<const std::byte> mgl) {
    auto image = decodeBitmap(decompressMgl(mgl));
    // Most pictures were made from 15-bit art, where the key is stored as
    // (248, 0, 248); others use (255, 0, 255). Comparing the top five bits
    // of each channel, as a 15-bit screen would, catches both.
    const auto top5 = [](std::uint8_t channel) { return channel >> 3; };
    for (std::size_t i = 0; i + 3 < image.pixels.size(); i += 4) {
        if (top5(image.pixels[i]) == 31 && top5(image.pixels[i + 1]) == 0 && top5(image.pixels[i + 2]) == 31) {
            image.pixels[i + 3] = 0;
        }
    }
    return image;
}

} // namespace openhogs::formats
