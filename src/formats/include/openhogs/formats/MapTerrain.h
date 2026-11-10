// MapTerrain.h
//
// Reader for .PMG files: the terrain of a map (see docs/formats/maps.md).
//
// A map is a square of 16 x 16 blocks. Each block is 2048 x 2048 world
// units, holds a 5 x 5 grid of height samples and is split into 4 x 4
// tiles of 512 x 512 units. Every tile has a surface type (grass, mud,
// water...) and a texture taken from the map's .PTG file.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

inline constexpr int kBlocksPerSide = 16;
inline constexpr int kTilesPerBlockSide = 4;
inline constexpr int kSamplesPerBlockSide = kTilesPerBlockSide + 1;  // 5 x 5 heights
inline constexpr int kTileSize = 512;                                // world units
inline constexpr int kBlockSize = kTileSize * kTilesPerBlockSide;    // 2048 world units

// Surface of a tile, stored in the low 5 bits of the tile type byte.
enum class SurfaceType : std::uint8_t {
    Mud = 0, Grass = 1, Metal = 2, Wood = 3, Water = 4, Stone = 5,
    Rock = 6, Sand = 7, Ice = 8, Snow = 9, Quagmire = 10, Lava = 11
};

// How the tile's texture is turned, as stored in the file. Values combine:
// e.g. FlipX | Rotate90.
namespace TileOrientation {
inline constexpr std::uint8_t FlipX = 1;
inline constexpr std::uint8_t Rotate90 = 2;
inline constexpr std::uint8_t Rotate180 = 4;
}

struct TerrainTile {
    SurfaceType surface = SurfaceType::Mud;
    bool isWater = false;
    bool isMine = false;
    bool isWall = false;
    std::uint8_t slip = 0;          // slope direction for walls (see docs)
    std::uint8_t orientation = 0;   // TileOrientation flags
    std::uint32_t textureIndex = 0; // index into the map's .PTG textures
};

struct TerrainBlock {
    // World position of the block's corner. See docs/formats/maps.md for
    // how sample rows map to world Z: row 0 is at z = zOffset + kBlockSize.
    std::int16_t xOffset = 0;
    std::int16_t zOffset = 0;

    // 5 x 5 samples, row by row.
    std::array<std::int16_t, 25> heights{};
    std::array<std::uint8_t, 25> shading{};  // baked lighting, 0 (dark) - 255 (bright)

    // 4 x 4 tiles, row by row.
    std::array<TerrainTile, 16> tiles{};

    std::int16_t height(int column, int row) const {
        return heights[static_cast<std::size_t>(row * kSamplesPerBlockSide + column)];
    }
    const TerrainTile& tile(int column, int row) const {
        return tiles[static_cast<std::size_t>(row * kTilesPerBlockSide + column)];
    }
};

struct MapTerrain {
    // kBlocksPerSide * kBlocksPerSide blocks, row by row.
    std::vector<TerrainBlock> blocks;

    // Throws core::FormatError if the file is not a complete PMG file.
    static MapTerrain parse(std::span<const std::byte> data);
};

} // namespace openhogs::formats
