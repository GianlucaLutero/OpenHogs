// MapCollision.cpp - see MapCollision.h for an overview.

#include "openhogs/game/MapCollision.h"

#include "openhogs/game/Hog.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace openhogs::game {

namespace {

float angleToRadians(std::int16_t angle) {
    return static_cast<float>(angle) * glm::two_pi<float>() / formats::kAngleUnitsPerTurn;
}

} // namespace

float CollisionBox::footprintDistance(float x, float z) const {
    // Into the box's own axes: undo its turn around the vertical axis
    // (same rotation as map objects, see scene::objectTransform).
    const float dx = x - centre.x;
    const float dz = z - centre.z;
    const float c = std::cos(yawRadians);
    const float s = std::sin(yawRadians);
    const float localX = dx * c - dz * s;
    const float localZ = dx * s + dz * c;

    const float outsideX = std::max(std::abs(localX) - halfSize.x, 0.0f);
    const float outsideZ = std::max(std::abs(localZ) - halfSize.z, 0.0f);
    return std::sqrt(outsideX * outsideX + outsideZ * outsideZ);
}

std::vector<CollisionBox> collisionBoxes(const std::vector<formats::MapObject>& objects,
                                         const std::function<bool(const std::string&)>& hasModel) {
    std::vector<CollisionBox> boxes;
    for (const auto& object : objects) {
        if (hogClassForObject(object.modelName) != nullptr || !hasModel(object.modelName)) {
            continue;
        }
        if (object.boxSizeX <= 0 || object.boxSizeY <= 0 || object.boxSizeZ <= 0) {
            continue;  // no box: nothing to bump into
        }
        // Only the turn around the vertical axis is used: in the game data
        // the other two angles are almost always zero.
        CollisionBox box;
        box.centre = {object.x, object.y, object.z};
        const float half = formats::kBoxSizeUnit / 2.0f;
        box.halfSize = {object.boxSizeX * half, object.boxSizeY * half, object.boxSizeZ * half};
        box.yawRadians = angleToRadians(object.angleY);
        boxes.push_back(box);
    }
    return boxes;
}

MapCollision::MapCollision(TerrainHeightField terrain, std::vector<CollisionBox> boxes)
    : terrain_(std::move(terrain)), boxes_(std::move(boxes)) {}

float MapCollision::groundHeight(float x, float z, float feetY) const {
    float ground = terrain_.heightAt(x, z);
    for (const auto& box : boxes_) {
        const float top = box.top();
        if (top > ground && top <= feetY + kMaxStepHeight && box.footprintDistance(x, z) == 0.0f) {
            ground = top;
        }
    }
    return ground;
}

bool MapCollision::hitsObstacle(const glm::vec3& feet) const {
    for (const auto& box : boxes_) {
        const bool canStepOn = box.top() <= feet.y + kMaxStepHeight;
        const bool overhead = box.bottom() >= feet.y + kHogHeight;
        if (canStepOn || overhead) {
            continue;
        }
        if (box.footprintDistance(feet.x, feet.z) < kHogRadius) {
            return true;
        }
    }
    return false;
}

bool MapCollision::canWalkTerrain(const glm::vec3& from, const glm::vec3& to) const {
    const float rise = terrain_.heightAt(to.x, to.z) - terrain_.heightAt(from.x, from.z);
    if (rise <= 0.0f) {
        return true;  // level or downhill: always possible
    }
    if (terrain_.tileAt(to.x, to.z).isWall) {
        return false;
    }
    const float run = glm::length(glm::vec2(to.x - from.x, to.z - from.z));
    return run > 0.0f && rise / run <= kMaxWalkableSlope;
}

} // namespace openhogs::game
