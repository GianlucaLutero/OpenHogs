// Tests for the language pack builder (src/tools/langpack).
//
// The builder takes text and pictures from the user's PlayStation disc and
// fits them into the PC layout. These tests use small hand-made discs and
// game folders, never the real copyrighted files.

#include "openhogs/langpack/Bitmap.h"
#include "openhogs/langpack/LanguagePackBuilder.h"
#include "openhogs/langpack/TextAlignment.h"
#include "openhogs/langpack/TranslationOverrides.h"

#include "ByteBuilder.h"
#include "IsoBuilder.h"
#include "TempDirectory.h"
#include "openhogs/content/ContentPack.h"
#include "openhogs/core/IniFile.h"
#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/TextTable.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>

using namespace openhogs;
using namespace openhogs::langpack;
using Catch::Matchers::ContainsSubstring;
using Strings = std::vector<std::string>;

namespace {

void writeBinary(const std::filesystem::path& path, const std::vector<std::byte>& data) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

formats::TextTable::EncodedFiles encodeTable(const Strings& entries) {
    formats::TextTable table;
    table.entries = entries;
    return table.encode();
}

} // namespace

// --- Text alignment ------------------------------------------------------------

// The PC menus have entries the PlayStation never had (network games), so
// indices drift. Translations must follow the English text to the right PC
// index, and PC-only entries must stay English and be reported.
TEST_CASE("Translations follow the English text onto the PC indices", "[langpack][alignment]") {
    const Strings pcEnglish{"YES", "NO", "ENTER IP ADDRESS", "OPTIONS", "QUIT"};
    const Strings psxEnglish{"YES", "NO", "OPTIONS", "QUIT"};
    const Strings psxItalian{"SÌ", "NO", "OPZIONI", "ESCI"};

    const auto result = alignTranslation(pcEnglish, psxEnglish, psxItalian);

    CHECK(result.entries == Strings{"SÌ", "NO", "ENTER IP ADDRESS", "OPZIONI", "ESCI"});
    CHECK(result.untranslated == std::vector<std::size_t>{2});
}

// Strings worded differently on the two versions (pad buttons versus keys)
// have no English match: they must keep the PC English rather than take a
// PlayStation sentence about the wrong controls. Untranslated PlayStation
// entries ("TEXT MISSING") must never replace a real string.
TEST_CASE("Differently worded and missing strings keep the PC English", "[langpack][alignment]") {
    const Strings pcEnglish{"PRESS SPACE TO FIRE.", "CHILL HILL", "RIP"};
    const Strings psxEnglish{"PRESS >X BUTTON TO FIRE.", "CHILL HILL", "RIP"};
    const Strings psxItalian{"PREMI >X PER FARE FUOCO.", "TEXT MISSING", "RIP"};

    const auto result = alignTranslation(pcEnglish, psxEnglish, psxItalian);

    CHECK(result.entries == Strings{"PRESS SPACE TO FIRE.", "CHILL HILL", "RIP"});
    CHECK(result.untranslated == std::vector<std::size_t>{0, 1});
}

// --- Overrides -----------------------------------------------------------------

// Hand-written translations fill the gaps left by the disc; an index
// outside the table means the file was written for another game version
// and must be refused instead of silently ignored.
TEST_CASE("Overrides replace listed strings and reject foreign indices", "[langpack][overrides]") {
    const auto overrides = TranslationOverrides::fromIni(core::IniFile::parse(
        "[gtext]\n1 = PREMI SPAZIO PER FARE FUOCO.\n[fetext]\n9 = TROPPO LONTANO\n"));

    Strings gtext{"RIP", "PRESS SPACE TO FIRE."};
    CHECK(overrides.apply("gtext", gtext) == 1);
    CHECK(gtext[1] == "PREMI SPAZIO PER FARE FUOCO.");

    Strings fetext{"YES", "NO"};
    CHECK_THROWS_WITH(overrides.apply("fetext", fetext), ContainsSubstring("outside the table"));

    CHECK_THROWS(TranslationOverrides::fromIni(core::IniFile::parse("[gtext]\nabc = X\n")));
}

