// GameText.h
//
// Conversion between the game's 1-byte text encoding and readable text.
//
// The original game does not store ASCII. Each byte is an index into its
// bitmap font, which only has upper-case letters (see docs/formats/text.md):
//
//     0x00          end of string
//     0x01          space
//     0x02 - 0x3B   ASCII 0x21 - 0x5A  ('!' ... 'Z'), i.e. code + 0x1F
//     other codes   extra font glyphs (accented letters, symbols)
//
// Codes without a known character are written as "{xx}" (two hex digits)
// so that decoding and re-encoding never loses information. This matters
// for translations: a translated table must contain exactly the bytes the
// game expects.

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::formats {

// Decodes one string (without its terminating zero) into readable text.
// Upper-case ASCII is used for normal characters, "{xx}" for the others.
std::string decodeGameText(std::span<const std::byte> encoded);

// Encodes readable text back into game bytes (without terminating zero).
// Lower-case letters are accepted and stored as upper case, since the font
// has no lower case. Throws core::FormatError for characters the game
// cannot display or for malformed "{xx}" escapes.
std::vector<std::byte> encodeGameText(std::string_view text);

} // namespace openhogs::formats
