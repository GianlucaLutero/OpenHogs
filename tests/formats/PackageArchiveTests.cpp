// Tests for formats::PackageArchive (.MAD / .MTD).
//
// Archives hold almost every model and texture of the game. The tests use a
// hand-made archive with the same layout as the real files.

#include "openhogs/formats/PackageArchive.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

#include <string>

using openhogs::core::FormatError;
using openhogs::formats::PackageArchive;
using openhogs::tests::ByteBuilder;

namespace {

// Two entries: "STF02PPP.VTX" -> "abc", "BOX.TIM" -> "defgh".
// Index = 2 * 24 bytes, so data starts at offset 48.
ByteBuilder makeSampleArchive() {
    ByteBuilder archive;
    archive.fixedString("STF02PPP.VTX", 16).u32(48).u32(3);
    archive.fixedString("BOX.TIM", 16).u32(51).u32(5);
    archive.raw("abc").raw("defgh");
    return archive;
}

std::string asText(std::span<const std::byte> data) {
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

} // namespace

// The format has no entry count: the index must end exactly where the first
// file's data starts. Getting this wrong would read data as index entries.
TEST_CASE("PackageArchive reads every entry and its data", "[formats][PackageArchive]") {
    const auto archive = PackageArchive::parse(makeSampleArchive().bytes());

    REQUIRE(archive.entries().size() == 2);
    CHECK(archive.entries()[0].name == "STF02PPP.VTX");
    CHECK(asText(archive.entryData(archive.entries()[0])) == "abc");
    CHECK(asText(archive.entryData(archive.entries()[1])) == "defgh");
}

// The game refers to entries with inconsistent case, as Windows allowed.
TEST_CASE("PackageArchive finds entries regardless of case", "[formats][PackageArchive]") {
    const auto archive = PackageArchive::parse(makeSampleArchive().bytes());

    REQUIRE(archive.find("box.tim") != nullptr);
    CHECK(archive.find("box.tim")->size == 5);
    CHECK(archive.find("missing.tim") == nullptr);
}

// An entry pointing past the end of the file (truncated download, broken
// mod) must be rejected when the archive is opened, not crash later.
TEST_CASE("PackageArchive rejects entries outside the file", "[formats][PackageArchive]") {
    ByteBuilder archive;
    archive.fixedString("BIG.TIM", 16).u32(24).u32(1000);
    archive.raw("tiny");

    CHECK_THROWS_AS(PackageArchive::parse(archive.bytes()), FormatError);
}

// Chars/mcap.mad uses the archive extension but holds animation data. The
// extractor relies on looksLikePackage() to skip it instead of failing.
TEST_CASE("PackageArchive recognises files that are not archives", "[formats][PackageArchive]") {
    CHECK(PackageArchive::looksLikePackage(makeSampleArchive().bytes()));

    ByteBuilder animation;  // index size not a multiple of 24 -> not an archive
    animation.u32(0x12345678).u32(1).u32(2).u32(3).u32(10808).u32(5);
    CHECK_FALSE(PackageArchive::looksLikePackage(animation.bytes()));
}
