// TextTable.h
//
// String tables of the original game: Language/Text/gtext and fetext.
//
// Each table is a pair of files (see docs/formats/text.md):
//
//     <name>.ofs   uint16 offsets, one per string, into the .bin file
//     <name>.bin   the strings, each terminated by a zero byte, in the
//                  game encoding (see GameText.h)
//
// gtext holds in-game text (weapon names, mission names...), fetext the
// front-end menus. The engine refers to strings by their index, so the
// order of the entries must never change.

#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::formats {

struct TextTable {
    // Decoded strings, in index order (see decodeGameText for the syntax).
    std::vector<std::string> entries;

    // Parses an .ofs/.bin pair. Throws core::FormatError if an offset points
    // outside the .bin file or a string has no terminating zero.
    static TextTable parse(std::span<const std::byte> offsets, std::span<const std::byte> strings);

    // Encoded files produced by encode().
    struct EncodedFiles {
        std::vector<std::byte> offsets;  // .ofs
        std::vector<std::byte> strings;  // .bin
    };

    // Encodes the table back into the game format, e.g. to build a
    // translated table for a language pack. Throws core::FormatError if a
    // string cannot be encoded or the .bin would exceed 64 KiB (the limit of
    // 16-bit offsets).
    EncodedFiles encode() const;

    // --- Readable listing ---------------------------------------------------
    // The form used by openhogs-extract and openhogs-text so people can read
    // and translate the tables: one UTF-8 line per string, with its index.
    //
    //     [0040] DEATH VALLEY
    //     [0041] CHILL HILL
    //
    // The index must be kept: the engine looks strings up by index.

    std::string toListing() const;

    // Parses a listing. Lines must be in index order starting at 0, so
    // that a missing or duplicated line is caught instead of shifting every
    // following string. Empty lines are ignored. Throws core::FormatError
    // with the line number on malformed input.
    static TextTable fromListing(std::string_view listing);
};

} // namespace openhogs::formats
