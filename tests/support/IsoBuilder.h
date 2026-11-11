// IsoBuilder.h
//
// Test helper: writes a small but valid ISO 9660 disc image containing the
// given files, in the raw 2352-byte layout of PlayStation .bin dumps
// (Mode 2 Form 1) or as a plain 2048-byte .iso.
//
// It lets tests exercise code that reads the PlayStation disc without a
// real (copyrighted) game disc. Limits, fine for tests: every directory
// must fit in one 2048-byte sector.

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace openhogs::tests {

// Disc path ("TEXT/GITALIAN.BIN", upper case, '/' separators) -> content.
using DiscFiles = std::map<std::string, std::vector<std::byte>>;

void writeIsoImage(const std::filesystem::path& path, const DiscFiles& files, bool rawSectors);

// Convenience: bytes of a string.
std::vector<std::byte> toBytes(const std::string& text);

} // namespace openhogs::tests
