// BitReader.h
//
// Reads a byte buffer as a stream of bits, least significant bit first,
// the order Bink video and audio use. Reading past the end returns zero
// bits (and bitsLeft() goes negative), so a damaged stream cannot read
// outside its buffer; decoders check bitsLeft() where it matters.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace openhogs::video {

class BitReader {
public:
    explicit BitReader(std::span<const std::byte> data) : data_(data) {}

    // The next `count` bits (0 .. 32) as an unsigned number; the first bit
    // read is the least significant.
    std::uint32_t read(int count);
    bool readBit() { return read(1) != 0; }

    // The next `count` bits without moving on.
    std::uint32_t peek(int count) const;
    void skip(std::int64_t count) { position_ += count; }

    // Moves on to the next multiple of 32 bits.
    void alignTo32();

    std::int64_t position() const { return position_; }
    std::int64_t bitsLeft() const { return static_cast<std::int64_t>(data_.size()) * 8 - position_; }

private:
    std::span<const std::byte> data_;
    std::int64_t position_ = 0;
};

} // namespace openhogs::video
