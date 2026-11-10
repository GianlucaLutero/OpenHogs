// Tests for extractor::AssetExtractor.
//
// These run the extractor on a tiny fake game folder made of hand-built
// files, to check what users will actually find in the output folder.

#include "openhogs/extractor/AssetExtractor.h"

#include "ByteBuilder.h"
#include "TempDirectory.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>
#include <set>
#include <sstream>
#include <string>

using openhogs::core::VirtualFileSystem;
using openhogs::extractor::AssetExtractor;
using openhogs::tests::ByteBuilder;
using openhogs::tests::TempDirectory;

namespace fs = std::filesystem;

namespace {

void writeBinary(const fs::path& path, const ByteBuilder& data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.bytes().data()),
               static_cast<std::streamsize>(data.size()));
}

std::string readText(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// A 4x1, 4-bit TIM image with a 2-colour palette.
ByteBuilder makeTinyTim() {
    ByteBuilder tim;
    tim.u32(0x10).u32(0x08);                                 // 4-bit + palette
    tim.u32(12 + 4).u16(0).u16(0).u16(2).u16(1).u16(0).u16(0x001F);
    tim.u32(12 + 2).u16(0).u16(0).u16(1).u16(1).u8(0x10).u8(0x01);
    return tim;
}

} // namespace

// End-to-end check of the three conversions on one fake game folder:
// archive entries are written as files, textures become PNG, text tables
// become readable text with their indices.
TEST_CASE("AssetExtractor unpacks archives, textures and text tables", "[extractor]") {
    TempDirectory game;
    TempDirectory output;

    const ByteBuilder tim = makeTinyTim();
    ByteBuilder archive;
    archive.fixedString("BOX.TIM", 16).u32(24).u32(static_cast<std::uint32_t>(tim.size()));
    for (const auto b : tim.bytes()) {
        archive.u8(std::to_integer<std::uint8_t>(b));
    }
    writeBinary(game.path() / "Chars" / "WEAPONS.MTD", archive);

    ByteBuilder offsets;
    offsets.u16(0);
    ByteBuilder strings;
    strings.u8(0x33).u8(0x2A).u8(0x31).u8(0x00);  // "RIP"
    writeBinary(game.path() / "Language" / "Text" / "gtext.ofs", offsets);
    writeBinary(game.path() / "Language" / "Text" / "gtext.bin", strings);

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    const auto report = AssetExtractor(output.path()).extractAll(vfs);

    CHECK(report.problems.empty());
    CHECK(report.archives == 1);
    CHECK(report.images == 1);
    CHECK(report.textTables == 1);
    CHECK(fs::exists(output.path() / "chars/weapons.mtd/BOX.TIM"));
    CHECK(fs::exists(output.path() / "chars/weapons.mtd/BOX.TIM.png"));
    CHECK(readText(output.path() / "language/text/gtext.txt") == "[0000] RIP\n");
}

// One bad file (like Chars/mcap.mad, which is not an archive) must be
// reported and skipped, not abort the extraction of everything else.
TEST_CASE("AssetExtractor reports bad files and keeps going", "[extractor]") {
    TempDirectory game;
    TempDirectory output;
    game.writeFile("Chars/mcap.mad", "animation data, not an archive");
    writeBinary(game.path() / "Language/Tims/logo.tim", makeTinyTim());

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    const auto report = AssetExtractor(output.path()).extractAll(vfs);

    REQUIRE(report.problems.size() == 1);
    CHECK_THAT(report.problems[0], Catch::Matchers::ContainsSubstring("mcap.mad"));
    CHECK(report.images == 1);
}

// Entry names come from the archive file. A crafted or corrupted name such
// as "..\..\evil.dll" must never let the extractor write outside the
// output folder.
TEST_CASE("Archive entry names cannot escape the output folder", "[extractor]") {
    CHECK(AssetExtractor::makeSafeFileName("..\\..\\evil.dll") == ".._.._evil.dll");
    CHECK(AssetExtractor::makeSafeFileName("C:/Windows") == "C__Windows");
    CHECK(AssetExtractor::makeSafeFileName("..") == "_..");
    CHECK(AssetExtractor::makeSafeFileName("BOX.TIM") == "BOX.TIM");
}

// About 1,500 archive entries in the original data share a name with
// another entry of the same archive. Writing them under the same file name
// would silently lose data, so each repeat gets its own name. The check is
// case-insensitive because "BOX.TIM" and "box.tim" are one file on Windows.
TEST_CASE("Repeated entry names are kept as separate files", "[extractor]") {
    std::set<std::string> used;

    CHECK(AssetExtractor::makeUniqueFileName("BOX.TIM", used) == "BOX.TIM");
    CHECK(AssetExtractor::makeUniqueFileName("box.tim", used) == "box~2.tim");
    CHECK(AssetExtractor::makeUniqueFileName("BOX.TIM", used) == "BOX~3.TIM");
    CHECK(AssetExtractor::makeUniqueFileName("NOEXT", used) == "NOEXT");
    CHECK(AssetExtractor::makeUniqueFileName("NOEXT", used) == "NOEXT~2");
}
