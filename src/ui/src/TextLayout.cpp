// TextLayout.cpp - see TextLayout.h for an overview.

#include "openhogs/ui/TextLayout.h"

#include "openhogs/formats/GameText.h"

#include <algorithm>
#include <cstddef>
#include <exception>

namespace openhogs::ui {

namespace {

constexpr std::uint8_t kSpaceCode = 0x01;

// Length in bytes of the UTF-8 character starting with `lead`.
std::size_t utf8Length(unsigned char lead) {
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;  // a stray continuation byte: skip it on its own
}

} // namespace

std::vector<std::uint8_t> encodeForDisplay(std::string_view text) {
    std::vector<std::uint8_t> codes;
    // Encoding one character at a time lets a single bad character turn
    // into '?' instead of losing the whole string.
    for (std::size_t at = 0; at < text.size();) {
        const std::size_t length = std::min(utf8Length(static_cast<unsigned char>(text[at])), text.size() - at);
        std::string_view character = text.substr(at, length);
        if (character == "{") {
            // An escape such as "{1f}" (an icon) belongs together.
            const auto close = text.find('}', at);
            if (close != std::string_view::npos) {
                character = text.substr(at, close - at + 1);
            }
        }
        try {
            for (const std::byte code : formats::encodeGameText(character)) {
                codes.push_back(static_cast<std::uint8_t>(code));
            }
        } catch (const std::exception&) {
            codes.push_back(static_cast<std::uint8_t>(formats::encodeGameText("?").front()));
        }
        at += character.size();
    }
    return codes;
}

float measureText(const formats::BitmapFont& font, std::string_view text, float scale) {
    float width = 0.0f;
    for (const std::uint8_t code : encodeForDisplay(text)) {
        const auto glyph = font.glyph(code);
        width += static_cast<float>(code == kSpaceCode || glyph.empty() ? font.spaceWidth : glyph.width);
    }
    return width * scale;
}

std::vector<GlyphQuad> layoutText(const formats::BitmapFont& font, std::string_view text, float x, float y,
                                  float scale) {
    std::vector<GlyphQuad> quads;
    float penX = x;
    for (const std::uint8_t code : encodeForDisplay(text)) {
        const auto glyph = font.glyph(code);
        if (code == kSpaceCode || glyph.empty()) {
            penX += static_cast<float>(font.spaceWidth) * scale;
            continue;
        }
        // Shorter characters (if a font mixes heights) sit on the baseline.
        const float top = y + static_cast<float>(font.lineHeight - glyph.height) * scale;
        quads.push_back({glyph, penX, top, static_cast<float>(glyph.width) * scale,
                         static_cast<float>(glyph.height) * scale});
        penX += static_cast<float>(glyph.width) * scale;
    }
    return quads;
}

std::string fillPlaceholders(std::string_view text, const std::vector<std::string>& strings,
                             const std::vector<int>& numbers) {
    std::string result;
    std::size_t nextString = 0;
    std::size_t nextNumber = 0;
    for (std::size_t at = 0; at < text.size(); ++at) {
        if (text[at] != '>') {
            result += text[at];
            continue;
        }
        // Optional digit count, then S or N.
        std::size_t look = at + 1;
        int digits = 0;
        if (look < text.size() && text[look] >= '1' && text[look] <= '9') {
            digits = text[look] - '0';
            ++look;
        }
        if (look >= text.size() || (text[look] != 'S' && text[look] != 'N')) {
            result += text[at];  // a plain '>'
            continue;
        }
        if (text[look] == 'S') {
            if (nextString < strings.size()) result += strings[nextString++];
        } else if (nextNumber < numbers.size()) {
            std::string number = std::to_string(numbers[nextNumber++]);
            if (number.size() < static_cast<std::size_t>(digits)) {
                number.insert(0, static_cast<std::size_t>(digits) - number.size(), '0');
            }
            result += number;
        }
        at = look;
    }
    return result;
}

} // namespace openhogs::ui
