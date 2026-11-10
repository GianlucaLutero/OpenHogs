// TerrainMesh.cpp - see TerrainMesh.h for an overview.

#include "openhogs/scene/TerrainMesh.h"

#include <map>
#include <utility>

namespace openhogs::scene {

namespace {

using formats::kBlockSize;
using formats::kSamplesPerBlockSide;
using formats::kTileSize;
using formats::kTilesPerBlockSide;

// Baked shading is stored as 0-255; most samples are between 150 and 230.
// Dividing by 255 keeps the textures' colours close to the original game
// (dividing by 128, the PlayStation "neutral" value, over-exposes them).
constexpr float kNeutralShading = 255.0f;

float sampleBrightness(const formats::TerrainBlock& block, int column, int row) {
    return static_cast<float>(block.shading[row * kSamplesPerBlockSide + column]) / kNeutralShading;
}

} // namespace

glm::vec3 terrainSamplePosition(const formats::TerrainBlock& block, int column, int row) {
    return {static_cast<float>(block.xOffset + column * kTileSize),
            static_cast<float>(block.height(column, row)),
            static_cast<float>(block.zOffset + kBlockSize - row * kTileSize)};
}

std::array<glm::vec2, 4> tileTextureCoordinates(std::uint8_t orientation) {
    // Corners in (column, row) order: top-left, top-right, bottom-left, bottom-right.
    std::array<glm::vec2, 4> uv{glm::vec2{0, 0}, glm::vec2{1, 0}, glm::vec2{0, 1}, glm::vec2{1, 1}};

    if (orientation & formats::TileOrientation::FlipX) {
        for (auto& c : uv) {
            c.x = 1.0f - c.x;
        }
    }
    // Rotations turn the texture by 90 degrees steps; 90 + 180 = 270.
    int quarterTurns = 0;
    if (orientation & formats::TileOrientation::Rotate90) quarterTurns += 1;
    if (orientation & formats::TileOrientation::Rotate180) quarterTurns += 2;
    for (int i = 0; i < quarterTurns; ++i) {
        for (auto& c : uv) {
            c = {1.0f - c.y, c.x};  // one quarter turn around the tile centre
        }
    }
    return uv;
}

Mesh buildTerrainMesh(const formats::MapTerrain& terrain) {
    // Collect triangles per texture first, then turn the map into a list.
    std::map<std::uint32_t, std::vector<Vertex>> trianglesByTexture;

    for (const auto& block : terrain.blocks) {
        for (int row = 0; row < kTilesPerBlockSide; ++row) {
            for (int column = 0; column < kTilesPerBlockSide; ++column) {
                const auto& tile = block.tile(column, row);
                const auto uv = tileTextureCoordinates(tile.orientation);

                // The four corners of the tile, in the same order as `uv`.
                const std::array<std::pair<int, int>, 4> corners{
                    std::pair{column, row}, std::pair{column + 1, row},
                    std::pair{column, row + 1}, std::pair{column + 1, row + 1}};
                std::array<Vertex, 4> v;
                for (std::size_t i = 0; i < 4; ++i) {
                    const auto [c, r] = corners[i];
                    v[i] = {terrainSamplePosition(block, c, r), uv[i], sampleBrightness(block, c, r)};
                }

                auto& out = trianglesByTexture[tile.textureIndex];
                out.insert(out.end(), {v[0], v[1], v[2]});
                out.insert(out.end(), {v[1], v[3], v[2]});
            }
        }
    }

    Mesh mesh;
    for (auto& [texture, vertices] : trianglesByTexture) {
        mesh.push_back({texture, std::move(vertices)});
    }
    return mesh;
}

} // namespace openhogs::scene
