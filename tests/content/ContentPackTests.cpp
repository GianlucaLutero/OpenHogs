// Tests for content::ContentPack (pack.ini manifests).
//
// Manifests are written by pack authors, so the important behaviour is that
// a valid manifest is read completely and an invalid one is rejected with a
// message that says what to fix.

#include "openhogs/content/ContentPack.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace openhogs::content;
using openhogs::core::FormatError;
using openhogs::core::IniFile;
using Catch::Matchers::ContainsSubstring;

// The full set of fields a pack author can set must reach the program,
// normalised to lower case where they are compared (id, language).
TEST_CASE("ContentPack reads a complete language manifest", "[content][ContentPack]") {
    const auto manifest = IniFile::parse(
        "[Pack]\n"
        "Id = Lang-IT\n"
        "Name = Italian language\n"
        "Type = Language\n"
        "Language = IT\n"
        "Priority = 100\n"
        "Author = Someone\n"
        "Version = 1.0\n");

    const auto pack = ContentPack::fromManifest(manifest, "packs/lang-it");

    CHECK(pack.id == "lang-it");
    CHECK(pack.name == "Italian language");
    CHECK(pack.type == PackType::Language);
    CHECK(pack.language == "it");
    CHECK(pack.priority == 100);
    CHECK(pack.dataDirectory == std::filesystem::path("packs/lang-it") / "data");
}

// A language pack without a language could never be selected; reporting it
// up front saves the author from a pack that silently does nothing.
TEST_CASE("ContentPack rejects a language pack without Language", "[content][ContentPack]") {
    const auto manifest = IniFile::parse("[Pack]\nId = lang-x\nType = language\n");
    CHECK_THROWS_WITH(ContentPack::fromManifest(manifest, "packs/lang-x"),
                      ContainsSubstring("must set Language"));
}

// Ids are typed by users into EnabledPacks; spaces or commas would break
// that list, so they are refused when the pack is loaded.
TEST_CASE("ContentPack rejects ids that cannot be used in EnabledPacks", "[content][ContentPack]") {
    CHECK_THROWS_AS(ContentPack::fromManifest(IniFile::parse("[Pack]\nId = hd music\n"), "p"),
                    FormatError);
    CHECK_THROWS_AS(ContentPack::fromManifest(IniFile::parse("[Pack]\nName = no id\n"), "p"),
                    FormatError);
}

// A misspelled type ("musik") must be reported, not silently treated as misc.
TEST_CASE("ContentPack rejects unknown pack types", "[content][ContentPack]") {
    const auto manifest = IniFile::parse("[Pack]\nId = hd-music\nType = musik\n");
    CHECK_THROWS_WITH(ContentPack::fromManifest(manifest, "p"), ContainsSubstring("musik"));
}
