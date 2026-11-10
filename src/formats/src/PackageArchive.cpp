// PackageArchive.cpp - see PackageArchive.h for an overview.

#include "openhogs/formats/PackageArchive.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/core/PathUtils.h"

namespace openhogs::formats {

namespace {

constexpr std::size_t kEntrySize = 24;
constexpr std::size_t kNameLength = 16;

// Names were written by the original tools and are plain printable ASCII.
// Anything else means we are not looking at an index.
bool isPrintableName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (const char c : name) {
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

// Reads and validates the whole index. Shared by parse() and
// looksLikePackage() so both always agree on what a valid archive is.
std::vector<PackageEntry> readIndex(std::span<const std::byte> data) {
    core::BinaryReader reader(data);

    // The first entry's offset tells us where the index stops.
    reader.seek(kNameLength);
    const std::uint32_t indexEnd = reader.readU32();
    if (indexEnd == 0 || indexEnd % kEntrySize != 0 || indexEnd > data.size()) {
        throw core::FormatError("Not a package archive: invalid index size " +
                                std::to_string(indexEnd));
    }

    std::vector<PackageEntry> entries;
    reader.seek(0);
    while (reader.position() < indexEnd) {
        PackageEntry entry;
        entry.name = reader.readFixedString(kNameLength);
        entry.offset = reader.readU32();
        entry.size = reader.readU32();

        const std::string where = "entry " + std::to_string(entries.size());
        if (!isPrintableName(entry.name)) {
            throw core::FormatError("Not a package archive: unreadable name in " + where);
        }
        // Done in 64 bits so that offset + size cannot overflow.
        const std::uint64_t end = std::uint64_t{entry.offset} + entry.size;
        if (entry.offset < indexEnd || end > data.size()) {
            throw core::FormatError("Package archive: data of '" + entry.name +
                                    "' lies outside the file");
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

} // namespace

PackageArchive PackageArchive::parse(std::vector<std::byte> data) {
    PackageArchive archive;
    archive.entries_ = readIndex(data);
    archive.data_ = std::move(data);
    return archive;
}

bool PackageArchive::looksLikePackage(std::span<const std::byte> data) {
    try {
        readIndex(data);
        return true;
    } catch (const core::FormatError&) {
        return false;
    }
}

const PackageEntry* PackageArchive::find(std::string_view name) const {
    const std::string wanted = core::toLower(name);
    for (const PackageEntry& entry : entries_) {
        if (core::toLower(entry.name) == wanted) {
            return &entry;
        }
    }
    return nullptr;
}

std::span<const std::byte> PackageArchive::entryData(const PackageEntry& entry) const {
    // Offsets were validated in readIndex(), so this cannot go out of range.
    return std::span<const std::byte>(data_).subspan(entry.offset, entry.size);
}

} // namespace openhogs::formats
