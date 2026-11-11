// Tests for the game module: the fixed timestep, the terrain height
// field, creating hogs from a map, idle animation, the world and the chase
// camera.
//
// This module holds the rules of the game, so its behaviour has to be the
// same on every computer and every run. The tests use small hand-made
// terrains and animations whose results can be worked out on paper.

#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/FixedTimestep.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/IdleAnimator.h"
#include "openhogs/game/TerrainHeightField.h"
#include "openhogs/game/World.h"
#include "openhogs/scene/TerrainMesh.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <functional>
#include <set>

using namespace openhogs;
using namespace openhogs::game;
using Catch::Approx;

namespace {

// A square terrain of 2 x 2 blocks (8 x 8 tiles) whose corner is at
// (-2048, -2048); `height` gives the ground height at each sample.
formats::MapTerrain makeTerrain(const std::function<int(int x, int z)>& height) {
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

TerrainHeightField flatGround(int height = 0) {
    return TerrainHeightField(makeTerrain([height](int, int) { return height; }));
}

formats::MapObject makeObject(const std::string& name, int x, int y, int z) {
    formats::MapObject object;
    object.modelName = name;
    object.x = static_cast<std::int16_t>(x);
    object.y = static_cast<std::int16_t>(y);
    object.z = static_cast<std::int16_t>(z);
    return object;
}

// 37 animations of 12 frames (about half a second, longer than the
// cross-fade) for a two-bone skeleton. All angles are zero,
// except in the fidget animations where the root is turned by one radian,
// so standing and fidgeting are clearly different poses.
std::vector<formats::Animation> makeAnimations() {
    std::vector<formats::Animation> animations(37);
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

formats::Skeleton makeSkeleton() {
    formats::Skeleton skeleton;
    skeleton.bones.push_back({0, 0, 0, 0});
    skeleton.bones.push_back({0, 10, 0, 0});
    return skeleton;
}

glm::vec3 lookDirection(const CameraPose& pose) {
    const float yaw = glm::radians(pose.yawDegrees);
    const float pitch = glm::radians(pose.pitchDegrees);
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

void checkClose(glm::vec3 actual, glm::vec3 expected, double margin = 1e-3) {
    CHECK(actual.x == Approx(expected.x).margin(margin));
    CHECK(actual.y == Approx(expected.y).margin(margin));
    CHECK(actual.z == Approx(expected.z).margin(margin));
}

} // namespace

// The simulation must run the same number of steps for the same amount of
// time however that time is cut into frames; the remainder is kept, not
// lost, or the game would run slower on fast displays.
TEST_CASE("FixedTimestep: steps depend on elapsed time, not on frames", "[game][timestep]") {
    FixedTimestep oneFrame(0.02);
    CHECK(oneFrame.advance(0.05) == 2);
    CHECK(oneFrame.alpha() == Approx(0.5));

    FixedTimestep manyFrames(0.02);
    int steps = 0;
    for (int frame = 0; frame < 10; ++frame) {
        steps += manyFrames.advance(0.005);  // a 200 Hz display
    }
    CHECK(steps == 2);
    CHECK(manyFrames.alpha() == Approx(0.5));
}

// After a stall (the window was dragged, the computer was busy) the game
// must not freeze while it simulates all the missed time: steps per frame
// are capped and the backlog is dropped.
TEST_CASE("FixedTimestep: a stall is capped and its backlog dropped", "[game][timestep]") {
    FixedTimestep timestep(0.02, 5);
    CHECK(timestep.advance(3.0) == 5);
    CHECK(timestep.advance(0.0) == 0);
    CHECK(timestep.advance(0.02) == 1);
    CHECK_THROWS_AS(FixedTimestep(0.0), std::invalid_argument);
}

// Inside a block the samples are stored far edge first; the height field
// must return each sample at its true world position, or hogs would stand
// on a mirrored copy of the landscape.
TEST_CASE("TerrainHeightField: sample heights are found at their world position", "[game][terrain]") {
    const TerrainHeightField field(makeTerrain([](int x, int z) { return x / 16 + z / 4; }));

    CHECK(field.minX() == -2048.0f);
    CHECK(field.maxZ() == 2048.0f);
    CHECK(field.heightAt(-2048.0f, -2048.0f) == Approx(-128.0f - 512.0f));
    CHECK(field.heightAt(512.0f, -1024.0f) == Approx(32.0f - 256.0f));
    CHECK(field.heightAt(2048.0f, 2048.0f) == Approx(128.0f + 512.0f));
    CHECK(field.contains(0.0f, 0.0f));
    CHECK_FALSE(field.contains(3000.0f, 0.0f));
}

// The ground must be exactly the triangles that are drawn. Every triangle
// of the terrain mesh is checked at its centre: a wrong diagonal or a
// wrong interpolation would leave hogs floating or sunk on slopes.
TEST_CASE("TerrainHeightField: heights match the drawn terrain triangles", "[game][terrain]") {
    // Bumpy ground: no two neighbouring samples have the same height.
    const auto terrain = makeTerrain([](int x, int z) { return ((x / 512) * 37 + (z / 512) * 91) % 211; });
    const TerrainHeightField field(terrain);

    std::size_t triangles = 0;
    for (const auto& part : scene::buildTerrainMesh(terrain)) {
        for (std::size_t i = 0; i + 2 < part.vertices.size(); i += 3) {
            const glm::vec3 centre =
                (part.vertices[i].position + part.vertices[i + 1].position + part.vertices[i + 2].position) / 3.0f;
            REQUIRE(field.heightAt(centre.x, centre.z) == Approx(centre.y).margin(0.01));
            ++triangles;
        }
    }
    CHECK(triangles == 8 * 8 * 2);
}

// Outside the map there is no ground data: the edge height is used, so a
// camera or a projectile leaving the map still gets a sensible answer.
TEST_CASE("TerrainHeightField: outside the map the edge height is used", "[game][terrain]") {
    const TerrainHeightField field(makeTerrain([](int x, int) { return x; }));
    CHECK(field.heightAt(9000.0f, 0.0f) == Approx(2048.0f));
    CHECK(field.heightAt(-9000.0f, -9000.0f) == Approx(-2048.0f));
}

// Surface types (water, mud, ice...) will drive footsteps, swimming and
// damage. Tiles are stored far row first inside a block, like the samples.
TEST_CASE("TerrainHeightField: finds the tile under a point", "[game][terrain]") {
    auto terrain = makeTerrain([](int, int) { return 0; });
    // Block 0 starts at (-2048, -2048); its tile (column 1, row 0) is in
    // the far row: x from -1536 to -1024, z from -512 to 0.
    terrain.blocks[0].tiles[1].surface = formats::SurfaceType::Water;
    const TerrainHeightField field(terrain);

    CHECK(field.tileAt(-1300.0f, -200.0f).surface == formats::SurfaceType::Water);
    CHECK(field.tileAt(-1300.0f, -600.0f).surface == formats::SurfaceType::Mud);
    CHECK(field.tileAt(-900.0f, -200.0f).surface == formats::SurfaceType::Mud);
}

// Maps name hog objects after the class model ("GR_ME" = grunt). Other
// objects (trees, crates) must not become hogs, and the commando, which
// has no model of its own on PC, must still appear.
TEST_CASE("Hogs: the class comes from the object name", "[game][hogs]") {
    CHECK(hogClassForObject("GR_ME")->id == "grunt");
    CHECK(hogClassForObject("sn_me")->id == "sniper");
    CHECK(hogClassForObject("CO_ME")->id == "saboteur");
    CHECK(hogClassForObject("CRATE1") == nullptr);
    CHECK(hogClassForObject("XX_ME") == nullptr);
}

// The object's team bit, player flag and angle must end up on the hog, and
// the hog must stand on the ground: the map stores the centre of the
// model, about 168 units above the feet.
TEST_CASE("Hogs: spawned on the ground with team, player flag and facing", "[game][hogs]") {
    const auto terrain = flatGround(1000);
    auto grunt = makeObject("GR_ME", 100, 1168, -200);
    grunt.team = 0x04;
    grunt.flags = 0x0001;
    grunt.angleY = 1024;  // a quarter turn

    const auto hogs = spawnHogs({makeObject("TREEP", 0, 0, 0), grunt, makeObject("HV_ME", 0, 1170, 0)}, terrain);

    REQUIRE(hogs.size() == 2);
    CHECK(hogs[0].characterClass->id == "grunt");
    CHECK(hogs[0].team == 2);
    CHECK(hogs[0].playerControlled);
    checkClose(hogs[0].position, {100, 1000, -200});
    CHECK(hogs[0].yawRadians == Approx(glm::half_pi<float>()));
    CHECK(hogs[1].characterClass->id == "heavy");
    CHECK(hogs[1].team == 0);
    CHECK_FALSE(hogs[1].playerControlled);
}

// Some hogs start on bridges, roofs and towers, far above the terrain.
// They must keep that height instead of being dropped to the ground below.
TEST_CASE("Hogs: a hog placed well above the ground keeps its height", "[game][hogs]") {
    const auto terrain = flatGround(0);
    const auto hogs = spawnHogs({makeObject("GR_ME", 0, 2272, 0), makeObject("GR_ME", 0, 250, 0)}, terrain);

    CHECK(hogs[0].position.y == Approx(2272.0f - 168.0f));  // on a tower
    CHECK(hogs[1].position.y == Approx(0.0f));              // a little high: still on the ground
}

// The model matrix must put the hog's feet on its position and turn the
// model's forward axis (+X) to the direction hogFacing() reports, so that
// drawing, movement and the camera agree on where a hog looks.
TEST_CASE("Hogs: the model stands on its position and faces its heading", "[game][hogs]") {
    Hog hog;
    hog.position = {500, 300, -700};
    hog.yawRadians = 1.0f;
    const glm::mat4 toWorld = hogModelToWorld(hog);

    const glm::vec3 feet = glm::vec3(toWorld * glm::vec4(0, scene::kStandingFeetDepth, 0, 1));
    checkClose(feet, hog.position);

    const glm::vec3 nose = glm::vec3(toWorld * glm::vec4(100, scene::kStandingFeetDepth, 0, 1));
    checkClose(glm::normalize(nose - feet), hogFacing(hog));
    CHECK(glm::length(nose - feet) == Approx(50.0f));  // models are twice the world scale
}

// Idle behaviour must be reproducible (same seed, same sequence), differ
// between hogs, consist mostly of standing, and every fidget must be
// followed by standing again.
TEST_CASE("IdleAnimator: reproducible, mostly standing, fidgets return to standing", "[game][idle]") {
    const auto animations = makeAnimations();
    IdleAnimator first(7);
    IdleAnimator same(7);
    IdleAnimator other(8);

    int standingSteps = 0;
    int differences = 0;
    std::set<std::size_t> played;
    std::size_t previous = first.animation();
    const int steps = 50 * 120;  // two minutes
    for (int i = 0; i < steps; ++i) {
        first.update(0.02, animations);
        same.update(0.02, animations);
        other.update(0.02, animations);

        REQUIRE(first.animation() == same.animation());
        REQUIRE(first.frame() == same.frame());
        differences += first.animation() != other.animation() ? 1 : 0;
        standingSteps += first.animation() == kStandingAnimation ? 1 : 0;

        if (first.animation() != previous && previous != kStandingAnimation) {
            REQUIRE(first.animation() == kStandingAnimation);
        }
        previous = first.animation();
        played.insert(first.animation());
    }
    CHECK(standingSteps > steps / 2);
    CHECK(played.size() > 1);  // some fidgets happened
    CHECK(differences > 0);
}

// Changing animation must not snap the body from one stance to the other:
// the cross-fade spreads the change over several steps.
TEST_CASE("IdleAnimator: animation changes are cross-faded", "[game][idle]") {
    const auto animations = makeAnimations();
    const auto skeleton = makeSkeleton();
    IdleAnimator animator(3);

    const auto handPosition = [&] { return glm::vec3(animator.pose(skeleton, animations)[1][3]); };
    // Standing and fidgeting place the hand 9.6 units apart (a one radian
    // turn of a 10 unit arm).
    float largestMove = 0.0f;
    bool fidgeted = false;
    glm::vec3 before = handPosition();
    for (int i = 0; i < 50 * 120; ++i) {
        animator.update(0.02, animations);
        const glm::vec3 now = handPosition();
        largestMove = std::max(largestMove, glm::length(now - before));
        before = now;
        fidgeted = fidgeted || animator.animation() != kStandingAnimation;
    }
    REQUIRE(fidgeted);
    CHECK(largestMove < 2.0f);
}

// The camera follows the selected hog: the selection starts on the hog the
// map marks for the player and wraps around in both directions.
TEST_CASE("World: hog selection starts on the player's hog and wraps", "[game][world]") {
    const auto terrain = flatGround();
    auto player = makeObject("GR_ME", 0, 168, 0);
    player.flags = 0x0001;
    World world(terrain, spawnHogs({makeObject("GR_ME", 0, 168, 0), player, makeObject("GR_ME", 0, 168, 0)}, terrain));

    CHECK(world.selectedHog() == 1u);
    world.selectNextHog(1);
    CHECK(world.selectedHog() == 2u);
    world.selectNextHog(1);
    CHECK(world.selectedHog() == 0u);
    world.selectNextHog(-1);
    CHECK(world.selectedHog() == 2u);

    World empty(terrain, {});
    CHECK_FALSE(empty.selectedHog().has_value());
    empty.selectNextHog(1);  // must not crash
}

// update() is the only thing that moves the match forward: time advances
// and the hogs' animations with it.
TEST_CASE("World: update advances time and the hogs' animations", "[game][world]") {
    const auto terrain = flatGround();
    World world(terrain, spawnHogs({makeObject("GR_ME", 0, 168, 0)}, terrain));
    const auto animations = makeAnimations();
    const float frameBefore = world.hogs()[0].animator.frame();

    world.update(0.02, animations);

    CHECK(world.timeSeconds() == Approx(0.02));
    CHECK(world.hogs()[0].animator.frame() != frameBefore);
}

// The camera looks at its focus from the chosen distance and direction;
// this is what puts the selected hog in the middle of the screen.
TEST_CASE("ChaseCamera: looks at the target from its distance", "[game][camera]") {
    const auto terrain = flatGround(-10000);  // far below: never in the way
    ChaseCamera camera;
    camera.snapTo({100, 200, 300});
    camera.lookAlong({1, 0, 0});

    const auto pose = camera.pose(terrain);

    checkClose(pose.position + lookDirection(pose) * camera.distance(), {100, 200, 300}, 0.05);
    CHECK(lookDirection(pose).x > 0.9f);  // behind the target, looking along +X
    CHECK(pose.position.y > 200.0f);      // above it, looking down
}

// Switching hog must glide, not jump (the player keeps their bearings),
// and the glide must not depend on how time is cut into steps.
TEST_CASE("ChaseCamera: glides to a new target, independently of the step", "[game][camera]") {
    ChaseCamera coarse;
    ChaseCamera fine;
    const glm::vec3 target{1000, 0, 0};

    coarse.update(0.1, target);
    for (int i = 0; i < 10; ++i) {
        fine.update(0.01, target);
    }
    CHECK(coarse.focus().x > 100.0f);  // on its way...
    CHECK(coarse.focus().x < 900.0f);  // ...but not there yet
    CHECK(fine.focus().x == Approx(coarse.focus().x).margin(0.5));

    for (int i = 0; i < 100; ++i) {
        coarse.update(0.02, target);
    }
    CHECK(coarse.focus().x == Approx(1000.0f).margin(1.0));
}

// With a hill between the camera and the hog the camera must rise above
// the ground and still look at the hog, never show the inside of the hill.
TEST_CASE("ChaseCamera: stays above the terrain and keeps looking at the target", "[game][camera]") {
    // Flat at 0 for x >= 0, a plateau 2000 high for x < 0.
    const TerrainHeightField terrain(makeTerrain([](int x, int) { return x < 0 ? 2000 : 0; }));
    ChaseCamera camera;
    camera.snapTo({600, 100, 0});
    camera.lookAlong({1, 0, 0});  // the camera sits at x < 0, inside the plateau

    const auto pose = camera.pose(terrain);

    CHECK(pose.position.y >= terrain.heightAt(pose.position.x, pose.position.z) + 50.0f);
    const glm::vec3 toFocus = glm::normalize(camera.focus() - pose.position);
    checkClose(lookDirection(pose), toFocus, 0.01);
}

// Zoom and pitch are limited so the player cannot lose the hog by zooming
// out for ever or flip the camera over.
TEST_CASE("ChaseCamera: zoom and pitch stay within limits", "[game][camera]") {
    ChaseCamera camera;
    for (int i = 0; i < 100; ++i) camera.zoom(0.5f);
    const float closest = camera.distance();
    for (int i = 0; i < 100; ++i) camera.zoom(2.0f);
    CHECK(closest >= 100.0f);
    CHECK(camera.distance() <= 10000.0f);
    CHECK(camera.distance() > closest);

    camera.orbit(0.0f, 500.0f);
    const auto terrain = flatGround(-10000);
    CHECK(camera.pose(terrain).pitchDegrees < 45.0f);
    camera.orbit(0.0f, -1000.0f);
    CHECK(camera.pose(terrain).pitchDegrees > -89.0f);
}
