// BitReader.cpp - see BitReader.h for an overview.

#include "openhogs/video/BitReader.h"

namespace openhogs::video {

std::uint32_t BitReader::peek(int count) const {
    if (count <= 0 || position_ < 0) return 0;
    // Gather the five bytes that hold the (up to 32) wanted bits; bytes
    // past the end count as zero.
    const auto firstByte = static_cast<std::size_t>(position_ >> 3);
    std::uint64_t window = 0;
    for (std::size_t k = 0; k < 5 && firstByte + k < data_.size(); ++k) {
        window |= static_cast<std::uint64_t>(std::to_integer<unsigned>(data_[firstByte + k])) << (8 * k);
    }
    const std::uint64_t mask = count >= 32 ? 0xFFFFFFFFull : ((1ull << count) - 1);
    return static_cast<std::uint32_t>((window >> (position_ & 7)) & mask);
}

std::uint32_t BitReader::read(int count) {
    const std::uint32_t value = peek(count);
    position_ += count;
    return value;
}

void BitReader::alignTo32() {
    const auto misalignment = position_ & 31;
    if (misalignment != 0) {
        position_ += 32 - misalignment;
    }
}

} // namespace openhogs::video
