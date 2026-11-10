// PackageArchive.h
//
// Reader for the .MAD and .MTD archives of the original game.
//
// Both extensions share one simple layout (see docs/formats/mad-mtd.md):
// an index of 24-byte entries followed by the file data.
//
//     +0   char[16]  file name, zero padded   (e.g. "STF02PPP.VTX")
//     +16  uint32    offset of the data from the start of the archive
//     +20  uint32    size of the data in bytes
//
// There is no entry count: the index ends where the data of the first entry
// begins. MAD archives usually hold models (.VTX/.NO2/.FAC), MTD archives
// usually hold textures (.TIM), but this is a convention, not a rule.
//
// Note: Chars/mcap.mad shares the extension but is not an archive (it holds
// animation data). Use looksLikePackage() to tell them apart.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::formats {

struct PackageEntry {
    std::string name;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

class PackageArchive {
public:
    // Parses an archive held in memory. The archive keeps its own copy of
    // the data, so the returned object is self-contained.
    // Throws core::FormatError if the data is not a valid archive.
    static PackageArchive parse(std::vector<std::byte> data);

    // Quick check, without throwing, used to skip files such as mcap.mad
    // that have an archive extension but a different content.
    static bool looksLikePackage(std::span<const std::byte> data);

    const std::vector<PackageEntry>& entries() const { return entries_; }

    // Finds an entry by name, ignoring case. Returns nullptr if absent.
    const PackageEntry* find(std::string_view name) const;

    // Bytes of one entry. The span stays valid as long as the archive lives.
    std::span<const std::byte> entryData(const PackageEntry& entry) const;

private:
    std::vector<std::byte> data_;
    std::vector<PackageEntry> entries_;
};

} // namespace openhogs::formats
