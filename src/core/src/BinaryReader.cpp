// BinaryReader.cpp - see BinaryReader.h for an overview.

#include "openhogs/core/BinaryReader.h"

namespace openhogs::core {

BinaryReader::BinaryReader(std::span<const std::byte> data)
    : data_(data) {}

void BinaryReader::requireBytes(std::size_t count) const {
    if (count > remaining()) {
        throw FormatError("Unexpected end of data: needed " + std::to_string(count) +
                          " bytes at offset " + std::to_string(position_) + ", but only " +
                          std::to_string(remaining()) + " are left");
    }
}

std::uint8_t BinaryReader::readU8() {
    requireBytes(1);
    return std::to_integer<std::uint8_t>(data_[position_++]);
}

// Multi-byte values are assembled byte by byte (least significant first).
// This is independent of the CPU we run on, unlike a reinterpret_cast.
// The size is checked once up front so that a failed read leaves the
// cursor where it was.
std::uint16_t BinaryReader::readU16() {
    requireBytes(2);
    const std::uint16_t low = readU8();
    const std::uint16_t high = readU8();
    return static_cast<std::uint16_t>(low | (high << 8));
}

std::uint32_t BinaryReader::readU32() {
    requireBytes(4);
    const std::uint32_t low = readU16();
    const std::uint32_t high = readU16();
    return low | (high << 16);
}

std::int16_t BinaryReader::readI16() {
    return static_cast<std::int16_t>(readU16());
}

std::int32_t BinaryReader::readI32() {
    return static_cast<std::int32_t>(readU32());
}

std::string BinaryReader::readFixedString(std::size_t length) {
    requireBytes(length);

    std::string text;
    for (std::size_t i = 0; i < length; ++i) {
        const char c = static_cast<char>(data_[position_ + i]);
        if (c == '\0') {
            break;
        }
        text.push_back(c);
    }

    position_ += length;  // always consume the whole field, padding included
    return text;
}

std::vector<std::byte> BinaryReader::readBytes(std::size_t count) {
    requireBytes(count);
    const auto first = data_.begin() + static_cast<std::ptrdiff_t>(position_);
    std::vector<std::byte> bytes(first, first + static_cast<std::ptrdiff_t>(count));
    position_ += count;
    return bytes;
}

void BinaryReader::seek(std::size_t position) {
    if (position > data_.size()) {
        throw FormatError("Seek to offset " + std::to_string(position) +
                          " is past the end of data (" + std::to_string(data_.size()) + " bytes)");
    }
    position_ = position;
}

void BinaryReader::skip(std::size_t count) {
    requireBytes(count);
    position_ += count;
}

} // namespace openhogs::core
