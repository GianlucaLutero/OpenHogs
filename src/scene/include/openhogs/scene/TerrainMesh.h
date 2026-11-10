// TerrainMesh.h
//
// Builds the triangles of a map's terrain from its .PMG data.
//
// World coordinates follow the original game: X and Z are the ground
// plane, Y is the height (larger = higher). Inside a block, sample row 0
// lies at the block's far edge (z = zOffset + 2048) and each next row is
// 512 units closer to zOffset. This was verified by checking that the
// objects of several maps sit on the terrain only with this orientation.

#pragma once

#include "openhogs/formats/MapTerrain.h"
#include "openhogs/scene/Geometry.h"

#include <array>

namespace openhogs::scene {

// World position of one height sample of a block.
glm::vec3 terrainSamplePosition(const formats::TerrainBlock& block, int column, int row);

// Texture coordinates for the four corners of a tile, in the order
// (column, row), (column+1, row), (column, row+1), (column+1, row+1),
// after applying the tile's flip and rotation flags.
std::array<glm::vec2, 4> tileTextureCoordinates(std::uint8_t orientation);

// Two triangles per tile, grouped by the tile's PTG texture index.
// Brightness comes from the baked shading stored with each sample.
Mesh buildTerrainMesh(const formats::MapTerrain& terrain);

} // namespace openhogs::scene
