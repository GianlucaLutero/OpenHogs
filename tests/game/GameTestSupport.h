// GameTestSupport.h
//
// Small hand-made terrains, objects and animations shared by the tests of
// the game module. Their values are chosen so that expected results can be
// worked out on paper.

#pragma once

#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"
#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/IdleAnimator.h"
#include "openhogs/game/TerrainHeightField.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/glm.hpp>

#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace openhogs::tests {

using namespace openhogs::game;
using Catch::Approx;

// A square terrain of 2 x 2 blocks (8 x 8 tiles) whose corner is at
// (-2048, -2048); `height` gives the ground height at each sample.
inline formats::MapTerrain makeTerrain(const std::function<int(int x, int z)>& height) {
    formats::MapTerrain terrain;
    for (int blockZ = 0; blockZ < 2; ++blockZ) {
        for (int blockX = 0; blockX < 2; ++blockX) {
            formats::TerrainBlock block;
            block.xOffset = static_cast<std::int16_t>(-2048 + blockX * formats::kBlockSize);
            block.zOffset = static_cast<std::int16_t>(-2048 + blockZ * formats::kBlockSize);
            for (int row = 0; row < 5; ++row) {
                for (int column = 0; column < 5; ++column) {
                    const int x = block.xOffset + column * formats::kTileSize;
                    const int z = block.zOffset + formats::kBlockSize - row * formats::kTileSize;
                    block.heights[static_cast<std::size_t>(row * 5 + column)] = static_cast<std::int16_t>(height(x, z));
                }
            }
            terrain.blocks.push_back(block);
        }
    }
    return terrain;
}

inline TerrainHeightField flatGround(int height = 0) {
    return TerrainHeightField(makeTerrain([height](int, int) { return height; }));
}

inline formats::MapObject makeObject(const std::string& name, int x, int y, int z) {
    formats::MapObject object;
    object.modelName = name;
    object.x = static_cast<std::int16_t>(x);
    object.y = static_cast<std::int16_t>(y);
    object.z = static_cast<std::int16_t>(z);
    return object;
}

// 40 animations (enough for every animation number the game module uses)
// of 12 frames (about half a second, longer than the cross-fade) for a
// two-bone skeleton. All angles are zero, except in every animation but
// standing, where the root is turned by one radian, so standing and any
// other animation are clearly different poses.
inline std::vector<formats::Animation> makeAnimations() {
    std::vector<formats::Animation> animations(40);
    for (std::size_t i = 0; i < animations.size(); ++i) {
        formats::AnimationFrame frame;
        frame.bones.resize(2);
        if (i != kStandingAnimation) {
            frame.bones[0].z = 1.0f;
        }
        animations[i].frames.assign(12, frame);
    }
    return animations;
}

inline formats::Skeleton makeSkeleton() {
    formats::Skeleton skeleton;
    skeleton.bones.push_back({0, 0, 0, 0});
    skeleton.bones.push_back({0, 10, 0, 0});
    return skeleton;
}

inline glm::vec3 lookDirection(const CameraPose& pose) {
    const float yaw = glm::radians(pose.yawDegrees);
    const float pitch = glm::radians(pose.pitchDegrees);
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

inline void checkClose(glm::vec3 actual, glm::vec3 expected, double margin = 1e-3) {
    CHECK(actual.x == Approx(expected.x).margin(margin));
    CHECK(actual.y == Approx(expected.y).margin(margin));
    CHECK(actual.z == Approx(expected.z).margin(margin));
}

inline constexpr double kStep = 1.0 / 50.0;  // the game's simulation step

// A hog standing at (x, ground, z), facing +X, in `team`.
inline Hog makeHog(const MapCollision& map, float x, float z, int team = 0) {
    Hog hog;
    hog.characterClass = hogClassForObject("GR_ME");
    hog.team = team;
    hog.position = {x, map.terrain().heightAt(x, z), z};
    hog.idle.start(hog.animation);
    return hog;
}

inline MapCollision flatMap(std::vector<CollisionBox> boxes = {}, int height = 0) {
    return MapCollision(flatGround(height), std::move(boxes));
}

inline CollisionBox makeBox(glm::vec3 centre, glm::vec3 halfSize, float yaw = 0.0f) {
    CollisionBox box;
    box.centre = centre;
    box.halfSize = halfSize;
    box.yawRadians = yaw;
    return box;
}

} // namespace openhogs::tests
