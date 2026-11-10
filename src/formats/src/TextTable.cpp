// TextTable.cpp - see TextTable.h for an overview.

#include "openhogs/formats/TextTable.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/GameText.h"

#include <cstdint>
#include <limits>

namespace openhogs::formats {

TextTable TextTable::parse(std::span<const std::byte> offsets, std::span<const std::byte> strings) {
    if (offsets.size() % 2 != 0) {
        throw core::FormatError("Text table: .ofs size must be a multiple of 2");
    }

    TextTable table;
    core::BinaryReader offsetReader(offsets);

    while (offsetReader.remaining() > 0) {
        const std::size_t start = offsetReader.readU16();
        const std::size_t index = table.entries.size();

        // Find the terminating zero of this string.
        std::size_t end = start;
        while (end < strings.size() && strings[end] != std::byte{0}) {
            ++end;
        }
        if (end >= strings.size()) {
            throw core::FormatError("Text table: string " + std::to_string(index) +
                                    " at offset " + std::to_string(start) +
                                    " has no terminating zero inside the .bin file");
        }

        table.entries.push_back(decodeGameText(strings.subspan(start, end - start)));
    }
    return table;
}

TextTable::EncodedFiles TextTable::encode() const {
    EncodedFiles files;

    for (std::size_t index = 0; index < entries.size(); ++index) {
        const std::size_t offset = files.strings.size();
        if (offset > std::numeric_limits<std::uint16_t>::max()) {
            throw core::FormatError("Text table too large: string " + std::to_string(index) +
                                    " starts beyond 64 KiB, which 16-bit offsets cannot address");
        }

        // Little-endian 16-bit offset.
        files.offsets.push_back(static_cast<std::byte>(offset & 0xFF));
        files.offsets.push_back(static_cast<std::byte>((offset >> 8) & 0xFF));

        const auto encoded = encodeGameText(entries[index]);
        files.strings.insert(files.strings.end(), encoded.begin(), encoded.end());
        files.strings.push_back(std::byte{0});
    }
    return files;
}

} // namespace openhogs::formats
