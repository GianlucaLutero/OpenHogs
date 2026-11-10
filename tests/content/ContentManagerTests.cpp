// Tests for the content manager: pack discovery, selection and mounting.
//
// These tests describe the user-visible promise of the content module:
// packs can be switched on and off from openhogs.ini, the right language
// is used, and the original game files are back as soon as a pack is off.

#include "openhogs/content/ContentManager.h"

#include "TempDirectory.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace openhogs::content;
using openhogs::tests::TempDirectory;
using Catch::Matchers::ContainsSubstring;

namespace {

ContentPack makePack(std::string id, PackType type, int priority = 0, std::string language = {}) {
    ContentPack pack;
    pack.id = std::move(id);
    pack.type = type;
    pack.priority = priority;
    pack.language = std::move(language);
    return pack;
}

std::vector<std::string> idsOf(const std::vector<ContentPack>& packs) {
    std::vector<std::string> ids;
    for (const auto& pack : packs) {
        ids.push_back(pack.id);
    }
    return ids;
}

std::string readText(const openhogs::core::VirtualFileSystem& vfs, const std::string& path) {
    const auto data = vfs.readFile(path);
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

} // namespace

// --- Selection rules ---------------------------------------------------------

// Optional packs are on only when listed in EnabledPacks: this is the
// "switch features on and off at will" requirement.
TEST_CASE("Only packs listed in EnabledPacks are active", "[content][ContentManager]") {
    const std::vector<ContentPack> available{
        makePack("hd-music", PackType::Music),
        makePack("ps1-ai", PackType::Misc),
    };
    ContentConfig config;
    config.enabledPacks = {"ps1-ai"};

    CHECK(idsOf(selectActivePacks(config, available)) == std::vector<std::string>{"ps1-ai"});

    config.enabledPacks.clear();
    CHECK(selectActivePacks(config, available).empty());
}

// The chosen language activates its pack(s) and no other language. English
// is the language of the original files and needs no pack.
TEST_CASE("The selected language activates the matching language pack", "[content][ContentManager]") {
    const std::vector<ContentPack> available{
        makePack("lang-it", PackType::Language, 100, "it"),
        makePack("lang-de", PackType::Language, 100, "de"),
    };
    ContentConfig config;

    config.language = "it";
    CHECK(idsOf(selectActivePacks(config, available)) == std::vector<std::string>{"lang-it"});

    config.language = "en";
    CHECK(selectActivePacks(config, available).empty());
}

// Mistakes in openhogs.ini must stop with an explanation instead of
// starting the game with content silently missing.
TEST_CASE("Invalid content selections are reported clearly", "[content][ContentManager]") {
    const std::vector<ContentPack> available{makePack("lang-it", PackType::Language, 0, "it")};
    ContentConfig config;

    config.enabledPacks = {"typo-pack"};
    CHECK_THROWS_WITH(selectActivePacks(config, available), ContainsSubstring("typo-pack"));

    config.enabledPacks = {"lang-it"};  // languages are chosen with Language =
    CHECK_THROWS_WITH(selectActivePacks(config, available), ContainsSubstring("Language = it"));

    config.enabledPacks.clear();
    config.language = "fr";
    CHECK_THROWS_WITH(selectActivePacks(config, available), ContainsSubstring("'fr'"));
}

// When two packs replace the same file, the one with the higher Priority
// must win, i.e. be mounted last. Ties are broken by id so the result never
// depends on the order of folders on disk.
TEST_CASE("Active packs are ordered by priority, then id", "[content][ContentManager]") {
    const std::vector<ContentPack> available{
        makePack("zeta", PackType::Misc, 5),
        makePack("alpha", PackType::Misc, 5),
        makePack("top", PackType::Misc, 50),
        makePack("lang-it", PackType::Language, 10, "it"),
    };
    ContentConfig config;
    config.language = "it";
    config.enabledPacks = {"top", "zeta", "alpha"};

    CHECK(idsOf(selectActivePacks(config, available)) ==
          std::vector<std::string>{"alpha", "zeta", "lang-it", "top"});
}

// --- Discovery ---------------------------------------------------------------

// Template folders ("_template") must not appear as real packs, and two packs
// with the same id would make EnabledPacks ambiguous.
TEST_CASE("discoverPacks skips templates and rejects duplicate ids", "[content][ContentManager]") {
    TempDirectory packs;
    packs.writeFile("hd-music/pack.ini", "[Pack]\nId = hd-music\nType = music\n");
    packs.writeFile("_template/pack.ini", "[Pack]\nId = template\n");
    packs.makeFolder("not-a-pack");

    CHECK(idsOf(discoverPacks(packs.path())) == std::vector<std::string>{"hd-music"});

    packs.writeFile("hd-music-copy/pack.ini", "[Pack]\nId = hd-music\n");
    CHECK_THROWS_WITH(discoverPacks(packs.path()), ContainsSubstring("same Id"));
}

// --- End to end --------------------------------------------------------------

// The whole promise in one test: with the pack on, its file replaces the
// original; with the pack off, the original file is used again.
TEST_CASE("Turning a pack off restores the original game file", "[content][ContentManager]") {
    TempDirectory root;
    root.writeFile("game/Speech/Sku1/Sarge/SGEN0101.wav", "english");
    root.writeFile("packs/lang-it/pack.ini",
                   "[Pack]\nId = lang-it\nType = language\nLanguage = it\n");
    root.writeFile("packs/lang-it/data/Speech/Sku1/Sarge/SGEN0101.wav", "italiano");

    ContentConfig config;
    config.gameDirectory = root.path() / "game";
    config.packsDirectory = root.path() / "packs";
    const auto available = discoverPacks(config.packsDirectory);

    config.language = "it";
    const auto italian = buildFileSystem(config, selectActivePacks(config, available));
    CHECK(readText(italian, "Speech/Sku1/Sarge/SGEN0101.wav") == "italiano");

    config.language = "en";
    const auto original = buildFileSystem(config, selectActivePacks(config, available));
    CHECK(readText(original, "Speech/Sku1/Sarge/SGEN0101.wav") == "english");
}
