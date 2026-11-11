// TerrainHeightField.h
//
// Answers "how high is the ground here?" and "what is the ground made of
// here?" for any point of a map. Everything that stands or moves on the
// terrain (hogs, the camera, later projectiles and vehicles) uses it.
//
// The .PMG terrain is a grid of height samples 512 units apart, drawn as
// two triangles per tile. heightAt() returns the height of exactly those
// triangles (the same split as scene::buildTerrainMesh), so things stand
// on the ground that is drawn, without sinking in or floating.

#pragma once

#include "openhogs/formats/MapTerrain.h"

#include <vector>

namespace openhogs::game {

class TerrainHeightField {
public:
    // Throws std::invalid_argument if the blocks do not form a regular
    // grid (every map of the original game does).
    explicit TerrainHeightField(const formats::MapTerrain& terrain);

    // The square covered by the terrain, in world units.
    float minX() const { return minX_; }
    float maxX() const { return minX_ + static_cast<float>(tilesPerSide_ * formats::kTileSize); }
    float minZ() const { return minZ_; }
    float maxZ() const { return minZ_ + static_cast<float>(tilesPerSide_ * formats::kTileSize); }
    bool contains(float x, float z) const;

    // Height of the ground. Points outside the map get the height of the
    // nearest point of its edge.
    float heightAt(float x, float z) const;

    // The tile under a point (surface type, water, wall...); outside the
    // map, the nearest tile of its edge.
    const formats::TerrainTile& tileAt(float x, float z) const;

private:
    float sample(int column, int row) const;

    float minX_ = 0.0f;
    float minZ_ = 0.0f;
    int tilesPerSide_ = 0;
    // (tilesPerSide_ + 1)^2 heights; row 0 is at minZ, each next row 512
    // units further along +Z.
    std::vector<float> heights_;
    std::vector<formats::TerrainTile> tiles_;  // tilesPerSide_^2, same order
};

} // namespace openhogs::game