// The overrides shipped with OpenHogs must always be valid: every value
// must be encodable by the game font, or building the Italian pack fails.
TEST_CASE("The shipped Italian overrides can be encoded", "[langpack][overrides]") {
    const auto ini = core::IniFile::load(std::filesystem::path(OPENHOGS_SOURCE_DIR) / "content/translations/it.ini");
    for (const char* table : {"gtext", "fetext"}) {
        const auto entries = ini.entries(table);
        CHECK_FALSE(entries.empty());
        for (const auto& [index, text] : entries) {
            INFO(table << " " << index << ": " << text);
            CHECK_NOTHROW(encodeTable({text}));
        }
    }
}

// --- Bitmaps -------------------------------------------------------------------

// The PC reads its pictures as bottom-up 24-bit BMPs with rows padded to 4
// bytes. A wrong row order would show the briefings upside down.
TEST_CASE("encodeBitmap24 writes a bottom-up 24-bit BMP", "[langpack][bitmap]") {
    formats::RgbaImage image{1, 2, {255, 0, 0, 255, 0, 0, 255, 255}};  // red on top, blue below

    const auto bmp = encodeBitmap24(image);

    const auto size = readBitmapSize(bmp);
    CHECK(size.width == 1);
    CHECK(size.height == 2);
    REQUIRE(bmp.size() == 54 + 2 * 4);  // headers + two rows of 3 bytes padded to 4
    CHECK(std::to_integer<int>(bmp[54]) == 255);      // first stored row = bottom = blue (B,G,R)
    CHECK(std::to_integer<int>(bmp[58 + 2]) == 255);  // second stored row = top = red
}

// The PC game ships one briefing (level24.BMP) with the old 12-byte OS/2
// header, where width and height are 16-bit. Reading it as a modern header
// gave a size of 31 million pixels and exhausted memory.
TEST_CASE("readBitmapSize understands the old OS/2 header", "[langpack][bitmap]") {
    tests::ByteBuilder bmp;
    bmp.raw("BM").u32(26 + 3).u32(0).u32(26);  // file header
    bmp.u32(12).u16(640).u16(480).u16(1).u16(24);  // BITMAPCOREHEADER
    bmp.u8(0).u8(0).u8(0);

    const auto size = readBitmapSize(bmp.bytes());
    CHECK(size.width == 640);
    CHECK(size.height == 480);

    tests::ByteBuilder absurd;  // a misread header must not allocate gigabytes
    absurd.raw("BM").u32(0).u32(0).u32(54).u32(40).u32(31457920).u32(1572865);
    CHECK_THROWS(readBitmapSize(absurd.bytes()));
}

// Briefings are scaled from 512 x 256 to the PC size; scaling must not
// change flat areas or shift the picture's corners.
TEST_CASE("resizeBilinear keeps flat colours and corners", "[langpack][bitmap]") {
    formats::RgbaImage image{2, 2, {10, 20, 30, 255, 10, 20, 30, 255, 200, 200, 200, 255, 200, 200, 200, 255}};

    const auto scaled = resizeBilinear(image, 5, 9);

    REQUIRE(scaled.pixels.size() == 5 * 9 * 4);
    CHECK(scaled.pixels[0] == 10);                     // top-left keeps the top colour
    CHECK(scaled.pixels[(8 * 5 + 4) * 4] == 200);      // bottom-right keeps the bottom colour
}

// --- Whole pack ----------------------------------------------------------------

