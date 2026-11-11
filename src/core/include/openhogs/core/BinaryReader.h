// BinaryReader.h
//
// Safe sequential reader for the binary files of the original game.
//
// Every Hogs of War format (archives, models, maps, text tables) is a packed
// little-endian structure written by a 32-bit PC in 1999. Reading such data
// with raw pointer casts is fragile: a truncated or modded file would read
// past the end of the buffer. BinaryReader keeps a cursor over a byte buffer
// and checks every read, throwing FormatError instead of reading garbage.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace openhogs::core {

// Thrown when a file does not have the structure we expect
// (too short, wrong header, offsets pointing outside the file...).
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class BinaryReader {
public:
    // The reader does not own the data: the buffer must outlive the reader.
    explicit BinaryReader(std::span<const std::byte> data);

    // --- Fixed-size little-endian integers -------------------------------
    std::uint8_t readU8();
    std::uint16_t readU16();
    std::uint32_t readU32();
    std::int16_t readI16();
    std::int32_t readI32();

    // IEEE 754 single-precision float, stored little-endian (PC files only:
    // the PlayStation had no floating point).
    float readF32();

    // Reads a fixed-length text field (e.g. a 16-byte file name).
    // The original tools padded names with zero bytes, so everything from
    // the first zero byte onwards is dropped.
    std::string readFixedString(std::size_t length);

    // Copies `count` raw bytes.
    std::vector<std::byte> readBytes(std::size_t count);

    // --- Cursor control ---------------------------------------------------
    void seek(std::size_t position);
    void skip(std::size_t count);

    std::size_t position() const { return position_; }
    std::size_t size() const { return data_.size(); }
    std::size_t remaining() const { return data_.size() - position_; }

private:
    // Throws FormatError if fewer than `count` bytes are left.
    void requireBytes(std::size_t count) const;

    std::span<const std::byte> data_;
    std::size_t position_ = 0;
};

} // namespace openhogs::core
