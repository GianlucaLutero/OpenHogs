// Tests for core::IniFile.
//
// INI files are written by hand by users and pack authors, so the parser
// must be forgiving where Windows is forgiving (case, spacing) and give
// precise errors where the file is really wrong.

#include "openhogs/core/IniFile.h"

#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using openhogs::core::FormatError;
using openhogs::core::IniFile;
using openhogs::core::setIniValue;

// Users copy settings from guides with any capitalisation ("languagecode",
// "LanguageCode"), as the original Windows API allowed. Lookups must not
// depend on case.
TEST_CASE("IniFile lookups ignore the case of sections and keys", "[core][IniFile]") {
    const auto ini = IniFile::parse("[Localisation]\nLanguageCode = 9\n");

    CHECK(ini.getInt("localisation", "languagecode", 0) == 9);
    CHECK(ini.getInt("LOCALISATION", "LANGUAGECODE", 0) == 9);
}

// The most common value is a Windows install path. Inner spaces, brackets
// and backslashes must survive untouched, or the game folder is not found.
TEST_CASE("IniFile keeps Windows paths intact and skips comments", "[core][IniFile]") {
    const auto ini = IniFile::parse(
        "; OpenHogs settings\n"
        "# another comment style\n"
        "[Game]\n"
        "Directory =   C:\\Program Files (x86)\\Steam\\steamapps\\common\\HogsofWar   \n");

    CHECK(ini.getString("Game", "Directory") ==
          "C:\\Program Files (x86)\\Steam\\steamapps\\common\\HogsofWar");
}

// A typo in a config file must point the user to the exact line.
TEST_CASE("IniFile reports the line number of malformed lines", "[core][IniFile]") {
    using Catch::Matchers::ContainsSubstring;

    CHECK_THROWS_WITH(IniFile::parse("[Game]\nDirectory C:\\Games\n"), ContainsSubstring("line 2"));
    CHECK_THROWS_WITH(IniFile::parse("[Game\n"), ContainsSubstring("line 1"));
    CHECK_THROWS_WITH(IniFile::parse("Key = 1\n"), ContainsSubstring("before any [Section]"));
}

// "Priority = 10x" is almost certainly a typo: silently reading 10 would
// hide it, so numbers must be complete. Missing keys use the default.
TEST_CASE("IniFile parses whole numbers strictly", "[core][IniFile]") {
    const auto ini = IniFile::parse("[Pack]\nPriority = 10x\n");

    CHECK_THROWS_AS(ini.getInt("Pack", "Priority", 0), FormatError);
    CHECK(ini.getInt("Pack", "Missing", 7) == 7);
}

// Enabled packs are written as a comma-separated list; stray spaces and
// trailing commas are common when editing by hand.
TEST_CASE("IniFile splits comma-separated lists", "[core][IniFile]") {
    const auto ini = IniFile::parse("[Content]\nEnabledPacks = hd-music ,  ps1-ai,, \n");

    const auto packs = ini.getList("Content", "EnabledPacks");
    REQUIRE(packs.size() == 2);
    CHECK(packs[0] == "hd-music");
    CHECK(packs[1] == "ps1-ai");
    CHECK(ini.getList("Content", "Missing").empty());
}

// Feature switches accept the usual spellings; anything else is an error
// rather than a silent "off".
TEST_CASE("IniFile understands common boolean spellings", "[core][IniFile]") {
    const auto ini = IniFile::parse("[F]\nA = yes\nB = Off\nC = 1\nD = maybe\n");

    CHECK(ini.getBool("F", "A", false));
    CHECK_FALSE(ini.getBool("F", "B", true));
    CHECK(ini.getBool("F", "C", false));
    CHECK_THROWS_AS(ini.getBool("F", "D", false), FormatError);
}

// Volumes and keys changed in the game are written into the user's own
// openhogs.ini. The file is full of comments explaining every setting:
// saving must change only the value asked for, never lose the comments or
// another section's key of the same name.
TEST_CASE("setIniValue replaces a value and keeps everything else", "[core][IniFile]") {
    const std::string text = "; volumes\n[Audio]\n; full is 100\nmastervolume = 100\nSoundMode = stereo\n\n"
                             "[Other]\nMasterVolume = 7\n";
    const std::string result = setIniValue(text, "audio", "MasterVolume", "60");

    CHECK(result == "; volumes\n[Audio]\n; full is 100\nMasterVolume = 60\nSoundMode = stereo\n\n"
                    "[Other]\nMasterVolume = 7\n");
    CHECK(IniFile::parse(result).getInt("Other", "MasterVolume", 0) == 7);
}

// The first save of a key the file never had (an old openhogs.ini without
// [Controls], say) must still work: the key goes into its section, or a
// new section at the end.
TEST_CASE("setIniValue adds a missing key or section", "[core][IniFile]") {
    const std::string text = "[Audio]\nMasterVolume = 100\n\n; the end\n[Game]\nVersion = pc\n";

    const auto withKey = IniFile::parse(setIniValue(text, "Audio", "MusicVolume", "40"));
    CHECK(withKey.getInt("Audio", "MusicVolume", 0) == 40);

    const std::string withSection = setIniValue(text, "Controls", "Jump", "Space");
    CHECK(withSection.ends_with("\n\n[Controls]\nJump = Space\n"));
    CHECK(IniFile::parse(withSection).getString("Game", "Version") == "pc");
    CHECK(IniFile::parse(setIniValue("", "Controls", "Jump", "Space")).getString("Controls", "Jump") == "Space");
}
