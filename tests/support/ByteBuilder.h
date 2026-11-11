// ByteBuilder.h
//
// Test helper to build small binary files in memory, in the little-endian
// layout used by the original game. Tests use it to create hand-made
// samples instead of relying on copyrighted game files.

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace openhogs::tests {

class ByteBuilder {
public:
    ByteBuilder& u8(std::uint8_t value) {
        bytes_.push_back(static_cast<std::byte>(value));
        return *this;
    }

    ByteBuilder& u16(std::uint16_t value) {
        u8(static_cast<std::uint8_t>(value & 0xFF));
        return u8(static_cast<std::uint8_t>(value >> 8));
    }

    ByteBuilder& u32(std::uint32_t value) {
        u16(static_cast<std::uint16_t>(value & 0xFFFF));
        return u16(static_cast<std::uint16_t>(value >> 16));
    }

    ByteBuilder& i16(std::int16_t value) { return u16(static_cast<std::uint16_t>(value)); }
    ByteBuilder& i32(std::int32_t value) { return u32(static_cast<std::uint32_t>(value)); }
    ByteBuilder& f32(float value) { return u32(std::bit_cast<std::uint32_t>(value)); }

    // Writes `text` padded with zeros to exactly `length` bytes.
    ByteBuilder& fixedString(std::string_view text, std::size_t length) {
        for (std::size_t i = 0; i < length; ++i) {
            u8(i < text.size() ? static_cast<std::uint8_t>(text[i]) : 0);
        }
        return *this;
    }

    ByteBuilder& raw(std::string_view text) {
        for (const char c : text) {
            u8(static_cast<std::uint8_t>(c));
        }
        return *this;
    }

    std::size_t size() const { return bytes_.size(); }
    const std::vector<std::byte>& bytes() const { return bytes_; }

private:
    std::vector<std::byte> bytes_;
};

} // namespace openhogs::tests
