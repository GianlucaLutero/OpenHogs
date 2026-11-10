// Tests for the map formats: terrain (.PMG), objects (.POG) and terrain
// textures (.PTG).

#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/TextureTable.h"

#include "ByteBuilder.h"
#include "openhogs/core/BinaryReader.h"  // FormatError

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;

namespace {

// Builds a full PMG file (256 blocks). Every block gets offsets from its
// grid position, height = 100 * sample index, and tile 5 of block 0 gets a
// distinctive type byte, orientation and texture.
ByteBuilder makePmg() {
    ByteBuilder pmg;
    for (int block = 0; block < 256; ++block) {
        pmg.u16(static_cast<std::uint16_t>(block * 2)).u16(0xFFFF);  // x offset, y (unused)
        pmg.u16(static_cast<std::uint16_t>(block * 3)).u16(0);       // z offset, unknown
        for (int sample = 0; sample < 25; ++sample) {
            pmg.u16(static_cast<std::uint16_t>(sample * 100)).u16(200);  // height, shading
        }
        pmg.u32(0);
        for (int tile = 0; tile < 16; ++tile) {
            const bool special = block == 0 && tile == 5;
            pmg.u16(0).u16(0).u16(0);
            pmg.u8(special ? 0xA4 : 0x01);  // 0xA4 = wall + water flag + surface 4
            pmg.u8(special ? 3 : 0);        // slip
            pmg.u16(0);
            pmg.u8(special ? 6 : 0);        // Rotate90 | Rotate180
            pmg.u32(special ? 42 : 0);      // texture
            pmg.u8(0);
        }
    }
    return pmg;
}

} // namespace

// The tile type byte packs a surface type and three flags; walls and water
// decide how hogs move, so each part must be decoded separately. Heights
// are stored row by row, which the terrain mesh relies on.
TEST_CASE("MapTerrain decodes blocks, heights and tile flags", "[formats][MapTerrain]") {
    const auto terrain = MapTerrain::parse(makePmg().bytes());

    REQUIRE(terrain.blocks.size() == 256);
    const auto& block = terrain.blocks[0];
    CHECK(terrain.blocks[17].xOffset == 34);
    CHECK(terrain.blocks[17].zOffset == 51);
    CHECK(block.height(1, 2) == 1100);  // sample index 2 * 5 + 1 = 11
    CHECK(block.shading[0] == 200);

    const auto& tile = block.tile(1, 1);  // tile index 5
    CHECK(tile.surface == SurfaceType::Water);
    CHECK(tile.isWater);
    CHECK(tile.isWall);
    CHECK_FALSE(tile.isMine);
    CHECK(tile.slip == 3);
    CHECK(tile.orientation == (TileOrientation::Rotate90 | TileOrientation::Rotate180));
    CHECK(tile.textureIndex == 42);
}

// All original maps are exactly 16 x 16 blocks; any other size means a
// corrupt or foreign file.
TEST_CASE("MapTerrain rejects files of the wrong size", "[formats][MapTerrain]") {
    ByteBuilder tooShort;
    tooShort.u32(0);
    CHECK_THROWS_AS(MapTerrain::parse(tooShort.bytes()), FormatError);
}

// Objects are fixed 94-byte records after a 16-bit count. Real files can
// end with extra bytes, which must not be read as an object. Team, script
// type and flags are checked at the offsets confirmed on ICE.POG and
// CAMP.POG (59, 60 and 88): they decide which hogs belong to which player.
TEST_CASE("parseMapObjects reads fixed-size records", "[formats][MapObjects]") {
    ByteBuilder pog;
    pog.u16(2);
    for (const char* name : {"CRATE1", "TREEP"}) {
        const auto start = pog.size();
        pog.fixedString(name, 16).fixedString("NULL", 16);
        pog.u16(static_cast<std::uint16_t>(-100)).u16(928).u16(300);  // position
        pog.u16(1).u16(0).u16(1024).u16(0).u16(0);                     // id, angles, type
        pog.u16(0).u16(0).u16(0).u16(0);                               // bounding box
        pog.u16(0).u8(0x3F);                                           // unknown (offset 56)
        pog.u8(0x04).u16(20);                                          // team, script type
        while (pog.size() < start + 88) {
            pog.u8(0);
        }
        pog.u16(1);  // flags: player
        while (pog.size() < start + 94) {
            pog.u8(0);
        }
    }
    pog.u16(0);  // trailing bytes, as in CAMP.POG

    const auto objects = parseMapObjects(pog.bytes());

    REQUIRE(objects.size() == 2);
    CHECK(objects[0].modelName == "CRATE1");
    CHECK(objects[0].x == -100);
    CHECK(objects[0].y == 928);
    CHECK(objects[0].angleY == 1024);  // a quarter turn
    CHECK(objects[0].team == 0x04);
    CHECK(objects[0].scriptType == 20);
    CHECK(objects[0].flags == 1);
    CHECK(objects[1].modelName == "TREEP");
}

// A count larger than the data would otherwise make the parser read
// garbage objects past the end of the file.
TEST_CASE("parseMapObjects rejects truncated files", "[formats][MapObjects]") {
    ByteBuilder pog;
    pog.u16(3).fixedString("CRATE1", 16);
    CHECK_THROWS_AS(parseMapObjects(pog.bytes()), FormatError);
}

// PTG has no per-image size: images are equal slices of the data. Getting
// the slice size wrong would shift every terrain texture.
TEST_CASE("splitTextureTable slices equal-size images", "[formats][TextureTable]") {
    ByteBuilder ptg;
    ptg.u32(3).raw("aabbcc");

    const auto images = splitTextureTable(ptg.bytes());

    REQUIRE(images.size() == 3);
    CHECK(images[1].size() == 2);
    CHECK(static_cast<char>(images[1][0]) == 'b');

    ByteBuilder uneven;
    uneven.u32(4).raw("abcdefg");
    CHECK_THROWS_AS(splitTextureTable(uneven.bytes()), FormatError);
}
