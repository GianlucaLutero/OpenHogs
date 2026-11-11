// MapCollision.h
//
// What a hog can stand on and what it bumps into: the terrain plus the
// collision boxes of the map's objects (see docs/formats/maps.md).
//
// Boxes work in two ways, decided by height:
//   - a box whose top is at most kMaxStepHeight above a hog's feet is
//     something to step onto (a bridge deck, a floor, a low crate);
//   - a taller box is an obstacle (a wall, a tree trunk, a big crate).
// The terrain has its own rules: slopes steeper than kMaxWalkableSlope
// cannot be walked up, and neither can tiles the map marks as walls (the
// level designers' "no climbing here", e.g. trench walls); walking down
// them is allowed, so a hog never gets stuck.
//
// Hogs are vertical cylinders for collisions (kHogRadius, kHogHeight).

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/game/TerrainHeightField.h"

#include <glm/glm.hpp>

#include <functional>
#include <string>
#include <vector>

namespace openhogs::game {

// A hog's collision cylinder (world units). The body of a standing hog is
// about 140 units wide and 300 high.
inline constexpr float kHogRadius = 70.0f;
inline constexpr float kHogHeight = 300.0f;

// Highest step a hog can walk up (or down without falling), world units.
inline constexpr float kMaxStepHeight = 96.0f;

// Steepest terrain a hog can walk up: 2 units up per unit forward (about
// 63 degrees). Tiles marked as walls in the map cannot be walked up at all.
inline constexpr float kMaxWalkableSlope = 2.0f;

struct CollisionBox {
    glm::vec3 centre{0.0f};
    glm::vec3 halfSize{0.0f};  // along the box's own X, Y, Z axes
    float yawRadians = 0.0f;   // turn around the vertical axis

    float top() const { return centre.y + halfSize.y; }
    float bottom() const { return centre.y - halfSize.y; }

    // Horizontal distance from a point to the box's footprint (0 inside).
    float footprintDistance(float x, float z) const;
};

// The boxes of a map's objects. Hogs (the "_ME" objects) are left out,
// and so are objects for which `hasModel` returns false: those are
// invisible markers, and an invisible wall would only confuse the player.
std::vector<CollisionBox> collisionBoxes(const std::vector<formats::MapObject>& objects,
                                         const std::function<bool(const std::string&)>& hasModel);

class MapCollision {
public:
    MapCollision(TerrainHeightField terrain, std::vector<CollisionBox> boxes);

    const TerrainHeightField& terrain() const { return terrain_; }
    const std::vector<CollisionBox>& boxes() const { return boxes_; }

    // Height of what a hog with its feet at `feetY` stands on at (x, z):
    // the terrain, or the top of a box it can step onto, whichever is
    // higher.
    float groundHeight(float x, float z, float feetY) const;

    // True if a hog with its feet at `feet` overlaps a box it cannot step
    // onto.
    bool hitsObstacle(const glm::vec3& feet) const;

    // True if a hog standing on the terrain at `from` may walk on the
    // terrain to `to`: not up a wall tile, not up a slope that is too steep.
    bool canWalkTerrain(const glm::vec3& from, const glm::vec3& to) const;

private:
    TerrainHeightField terrain_;
    std::vector<CollisionBox> boxes_;
};

} // namespace openhogs::game
