// MapTerrain.cpp - see MapTerrain.h for an overview.

#include "openhogs/formats/MapTerrain.h"

#include "openhogs/core/BinaryReader.h"

#include <algorithm>
#include <string>

namespace openhogs::formats {

namespace {

constexpr std::size_t kTileRecordSize = 16;
constexpr std::size_t kBlockRecordSize = 8 + 25 * 4 + 4 + 16 * kTileRecordSize;  // 368 bytes
constexpr std::size_t kBlockCount = kBlocksPerSide * kBlocksPerSide;

constexpr std::uint8_t kSurfaceMask = 0x1F;
constexpr std::uint8_t kWaterFlag = 0x20;
constexpr std::uint8_t kMineFlag = 0x40;
constexpr std::uint8_t kWallFlag = 0x80;

TerrainTile readTile(core::BinaryReader& reader) {
    TerrainTile tile;
    reader.skip(6);  // three 16-bit fields, always zero

    const std::uint8_t type = reader.readU8();
    tile.surface = static_cast<SurfaceType>(type & kSurfaceMask);
    tile.isWater = (type & kWaterFlag) != 0;
    tile.isMine = (type & kMineFlag) != 0;
    tile.isWall = (type & kWallFlag) != 0;

    tile.slip = reader.readU8();
    reader.skip(2);  // always zero
    tile.orientation = reader.readU8();
    tile.textureIndex = reader.readU32();
    reader.skip(1);  // always zero
    return tile;
}

TerrainBlock readBlock(core::BinaryReader& reader) {
    TerrainBlock block;
    block.xOffset = reader.readI16();
    reader.skip(2);  // Y offset: not reliable in the game data, not used
    block.zOffset = reader.readI16();
    reader.skip(2);  // unknown

    for (std::size_t i = 0; i < block.heights.size(); ++i) {
        block.heights[i] = reader.readI16();
        // Stored as 16 bits but only 0-255 is used.
        block.shading[i] = static_cast<std::uint8_t>(std::min<std::uint16_t>(reader.readU16(), 255));
    }

    reader.skip(4);  // always zero
    for (auto& tile : block.tiles) {
        tile = readTile(reader);
    }
    return block;
}

} // namespace

MapTerrain MapTerrain::parse(std::span<const std::byte> data) {
    // Every PMG file of the original game has exactly 16 x 16 blocks.
    if (data.size() != kBlockCount * kBlockRecordSize) {
        throw core::FormatError("PMG: expected " + std::to_string(kBlockCount * kBlockRecordSize) +
                                " bytes (16 x 16 blocks), got " + std::to_string(data.size()));
    }

    MapTerrain terrain;
    terrain.blocks.reserve(kBlockCount);
    core::BinaryReader reader(data);
    for (std::size_t i = 0; i < kBlockCount; ++i) {
        terrain.blocks.push_back(readBlock(reader));
    }
    return terrain;
}

} // namespace openhogs::formats
