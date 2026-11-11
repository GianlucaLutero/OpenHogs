// GameText.cpp - see GameText.h for an overview.

#include "openhogs/formats/GameText.h"

#include "openhogs/core/BinaryReader.h"  // FormatError

#include <array>
#include <cstdint>

namespace openhogs::formats {

namespace {

constexpr std::uint8_t kSpaceCode = 0x01;
constexpr std::uint8_t kFirstAsciiCode = 0x02;  // '!'
constexpr std::uint8_t kLastAsciiCode = 0x3E;   // ']'
constexpr std::uint8_t kAsciiOffset = 0x1F;     // ASCII = code + 0x1F

// Extra glyphs of the fonts, as UTF-8. The order of the codes follows the
// fonts: each accented letter comes right after the plain letter's group.
// Verified by drawing every glyph of FEText/BIG.BMP with its rectangle
// from FEText/BIG.tab (PC version), and by real words in the PlayStation
// translations: Italian "MODALITÀ", "SÌ", "GIÙ", French "FRANÇOIS",
// German "SANITÄTER", "TRÖGE", "WÜSTE", Spanish "INVASIÓN", "MAÑANA".
struct ExtraGlyph {
    std::uint8_t code;
    std::string_view upper;  // how the glyph is written when decoding
    std::string_view lower;  // lower-case form also accepted when encoding
};

constexpr std::array<ExtraGlyph, 29> kExtraGlyphs{{
    {0x4C, "¡", "¡"}, {0x4D, "©", "©"}, {0x4E, "®", "®"}, {0x4F, "¿", "¿"},
    {0x50, "À", "à"}, {0x51, "Á", "á"}, {0x52, "Â", "â"}, {0x53, "Ã", "ã"}, {0x54, "Ä", "ä"},
    {0x55, "Ç", "ç"},
    {0x56, "È", "è"}, {0x57, "É", "é"}, {0x58, "Ê", "ê"}, {0x59, "Ë", "ë"},
    {0x5A, "Ì", "ì"}, {0x5B, "Í", "í"}, {0x5C, "Î", "î"}, {0x5D, "Ï", "ï"},
    {0x5E, "Ñ", "ñ"},
    {0x5F, "Ò", "ò"}, {0x60, "Ó", "ó"}, {0x61, "Ô", "ô"}, {0x62, "Õ", "õ"}, {0x63, "Ö", "ö"},
    {0x64, "Ù", "ù"}, {0x65, "Ú", "ú"}, {0x66, "Û", "û"}, {0x67, "Ü", "ü"},
    {0x68, "ß", "ß"},
}};

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

const ExtraGlyph* findGlyphByCode(std::uint8_t code) {
    for (const auto& glyph : kExtraGlyphs) {
        if (glyph.code == code) {
            return &glyph;
        }
    }
    return nullptr;
}

// Returns the glyph whose UTF-8 form starts `text`, and its length in bytes.
const ExtraGlyph* findGlyphAtStart(std::string_view text, std::size_t& length) {
    for (const auto& glyph : kExtraGlyphs) {
        for (const std::string_view form : {glyph.upper, glyph.lower}) {
            if (text.starts_with(form)) {
                length = form.size();
                return &glyph;
            }
        }
    }
    return nullptr;
}

// Parses "{xx}" at the start of `text`. Throws if malformed.
std::uint8_t parseEscape(std::string_view text, std::size_t position) {
    const bool complete = text.size() >= 4 && text[3] == '}';
    const int high = complete ? hexValue(text[1]) : -1;
    const int low = complete ? hexValue(text[2]) : -1;
    if (high < 0 || low < 0) {
        throw core::FormatError("Invalid escape in game text at position " + std::to_string(position) +
                                ": expected {xx} with two hex digits");
    }
    const auto code = static_cast<std::uint8_t>(high * 16 + low);
    if (code == 0) {
        // A zero byte would end the string early inside the game.
        throw core::FormatError("Game text cannot contain {00}");
    }
    return code;
}

} // namespace

std::string decodeGameText(std::span<const std::byte> encoded) {
    std::string text;
    for (const std::byte value : encoded) {
        const auto code = std::to_integer<std::uint8_t>(value);

        if (code == kSpaceCode) {
            text.push_back(' ');
        } else if (code >= kFirstAsciiCode && code <= kLastAsciiCode) {
            // This range ends at ']', so it never produces '{' and plain
            // characters can never be confused with an escape.
            text.push_back(static_cast<char>(code + kAsciiOffset));
        } else if (const ExtraGlyph* glyph = findGlyphByCode(code)) {
            text += glyph->upper;
        } else {
            appendEscape(text, code);
        }
    }
    return text;
}

std::vector<std::byte> encodeGameText(std::string_view text) {
    std::vector<std::byte> encoded;

    std::size_t i = 0;
    while (i < text.size()) {
        const std::string_view rest = text.substr(i);

        // "{xx}" escape: an explicit font code.
        if (rest.front() == '{') {
            encoded.push_back(static_cast<std::byte>(parseEscape(rest, i)));
            i += 4;
            continue;
        }

        // Accented letters and symbols are several bytes long in UTF-8.
        std::size_t length = 0;
        if (const ExtraGlyph* glyph = findGlyphAtStart(rest, length)) {
            encoded.push_back(static_cast<std::byte>(glyph->code));
            i += length;
            continue;
        }

        char c = rest.front();
        ++i;
        if (c == ' ') {
            encoded.push_back(static_cast<std::byte>(kSpaceCode));
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');  // the fonts are upper case only
        }
        const int code = c - kAsciiOffset;
        if (code < kFirstAsciiCode || code > kLastAsciiCode) {
            throw core::FormatError("Character at position " + std::to_string(i - 1) +
                                    " cannot be shown by the game font; use a {xx} code");
        }
        encoded.push_back(static_cast<std::byte>(code));
    }
    return encoded;
}

} // namespace openhogs::formats
