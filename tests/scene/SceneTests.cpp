// Tests for the scene module: how map data becomes world geometry.
//
// The conventions tested here (row direction inside a block, tile texture
// rotation, model scale and axis flip) were found by comparing with the
// original game data. Each one, if broken, visibly misplaces the map.

#include "openhogs/scene/MapScene.h"
#include "openhogs/scene/ModelMesh.h"
#include "openhogs/scene/TerrainMesh.h"

#include "ByteBuilder.h"
#include "TempDirectory.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>

using namespace openhogs;
using Catch::Approx;

namespace {

// Rounds to avoid comparing floating-point values exactly.
glm::ivec3 rounded(const glm::vec3& v) {
    return glm::ivec3(glm::round(v));
}

} // namespace

// --- Terrain -----------------------------------------------------------------

// Inside a block, sample row 0 is at the far edge (zOffset + 2048) and rows
// move towards zOffset. With the opposite direction the terrain is mirrored
// inside each block and objects float or sink (checked on several maps).
TEST_CASE("Terrain samples are placed with rows going towards -Z", "[scene][terrain]") {
    formats::TerrainBlock block;
    block.xOffset = -16384;
    block.zOffset = 14336;
    block.heights[0] = 300;                   // column 0, row 0
    block.heights[4 * 5 + 4] = 700;           // column 4, row 4

    CHECK(rounded(scene::terrainSamplePosition(block, 0, 0)) == glm::ivec3(-16384, 300, 16384));
    CHECK(rounded(scene::terrainSamplePosition(block, 4, 4)) == glm::ivec3(-14336, 700, 14336));
}

// Tile textures are rotated and flipped to hide repetition; paths and
// shorelines only line up across tiles when these flags are applied right.
TEST_CASE("Tile texture coordinates follow the flip and rotation flags", "[scene][terrain]") {
    using formats::TileOrientation::FlipX;
    using formats::TileOrientation::Rotate180;
    using formats::TileOrientation::Rotate90;

    const auto plain = scene::tileTextureCoordinates(0);
    CHECK(plain[0] == glm::vec2(0, 0));
    CHECK(plain[3] == glm::vec2(1, 1));

    const auto flipped = scene::tileTextureCoordinates(FlipX);
    CHECK(flipped[0] == glm::vec2(1, 0));

    // A quarter turn moves each corner's texture coordinate one step around
    // the tile; four quarter turns (90 + 180 + 90) would be the identity.
    const auto quarter = scene::tileTextureCoordinates(Rotate90);
    CHECK(quarter[0] == glm::vec2(1, 0));
    CHECK(quarter[1] == glm::vec2(1, 1));

    const auto half = scene::tileTextureCoordinates(Rotate180);
    CHECK(half[0] == glm::vec2(1, 1));
    CHECK(half[3] == glm::vec2(0, 0));

    const auto threeQuarters = scene::tileTextureCoordinates(Rotate90 | Rotate180);
    CHECK(threeQuarters[0] == glm::vec2(0, 1));
}

// Every tile is drawn as two triangles, grouped by texture so the renderer
// binds each texture once.
TEST_CASE("Terrain mesh has two triangles per tile, grouped by texture", "[scene][terrain]") {
    formats::MapTerrain terrain;
    terrain.blocks.resize(1);
    terrain.blocks[0].tiles[0].textureIndex = 7;  // one tile uses texture 7, the rest texture 0

    const auto mesh = scene::buildTerrainMesh(terrain);

    REQUIRE(mesh.size() == 2);
    CHECK(mesh[0].textureIndex == 0);
    CHECK(mesh[0].vertices.size() == 15 * 6);
    CHECK(mesh[1].textureIndex == 7);
    CHECK(mesh[1].vertices.size() == 6);
}

// --- Models ------------------------------------------------------------------

