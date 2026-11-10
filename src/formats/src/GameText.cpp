// GameText.cpp - see GameText.h for an overview.

#include "openhogs/formats/GameText.h"

#include "openhogs/core/BinaryReader.h"  // FormatError

#include <cstdint>

namespace openhogs::formats {

namespace {

constexpr std::uint8_t kSpaceCode = 0x01;
constexpr std::uint8_t kFirstAsciiCode = 0x02;  // '!'
constexpr std::uint8_t kLastAsciiCode = 0x3B;   // 'Z'
constexpr std::uint8_t kAsciiOffset = 0x1F;     // ASCII = code + 0x1F

const char* const kHexDigits = "0123456789ABCDEF";

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void appendEscape(std::string& text, std::uint8_t code) {
    text.push_back('{');
    text.push_back(kHexDigits[code >> 4]);
    text.push_back(kHexDigits[code & 0x0F]);
    text.push_back('}');
}

} // namespace

std::string decodeGameText(std::span<const std::byte> encoded) {
    std::string text;
    for (const std::byte value : encoded) {
        const auto code = std::to_integer<std::uint8_t>(value);

        if (code == kSpaceCode) {
            text.push_back(' ');
        } else if (code >= kFirstAsciiCode && code <= kLastAsciiCode) {
            // This range ends at 'Z', so it never produces '{' and plain
            // characters can never be confused with an escape.
            text.push_back(static_cast<char>(code + kAsciiOffset));
        } else {
            appendEscape(text, code);
        }
    }
    return text;
}

std::vector<std::byte> encodeGameText(std::string_view text) {
    std::vector<std::byte> encoded;

    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        // "{xx}" escape: an explicit font code.
        if (c == '{') {
            const bool complete = i + 3 < text.size() && text[i + 3] == '}';
            const int high = complete ? hexValue(text[i + 1]) : -1;
            const int low = complete ? hexValue(text[i + 2]) : -1;
            if (high < 0 || low < 0) {
                throw core::FormatError("Invalid escape in game text at position " +
                                        std::to_string(i) + ": expected {xx} with two hex digits");
            }
            const auto code = static_cast<std::uint8_t>(high * 16 + low);
            if (code == 0) {
                // A zero byte would end the string early inside the game.
                throw core::FormatError("Game text cannot contain {00}");
            }
            encoded.push_back(static_cast<std::byte>(code));
            i += 3;
            continue;
        }

        if (c == ' ') {
            encoded.push_back(static_cast<std::byte>(kSpaceCode));
            continue;
        }

        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');  // the font is upper case only
        }

        const int code = c - kAsciiOffset;
        if (code < kFirstAsciiCode || code > kLastAsciiCode) {
            throw core::FormatError(std::string("Character '") + c +
                                    "' cannot be shown by the game font; use a {xx} code");
        }
        encoded.push_back(static_cast<std::byte>(code));
    }
    return encoded;
}

} // namespace openhogs::formats
