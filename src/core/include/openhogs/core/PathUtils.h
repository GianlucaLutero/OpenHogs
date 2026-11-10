// PathUtils.h
//
// Small string helpers used when dealing with names coming from the
// original game.
//
// The game was written for Windows, where file names are case-insensitive
// and use '\' as separator. Its data files refer to the same file with
// different spellings ("Level1.BMP", "level10.BMP"), so OpenHogs always
// compares paths through a normalised "lookup key".

#pragma once

#include <string>
#include <string_view>

namespace openhogs::core {

// Returns a copy with ASCII letters converted to lower case.
std::string toLower(std::string_view text);

// Returns a copy without leading and trailing spaces, tabs and line breaks.
std::string trim(std::string_view text);

// Turns a game-relative path into the key used for lookups:
//   - '\' becomes '/'
//   - leading "./" and '/' are removed, repeated '/' are collapsed
//   - everything is lower case
// Example: ".\\Speech\\Sku1\\Pig01\\01AM0101.WAV" -> "speech/sku1/pig01/01am0101.wav"
std::string makeLookupKey(std::string_view relativePath);

} // namespace openhogs::core
