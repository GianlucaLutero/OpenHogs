// MapObjects.h
//
// Reader for .POG files: the objects placed on a map (trees, crates,
// buildings, bridges, start positions of the hogs...).
// See docs/formats/maps.md.
//
// Each object names a model (looked up in the map's .MAD archive), a world
// position and a rotation. Several fields drive gameplay scripts (mission
// targets, teams) and are kept for later blocks.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace openhogs::formats {

// Angles are stored as 1/4096 of a full turn (PlayStation convention).
inline constexpr int kAngleUnitsPerTurn = 4096;

// Collision box sizes are stored in steps of 64 world units.
inline constexpr int kBoxSizeUnit = 64;

struct MapObject {
    std::string modelName;  // e.g. "CRATE1": model files are CRATE1.VTX/.FAC/.NO2

    // World position: Y is the height, like the terrain samples.
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::int16_t z = 0;

    // Rotation around each axis, in 1/4096 of a turn.
    std::int16_t angleX = 0;
    std::int16_t angleY = 0;
    std::int16_t angleZ = 0;

    std::int16_t id = 0;
    std::int16_t typeId = 0;
    // Collision box: full size along the model's own X, Y and Z axes, in
    // kBoxSizeUnit steps, centred on the position. For trees it covers
    // only the trunk. Verified on the game data: e.g. a 1024 x 64 x 512
    // bridge deck has sizes 16, 1, 8 (see docs/formats/maps.md).
    std::int16_t boxSizeX = 0;
    std::int16_t boxSizeY = 0;
    std::int16_t boxSizeZ = 0;
    std::uint8_t team = 0;         // bit mask of teams (hogs only)
    std::int16_t scriptType = 0;   // mission script role, 0 = none
    std::uint16_t flags = 0;
};

// Throws core::FormatError if the file is shorter than its object count says.
std::vector<MapObject> parseMapObjects(std::span<const std::byte> data);

} // namespace openhogs::formats
