// Tests for core::VirtualFileSystem.
//
// The layered file system is what lets content packs replace game files
// without modifying the installation, so these tests cover the override
// rules users rely on.

#include "openhogs/core/VirtualFileSystem.h"

#include "TempDirectory.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using openhogs::core::VirtualFileSystem;
using openhogs::tests::TempDirectory;

namespace {

std::string readText(const VirtualFileSystem& vfs, const std::string& path) {
    const auto data = vfs.readFile(path);
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

} // namespace

// Core rule of the pack system: a file in a higher layer replaces the same
// file in a lower layer, and the game folder itself is left untouched.
TEST_CASE("VirtualFileSystem prefers the most recently mounted layer", "[core][VFS]") {
    TempDirectory game;
    TempDirectory pack;
    game.writeFile("Speech/hello.wav", "english");
    pack.writeFile("Speech/hello.wav", "italiano");

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    vfs.mount("lang-it", pack.path());

    CHECK(readText(vfs, "Speech/hello.wav") == "italiano");
    CHECK(vfs.providingLayer("Speech/hello.wav") == "lang-it");
}

// Packs only contain the files they change; everything else must still be
// found in the original game folder below.
TEST_CASE("VirtualFileSystem falls back to lower layers", "[core][VFS]") {
    TempDirectory game;
    TempDirectory pack;
    game.writeFile("Maps/CAMP.MAD", "map");
    pack.writeFile("Speech/hello.wav", "italiano");

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    vfs.mount("lang-it", pack.path());

    CHECK(vfs.providingLayer("Maps/CAMP.MAD") == "game");
    CHECK_FALSE(vfs.exists("Maps/MISSING.MAD"));
    CHECK_THROWS(vfs.readFile("Maps/MISSING.MAD"));
}

// A pack author may name a file "speech/HELLO.WAV" while the game asks for
// "Speech\hello.wav": both must refer to the same file, on every OS.
TEST_CASE("VirtualFileSystem overrides files regardless of name spelling", "[core][VFS]") {
    TempDirectory game;
    TempDirectory pack;
    game.writeFile("Speech/Hello.wav", "english");
    pack.writeFile("speech/HELLO.WAV", "italiano");

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    vfs.mount("lang-it", pack.path());

    CHECK(readText(vfs, "SPEECH\\hello.wav") == "italiano");
}

// Listing a folder (e.g. all maps) must show each file once, even when it
// exists in several layers, and must include files added only by a pack.
TEST_CASE("VirtualFileSystem lists files merged across layers", "[core][VFS]") {
    TempDirectory game;
    TempDirectory pack;
    game.writeFile("Maps/CAMP.MAD", "a");
    game.writeFile("Maps/BAY.MAD", "b");
    pack.writeFile("Maps/CAMP.MAD", "c");
    pack.writeFile("Maps/NEWMAP.MAD", "d");
    pack.writeFile("Maps/.gitkeep", "");  // hidden files are not game data

    VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    vfs.mount("maps-pack", pack.path());

    const auto files = vfs.listFiles("maps");
    REQUIRE(files.size() == 3);
    CHECK(files[0] == "maps/bay.mad");
    CHECK(files[1] == "maps/camp.mad");
    CHECK(files[2] == "maps/newmap.mad");
}

// A wrong game path in the config must fail immediately with a clear error,
// not later as a confusing "file not found".
TEST_CASE("VirtualFileSystem rejects folders that do not exist", "[core][VFS]") {
    TempDirectory root;
    VirtualFileSystem vfs;
    CHECK_THROWS(vfs.mount("game", root.path() / "does-not-exist"));
}
