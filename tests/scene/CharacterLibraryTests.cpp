// Tests for the character library: the class catalogue and loading the
// hogs' files through the virtual file system.
//
// The catalogue ties command-line names to the original file names, and
// the library is what every future game system will use to get a hog on
// screen. The tests use a miniature Chars/ folder built in memory, laid
// out like the game's (no original files are needed).

#include "openhogs/scene/CharacterLibrary.h"

#include "ByteBuilder.h"
#include "TempDirectory.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>
#include <utility>

using namespace openhogs;
using openhogs::tests::ByteBuilder;
using openhogs::tests::TempDirectory;

namespace {

void writeBinary(const std::filesystem::path& path, const std::vector<std::byte>& data) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

// A .MAD/.MTD archive: an index of (name[16], offset, size), then the data.
std::vector<std::byte> makeArchive(const std::vector<std::pair<std::string, ByteBuilder>>& files) {
    ByteBuilder archive;
    auto offset = static_cast<std::uint32_t>(files.size() * 24);
    for (const auto& [name, data] : files) {
        archive.fixedString(name, 16).u32(offset).u32(static_cast<std::uint32_t>(data.size()));
        offset += static_cast<std::uint32_t>(data.size());
    }
    for (const auto& [name, data] : files) {
        for (const auto b : data.bytes()) {
            archive.u8(std::to_integer<std::uint8_t>(b));
        }
    }
    return archive.bytes();
}

ByteBuilder makeSkeleton(std::size_t bones) {
    ByteBuilder hir;
    for (std::size_t i = 0; i < bones; ++i) {
        hir.i16(0).u16(0).i16(0).i16(static_cast<std::int16_t>(i == 0 ? 0 : -10)).i16(0);
        for (int k = 0; k < 5; ++k) hir.u16(0);
    }
    return hir;
}

// One animation of one frame with all angles zero.
ByteBuilder makeAnimations(std::size_t bones) {
    ByteBuilder mcap;
    const auto frameSize = static_cast<std::uint32_t>(32 + 16 * bones);
    mcap.u32(8).u32(frameSize);
    mcap.i32(0).i32(-100).i32(0);
    for (int k = 0; k < 5; ++k) mcap.i32(0);
    for (std::size_t b = 0; b < bones; ++b) mcap.f32(0).f32(0).f32(0).f32(1);
    return mcap;
}

// A one-triangle model whose vertices belong to the head (bone 2) and
// which uses texture 1 of the team archive.
std::vector<std::pair<std::string, ByteBuilder>> makeModelFiles(const std::string& name) {
    ByteBuilder vtx;
    vtx.i16(0).i16(0).i16(0).u16(2).i16(10).i16(0).i16(0).u16(2).i16(0).i16(10).i16(0).u16(2);
    ByteBuilder fac;
    fac.fixedString("", 16).u32(1);
    fac.u8(0).u8(0).u8(1).u8(0).u8(0).u8(1);  // U, V of the corners
    fac.u16(0).u16(1).u16(2);                 // vertices
    fac.u16(0).u16(0).u16(0);                 // normals
    fac.u16(0).u32(1).u16(0).u16(0).u16(0).u16(0);
    fac.u32(0);  // no quads
    return {{name + ".vtx", vtx}, {name + ".fac", fac}};
}

// A 4x1, 4-bit TIM image with a 2-colour palette.
ByteBuilder makeTinyTim() {
    ByteBuilder tim;
    tim.u32(0x10).u32(0x08);
    tim.u32(12 + 4).u16(0).u16(0).u16(2).u16(1).u16(0).u16(0x001F);
    tim.u32(12 + 2).u16(0).u16(0).u16(1).u16(1).u8(0x10).u8(0x01);
    return tim;
}

// A Chars/ folder with the skeleton, the animations, the grunt's high
// detail model and a two-texture "british" team.
void writeCharsFolder(const TempDirectory& game, std::size_t skeletonBones = 15) {
    writeBinary(game.path() / "Chars" / "pig.HIR", makeSkeleton(skeletonBones).bytes());
    writeBinary(game.path() / "Chars" / "mcap.mad", makeAnimations(skeletonBones).bytes());
    writeBinary(game.path() / "Chars" / "british.mad", makeArchive(makeModelFiles("pcgru_hi")));
    writeBinary(game.path() / "Chars" / "BRITISH.MTD", makeArchive({{"GRUN_000.TIM", makeTinyTim()},
                                                                    {"eyes000.tim", makeTinyTim()}}));
}

} // namespace

// Every class must name models that exist in british.mad, with the three
// levels of detail built from the right prefix; a wrong name here means a
// class that cannot be shown at all.
TEST_CASE("Character catalogue: model names and lookup by id", "[scene][characters]") {
    const auto* grunt = scene::findCharacterClass("Grunt");
    REQUIRE(grunt != nullptr);
    CHECK(scene::characterModelName(*grunt, scene::CharacterDetail::High) == "pcgru_hi");
    CHECK(scene::characterModelName(*grunt, scene::CharacterDetail::Medium) == "pcgru_me");
    CHECK(scene::characterModelName(*grunt, scene::CharacterDetail::Low) == "gr_hi");

    CHECK(scene::characterClasses().size() == 9);
    CHECK(scene::findCharacterClass("chef") == nullptr);
}

// Animation names come from the executable's list of 59; animations past
// the list must give an empty name, not read outside it.
TEST_CASE("Character catalogue: animation names", "[scene][characters]") {
    CHECK(scene::animationName(0) == "Run cycle (normal)");
    CHECK(scene::animationName(44) == "Salute");
    CHECK(scene::animationName(58) == "Parachuting");
    CHECK(scene::animationName(59).empty());
}

// The library reads the hog files through the VFS (so packs can replace
// them), with file names in any letter case, and keeps team textures in
// archive order because FAC texture indices count archive entries.
TEST_CASE("CharacterLibrary loads skeleton, animations, models and team textures", "[scene][characters]") {
    TempDirectory game;
    writeCharsFolder(game);
    core::VirtualFileSystem vfs;
    vfs.mount("game", game.path());

    const scene::CharacterLibrary library(vfs);
    CHECK(library.skeleton().bones.size() == 15);
    REQUIRE(library.animations().size() == 1);
    CHECK(library.animations()[0].frames[0].rootPosition[1] == -100);

    const auto model = library.loadModel(*scene::findCharacterClass("grunt"), scene::CharacterDetail::High);
    REQUIRE(model.vertices.size() == 3);
    CHECK(model.vertices[0].bone == 2);
    CHECK(model.faces[0].textureIndex == 1);

    const auto textures = library.loadTeamTextures("british");
    REQUIRE(textures.size() == 2);
    CHECK(scene::textureSizes(textures)[1] == glm::uvec2(4, 1));

    CHECK_THROWS(library.loadModel(*scene::findCharacterClass("spy"), scene::CharacterDetail::High));
    CHECK_THROWS(library.loadTeamTextures("martian"));
}

// The models' bone numbers are made for the hogs' 15-bone skeleton. A
// replacement skeleton with another bone count would scramble every hog,
// so it is refused with a clear error.
TEST_CASE("CharacterLibrary rejects a skeleton with the wrong number of bones", "[scene][characters]") {
    TempDirectory game;
    writeCharsFolder(game, 3);
    core::VirtualFileSystem vfs;
    vfs.mount("game", game.path());

    CHECK_THROWS_AS(scene::CharacterLibrary(vfs), std::runtime_error);
}