// FAC texture coordinates are texels; dividing by the texture size (and
// sampling texel centres) is what makes model textures line up.
TEST_CASE("Model mesh splits quads and normalises texture coordinates", "[scene][model]") {
    formats::Model model;
    model.vertices = {{0, 0, 0, 0}, {10, 0, 0, 0}, {10, 10, 0, 0}, {0, 10, 0, 0}};
    formats::ModelFace quad;
    quad.textureIndex = 1;
    quad.corners = {{0, 0, 0, 0}, {1, 0, 63, 0}, {2, 0, 63, 31}, {3, 0, 0, 31}};
    model.faces = {quad};

    const auto mesh = scene::buildModelMesh(model, {{16, 16}, {64, 32}});

    REQUIRE(mesh.size() == 1);
    const auto& v = mesh[0].vertices;
    REQUIRE(v.size() == 6);  // quad -> triangles 0-1-2 and 0-2-3
    CHECK(v[4].position == glm::vec3(10, 10, 0));
    CHECK(v[5].position == glm::vec3(0, 10, 0));
    CHECK(v[1].uv.x == Approx(63.5f / 64.0f));
    CHECK(v[2].uv.y == Approx(31.5f / 32.0f));
}

// Objects are placed with: world position, yaw in 1/4096 turns, model
// scale 0.5 and Y mirrored (models are stored Y-down). Without the mirror,
// trees are drawn upside down.
TEST_CASE("Object transform places, turns, scales and flips models", "[scene][objects]") {
    formats::MapObject object;
    object.x = 1000;
    object.y = 200;
    object.z = -500;
    object.angleY = 1024;  // a quarter turn

    const glm::mat4 transform = scene::objectTransform(object);
    const auto apply = [&](glm::vec3 p) { return glm::vec3(transform * glm::vec4(p, 1.0f)); };

    CHECK(rounded(apply({0, 0, 0})) == glm::ivec3(1000, 200, -500));
    CHECK(rounded(apply({0, -100, 0})) == glm::ivec3(1000, 250, -500));  // model "up" is -Y
    // +X in the model, turned a quarter turn around Y, points to -Z.
    CHECK(rounded(apply({100, 0, 0})) == glm::ivec3(1000, 200, -550));
}

// --- Loading a whole map -------------------------------------------------------

// Objects without a model in the map's .MAD (hog start positions such as
// GR_ME) must become markers, not errors, and a missing texture file must
// only produce a warning: the map should still be shown.
TEST_CASE("loadMapScene turns objects without models into markers", "[scene][MapScene]") {
    tests::TempDirectory game;
    const auto write = [&](const char* name, const tests::ByteBuilder& data) {
        std::filesystem::create_directories(game.path() / "Maps");
        std::ofstream file(game.path() / "Maps" / name, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.bytes().data()),
                   static_cast<std::streamsize>(data.size()));
    };

    tests::ByteBuilder pmg;  // flat terrain: 256 blocks of zeros
    for (int i = 0; i < 256 * 368; ++i) pmg.u8(0);
    write("TEST.PMG", pmg);

    tests::ByteBuilder pog;  // one object, "GR_ME", at (100, 50, 200)
    pog.u16(1).fixedString("GR_ME", 16).fixedString("NULL", 16).u16(100).u16(50).u16(200);
    while (pog.size() < 2 + 94) pog.u8(0);
    write("TEST.POG", pog);

    tests::ByteBuilder mad;  // archive with one unrelated entry
    mad.fixedString("OTHER.VTX", 16).u32(24).u32(8).u32(0).u32(0);
    write("TEST.MAD", mad);

    core::VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    const auto scene = scene::loadMapScene(vfs, "test");

    CHECK(scene.name == "TEST");
    CHECK(scene.objects.empty());
    REQUIRE(scene.markers.size() == 1);
    CHECK(scene.markers[0] == glm::vec3(100, 50, 200));
    CHECK_FALSE(scene.warnings.empty());  // no .PTG / .MTD
    CHECK(scene::listMaps(vfs) == std::vector<std::string>{"TEST"});
}
