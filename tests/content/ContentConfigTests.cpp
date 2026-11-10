// Tests for content::ContentConfig (config/openhogs.ini).

#include "openhogs/content/ContentConfig.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

#include <catch2/catch_test_macros.hpp>

using openhogs::content::ContentConfig;
using openhogs::core::FormatError;
using openhogs::core::IniFile;

namespace fs = std::filesystem;

// Relative paths are resolved against the config folder, so the default
// "../content/packs" works wherever the user unzips OpenHogs, regardless of
// the folder the program is started from.
TEST_CASE("ContentConfig resolves relative paths against the config folder", "[content][ContentConfig]") {
    const auto ini = IniFile::parse("[Game]\nDirectory = ../game\n");
    const fs::path configFolder = fs::path("openhogs") / "config";

    const auto config = ContentConfig::fromIni(ini, configFolder);

    CHECK(config.gameDirectory == fs::path("openhogs") / "game");
    CHECK(config.packsDirectory == fs::path("openhogs") / "content" / "packs");
    CHECK(config.language == "en");  // the original language by default
}

// Without the game folder nothing can work: the error must say which
// setting to fill in rather than failing later with a missing file.
TEST_CASE("ContentConfig requires the game directory", "[content][ContentConfig]") {
    CHECK_THROWS_AS(ContentConfig::fromIni(IniFile::parse("[Content]\nLanguage = it\n"), "."),
                    FormatError);
}

// Language codes and pack ids are matched in lower case elsewhere; the
// config must normalise them so "IT" or "HD-Music" still work.
TEST_CASE("ContentConfig normalises language and pack ids", "[content][ContentConfig]") {
    const auto ini = IniFile::parse(
        "[Game]\nDirectory = /games/hogs\n"
        "[Content]\nLanguage = IT\nEnabledPacks = HD-Music, ps1-ai\n");

    const auto config = ContentConfig::fromIni(ini, ".");

    CHECK(config.language == "it");
    REQUIRE(config.enabledPacks.size() == 2);
    CHECK(config.enabledPacks[0] == "hd-music");
}
