// TerrainHeightField.cpp - see TerrainHeightField.h for an overview.

#include "openhogs/game/TerrainHeightField.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace openhogs::game {

namespace {

using formats::kBlockSize;
using formats::kSamplesPerBlockSide;
using formats::kTileSize;
using formats::kTilesPerBlockSide;

} // namespace

TerrainHeightField::TerrainHeightField(const formats::MapTerrain& terrain) {
    if (terrain.blocks.empty()) {
        throw std::invalid_argument("Terrain has no blocks");
    }

    int lowX = std::numeric_limits<int>::max();
    int lowZ = std::numeric_limits<int>::max();
    int highX = std::numeric_limits<int>::lowest();
    for (const auto& block : terrain.blocks) {
        lowX = std::min<int>(lowX, block.xOffset);
        lowZ = std::min<int>(lowZ, block.zOffset);
        highX = std::max<int>(highX, block.xOffset + kBlockSize);
    }
    minX_ = static_cast<float>(lowX);
    minZ_ = static_cast<float>(lowZ);
    tilesPerSide_ = (highX - lowX) / kTileSize;

    const auto samplesPerSide = static_cast<std::size_t>(tilesPerSide_ + 1);
    heights_.assign(samplesPerSide * samplesPerSide, 0.0f);
    tiles_.resize(static_cast<std::size_t>(tilesPerSide_) * static_cast<std::size_t>(tilesPerSide_));

    for (const auto& block : terrain.blocks) {
        const int blockColumn = (block.xOffset - lowX) / kTileSize;
        const int blockRow = (block.zOffset - lowZ) / kTileSize;
        const bool onGrid = (block.xOffset - lowX) % kBlockSize == 0 && (block.zOffset - lowZ) % kBlockSize == 0 &&
                            blockColumn + kTilesPerBlockSide <= tilesPerSide_ &&
                            blockRow + kTilesPerBlockSide <= tilesPerSide_;
        if (!onGrid) {
            throw std::invalid_argument("Terrain blocks do not form a regular square grid");
        }

        // Inside a block, sample row 0 is the far edge (largest Z) and the
        // rows go towards smaller Z; here rows go towards larger Z.
        for (int row = 0; row < kSamplesPerBlockSide; ++row) {
            for (int column = 0; column < kSamplesPerBlockSide; ++column) {
                const int globalRow = blockRow + (kTilesPerBlockSide - row);
                const auto index = static_cast<std::size_t>(globalRow) * samplesPerSide +
                                   static_cast<std::size_t>(blockColumn + column);
                heights_[index] = static_cast<float>(block.height(column, row));
            }
        }
        for (int row = 0; row < kTilesPerBlockSide; ++row) {
            for (int column = 0; column < kTilesPerBlockSide; ++column) {
                const int globalRow = blockRow + (kTilesPerBlockSide - 1 - row);
                const auto index = static_cast<std::size_t>(globalRow) * static_cast<std::size_t>(tilesPerSide_) +
                                   static_cast<std::size_t>(blockColumn + column);
                tiles_[index] = block.tile(column, row);
            }
        }
    }
}

bool TerrainHeightField::contains(float x, float z) const {
    return x >= minX() && x <= maxX() && z >= minZ() && z <= maxZ();
}

float TerrainHeightField::sample(int column, int row) const {
    const auto samplesPerSide = static_cast<std::size_t>(tilesPerSide_ + 1);
    return heights_[static_cast<std::size_t>(row) * samplesPerSide + static_cast<std::size_t>(column)];
}

float TerrainHeightField::heightAt(float x, float z) const {
    // Position in tiles, clamped to the map.
    const float limit = static_cast<float>(tilesPerSide_);
    const float tx = std::clamp((x - minX_) / kTileSize, 0.0f, limit);
    const float tz = std::clamp((z - minZ_) / kTileSize, 0.0f, limit);
    const int column = std::min(static_cast<int>(tx), tilesPerSide_ - 1);
    const int row = std::min(static_cast<int>(tz), tilesPerSide_ - 1);
    const float fx = tx - static_cast<float>(column);  // 0..1 across the tile
    const float fz = tz - static_cast<float>(row);

    const float nearLeft = sample(column, row);          // smaller Z
    const float nearRight = sample(column + 1, row);
    const float farLeft = sample(column, row + 1);       // larger Z
    const float farRight = sample(column + 1, row + 1);

    // The tile is drawn as two triangles split along the diagonal from
    // near-left to far-right (see scene::buildTerrainMesh): the height is
    // read from the triangle the point is in.
    if (fz >= fx) {
        return nearLeft + (farRight - farLeft) * fx + (farLeft - nearLeft) * fz;
    }
    return nearLeft + (nearRight - nearLeft) * fx + (farRight - nearRight) * fz;
}

const formats::TerrainTile& TerrainHeightField::tileAt(float x, float z) const {
    const int last = tilesPerSide_ - 1;
    const int column = std::clamp(static_cast<int>(std::floor((x - minX_) / kTileSize)), 0, last);
    const int row = std::clamp(static_cast<int>(std::floor((z - minZ_) / kTileSize)), 0, last);
    return tiles_[static_cast<std::size_t>(row) * static_cast<std::size_t>(tilesPerSide_) +
                  static_cast<std::size_t>(column)];
}

} // namespace openhogs::game
