// TextTable.cpp - see TextTable.h for an overview.

#include "openhogs/formats/TextTable.h"

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/GameText.h"

#include <cstdint>
#include <cstdio>
#include <limits>
#include <sstream>

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

std::string TextTable::toListing() const {
    std::string text;
    char index[32];  // large enough for any std::size_t
    for (std::size_t i = 0; i < entries.size(); ++i) {
        std::snprintf(index, sizeof(index), "[%04zu] ", i);
        text += index;
        text += entries[i];
        text += '\n';
    }
    return text;
}

TextTable TextTable::fromListing(std::string_view listing) {
    TextTable table;
    std::istringstream stream{std::string(listing)};
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();  // files edited on Windows
        }
        if (line.empty()) {
            continue;
        }

        const auto close = line.find(']');
        const bool wellFormed = line.front() == '[' && close != std::string::npos && close > 1;
        std::size_t index = 0;
        bool numeric = wellFormed;
        for (std::size_t i = 1; numeric && i < close; ++i) {
            numeric = line[i] >= '0' && line[i] <= '9';
            index = index * 10 + static_cast<std::size_t>(line[i] - '0');
        }
        if (!numeric) {
            throw core::FormatError("Text listing line " + std::to_string(lineNumber) +
                                    ": expected \"[index] text\"");
        }
        if (index != table.entries.size()) {
            throw core::FormatError("Text listing line " + std::to_string(lineNumber) + ": expected index " +
                                    std::to_string(table.entries.size()) + ", found " + std::to_string(index));
        }

        // One space separates the index from the text; the rest is kept as
        // is (strings may legitimately start or end with spaces).
        std::string text = line.substr(close + 1);
        if (!text.empty() && text.front() == ' ') {
            text.erase(0, 1);
        }
        table.entries.push_back(std::move(text));
    }
    return table;
}

} // namespace openhogs::formats
