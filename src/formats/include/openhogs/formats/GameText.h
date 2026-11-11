// GameText.h
//
// Conversion between the game's 1-byte text encoding and readable text.
//
// The original game does not store ASCII. Each byte is an index into its
// bitmap fonts, which have upper-case letters only (see docs/formats/text.md):
//
//     0x00          end of string
//     0x01          space
//     0x02 - 0x3E   ASCII 0x21 - 0x5D  ('!' ... 'Z', '[', '\', ']'),
//                   i.e. code + 0x1F
//     0x4C - 0x68   accented capitals and a few symbols used by the
//                   European versions: ¡ © ® ¿ À Á Â Ã Ä Ç È É Ê Ë Ì Í Î Ï
//                   Ñ Ò Ó Ô Õ Ö Ù Ú Û Ü ß
//     other codes   icons (hearts, PlayStation buttons...)
//
// Readable text is UTF-8. Codes without a character (the icons) are written
// as "{xx}" (two hex digits) so that decoding and re-encoding never loses
// information. This matters for translations: a translated table must
// contain exactly the bytes the game expects.

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::formats {

// Decodes one string (without its terminating zero) into UTF-8 text.
std::string decodeGameText(std::span<const std::byte> encoded);

// Encodes UTF-8 text back into game bytes (without terminating zero).
// Lower-case letters, accented ones included, are accepted and stored as
// upper case, since the fonts have no lower case. Throws core::FormatError
// for characters the game cannot display or for malformed "{xx}" escapes.
std::vector<std::byte> encodeGameText(std::string_view text);

} // namespace openhogs::formats
