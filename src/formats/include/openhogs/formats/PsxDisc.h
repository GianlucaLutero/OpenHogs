// PsxDisc.h
//
// Read-only access to the files on a PlayStation disc image.
//
// Used to take content (e.g. the Italian text and briefings) from the
// user's own copy of the PlayStation version. Supported images:
//
//   - raw "BIN" images with 2352-byte sectors (the usual .bin/.cue dumps;
//     PlayStation data tracks are "Mode 2 Form 1": 2048 bytes of data at
//     offset 24 of each sector), and
//   - plain ISO images with 2048-byte sectors.
//
// Only the first (data) track is needed; audio tracks are ignored. Files
// are found through the ISO 9660 file system stored on the disc. See
// docs/formats/psx-disc.md.

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::formats {

class PsxDisc {
public:
    // Opens the image and reads its directory tree. Throws
    // core::FormatError if the file is not a readable ISO 9660 image.
    explicit PsxDisc(const std::filesystem::path& imagePath);

    // Paths of all files, like "TEXT/GITALIAN.BIN" (upper case, '/' as
    // separator, without the ";1" version suffix), sorted.
    std::vector<std::string> listFiles() const;

    bool exists(std::string_view path) const;

    // Reads a whole file. `path` is case-insensitive and may use '\' or a
    // leading '/'. Throws std::runtime_error if the file does not exist.
    std::vector<std::byte> readFile(std::string_view path) const;

    // Sector size of the image: 2352 (raw) or 2048 (ISO).
    std::size_t sectorSize() const { return sectorSize_; }

private:
    struct FileLocation {
        std::uint32_t firstSector = 0;
        std::uint32_t size = 0;
    };

    // Returns the 2048 data bytes of a sector.
    std::vector<std::byte> readSector(std::uint32_t sector) const;
    void readDirectory(std::uint32_t firstSector, std::uint32_t size, const std::string& prefix, int depth);

    mutable std::ifstream file_;
    std::size_t sectorSize_ = 0;
    std::size_t dataOffset_ = 0;  // where the 2048 data bytes start in a sector
    std::map<std::string, FileLocation> files_;  // key: normalised path
};

// Normalises a disc path for lookups: upper case, '/' separators, no
// leading '/', no ";1" suffix. "\\text\\gitalian.bin;1" -> "TEXT/GITALIAN.BIN".
std::string normalisePsxPath(std::string_view path);

} // namespace openhogs::formats