// End to end on a miniature disc and PC folder: text is aligned and
// overridden, the briefing is converted to the PC picture's size and name,
// and the manifest makes a pack the content module selects for Italian.
TEST_CASE("buildLanguagePack creates a usable language pack", "[langpack][builder]") {
    tests::TempDirectory root;
    const auto game = root.makeFolder("game");

    // PC: game text, menu text with one PC-only entry, one briefing BMP.
    const auto pcGame = encodeTable({"RIP", "PRESS SPACE TO FIRE."});
    const auto pcMenu = encodeTable({"YES", "ENTER IP ADDRESS", "NO"});
    writeBinary(game / "Language/Text/gtext.ofs", pcGame.offsets);
    writeBinary(game / "Language/Text/gtext.bin", pcGame.strings);
    writeBinary(game / "Language/Text/fetext.ofs", pcMenu.offsets);
    writeBinary(game / "Language/Text/fetext.bin", pcMenu.strings);
    writeBinary(game / "Language/Tims/Briefing/Level1.BMP", encodeBitmap24({4, 3, std::vector<std::uint8_t>(48, 0)}));

    // Disc: English and Italian tables, one Italian briefing (512 x 256 white).
    tests::DiscFiles disc;
    const auto put = [&](const std::string& base, const Strings& entries) {
        const auto encoded = encodeTable(entries);
        disc[base + ".OFS"] = encoded.offsets;
        disc[base + ".BIN"] = encoded.strings;
    };
    put("TEXT/GENGLISH", {"RIP", "PRESS >X BUTTON TO FIRE."});
    put("TEXT/GITALIAN", {"RIP", "PREMI >X PER FARE FUOCO."});
    put("TEXT/FETEXT/ENGLISH", {"YES", "NO"});
    put("TEXT/FETEXT/ITALIAN", {"SÌ", "NO"});
    disc["BRIEF/ITALIAN/LEVEL1.BIN"] = std::vector<std::byte>(512 * 256 * 2, std::byte{0xFF});
    tests::writeIsoImage(root.path() / "disc.bin", disc, true);

    const auto overrides = root.writeFile("it.ini", "[gtext]\n1 = PREMI SPAZIO PER FARE FUOCO.\n");

    LanguagePackOptions options;
    options.gameDirectory = game;
    options.discImage = root.path() / "disc.bin";
    options.language = "it";
    options.outputDirectory = root.path() / "packs/lang-it";
    options.overridesFile = overrides;
    const auto report = buildLanguagePack(options);

    CHECK(report.gameText.overridden == 1);
    CHECK(report.gameText.untranslated.empty());
    CHECK(report.menuText.untranslated == std::vector<std::size_t>{1});  // the PC-only entry
    CHECK(report.briefings == 1);

    core::VirtualFileSystem pack;
    pack.mount("lang-it", options.outputDirectory / "data");
    const auto gtext = formats::TextTable::parse(pack.readFile("Language/Text/gtext.ofs"),
                                                 pack.readFile("Language/Text/gtext.bin"));
    CHECK(gtext.entries == Strings{"RIP", "PREMI SPAZIO PER FARE FUOCO."});
    const auto fetext = formats::TextTable::parse(pack.readFile("Language/Text/fetext.ofs"),
                                                  pack.readFile("Language/Text/fetext.bin"));
    CHECK(fetext.entries == Strings{"SÌ", "ENTER IP ADDRESS", "NO"});

    const auto briefing = readBitmapSize(pack.readFile("Language/Tims/Briefing/Level1.BMP"));
    CHECK(briefing.width == 4);
    CHECK(briefing.height == 3);

    const auto manifest = content::ContentPack::load(options.outputDirectory);
    CHECK(manifest.id == "lang-it");
    CHECK(manifest.type == content::PackType::Language);
    CHECK(manifest.language == "it");
}

// A disc without the requested language (another release, or a language
// the PlayStation version does not have) must fail with an explanation.
TEST_CASE("buildLanguagePack explains unusable discs and languages", "[langpack][builder]") {
    tests::TempDirectory root;
    tests::writeIsoImage(root.path() / "other.bin", {{"README.TXT", tests::toBytes("hello")}}, true);

    LanguagePackOptions options;
    options.gameDirectory = root.makeFolder("game");
    options.discImage = root.path() / "other.bin";
    options.outputDirectory = root.path() / "out";

    CHECK_THROWS_WITH(buildLanguagePack(options), ContainsSubstring("SLES-01041"));

    options.language = "ja";
    CHECK_THROWS_WITH(buildLanguagePack(options), ContainsSubstring("not on the PlayStation disc"));
}
