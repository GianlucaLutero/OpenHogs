// Tests for core::BinaryReader.
//
// BinaryReader is the foundation of every file-format parser, so a bug here
// would silently corrupt everything read from the game. The tests focus on
// byte order, bounds checking and fixed-size fields.

#include "openhogs/core/BinaryReader.h"

#include <catch2/catch_test_macros.hpp>

#include <array>

using openhogs::core::BinaryReader;
using openhogs::core::FormatError;

namespace {

// Builds a byte array from integer literals, to keep test data readable.
template <typename... Values>
std::array<std::byte, sizeof...(Values)> bytes(Values... values) {
    return {static_cast<std::byte>(values)...};
}

} // namespace

// All game files were written on x86 and store numbers least-significant
// byte first. If the byte order were wrong, every offset and size read from
// an archive would be wrong too.
TEST_CASE("BinaryReader decodes little-endian integers", "[core][BinaryReader]") {
    const auto data = bytes(0x34, 0x12, 0x78, 0x56, 0x34, 0x12);
    BinaryReader reader(data);

    CHECK(reader.readU16() == 0x1234);
    CHECK(reader.readU32() == 0x12345678);
    CHECK(reader.remaining() == 0);
}

// Map coordinates and heights are stored as signed values; they must keep
// their sign instead of turning into huge positive numbers.
TEST_CASE("BinaryReader keeps the sign of signed integers", "[core][BinaryReader]") {
    const auto data = bytes(0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF);
    BinaryReader reader(data);

    CHECK(reader.readI16() == -16384);
    CHECK(reader.readI32() == -1);
}

// A truncated or badly modded file must produce a clear error, never a read
// outside the buffer (which would crash or return garbage).
TEST_CASE("BinaryReader refuses to read past the end of the data", "[core][BinaryReader]") {
    const auto data = bytes(0x01, 0x02, 0x03);
    BinaryReader reader(data);

    reader.skip(2);
    CHECK_THROWS_AS(reader.readU16(), FormatError);
    CHECK_THROWS_AS(reader.seek(4), FormatError);
    CHECK(reader.position() == 2);  // a failed read must not move the cursor
}

// Archive entries store names in fixed 16-byte fields padded with zeros.
// The text stops at the first zero, but the whole field must be consumed,
// otherwise every field after the name would be read from the wrong place.
TEST_CASE("BinaryReader reads zero-padded fixed-size strings", "[core][BinaryReader]") {
    const auto data = bytes('C', 'A', 'M', 'P', 0, 0, 0, 0, 0x2A, 0x00);
    BinaryReader reader(data);

    CHECK(reader.readFixedString(8) == "CAMP");
    CHECK(reader.position() == 8);
    CHECK(reader.readU16() == 42);
}
