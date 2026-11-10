// Tests for formats::TextTable (.OFS + .BIN string tables).

#include "openhogs/formats/TextTable.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

using openhogs::core::FormatError;
using openhogs::formats::TextTable;
using openhogs::tests::ByteBuilder;

namespace {

// Two strings: "RIP" and "NO", each zero-terminated.
struct SampleTable {
    ByteBuilder offsets;
    ByteBuilder strings;
};

SampleTable makeSampleTable() {
    SampleTable table;
    table.strings.u8(0x33).u8(0x2A).u8(0x31).u8(0x00);  // R I P
    table.strings.u8(0x2F).u8(0x30).u8(0x00);           // N O
    table.offsets.u16(0).u16(4);
    return table;
}

} // namespace

// The engine looks strings up by index, so each offset must produce the
// string at that position.
TEST_CASE("TextTable decodes every string in index order", "[formats][TextTable]") {
    const auto sample = makeSampleTable();
    const auto table = TextTable::parse(sample.offsets.bytes(), sample.strings.bytes());

    REQUIRE(table.entries.size() == 2);
    CHECK(table.entries[0] == "RIP");
    CHECK(table.entries[1] == "NO");
}

// The Italian language pack will be built by editing the decoded text and
// encoding it again. Unchanged text must give back the original files
// exactly, otherwise the game could show the wrong strings.
TEST_CASE("TextTable re-encodes to the original bytes", "[formats][TextTable]") {
    const auto sample = makeSampleTable();
    const auto encoded = TextTable::parse(sample.offsets.bytes(), sample.strings.bytes()).encode();

    CHECK(encoded.offsets == sample.offsets.bytes());
    CHECK(encoded.strings == sample.strings.bytes());
}

// Translations are often longer than the original: offsets must be
// recomputed from the new lengths, not copied.
TEST_CASE("TextTable recomputes offsets for longer translations", "[formats][TextTable]") {
    TextTable table;
    table.entries = {"MORTO", "NO"};

    const auto encoded = table.encode();
    const auto reparsed = TextTable::parse(encoded.offsets, encoded.strings);
    CHECK(reparsed.entries == table.entries);
}

// A string without its terminating zero means a corrupt or truncated file.
TEST_CASE("TextTable rejects strings without a terminator", "[formats][TextTable]") {
    ByteBuilder offsets;
    offsets.u16(0);
    ByteBuilder strings;
    strings.u8(0x33).u8(0x2A);  // "RI" and no zero

    CHECK_THROWS_AS(TextTable::parse(offsets.bytes(), strings.bytes()), FormatError);
}
