// Tests for the game module: the fixed timestep, the terrain height
// field, creating hogs from a map and the chase camera. Movement, turns
// and animation playback are in MovementTests.cpp.
//
// This module holds the rules of the game, so its behaviour has to be the
// same on every computer and every run. The tests use small hand-made
// terrains and animations whose results can be worked out on paper.

#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/FixedTimestep.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/TerrainHeightField.h"
#include "openhogs/game/World.h"
#include "openhogs/scene/TerrainMesh.h"

#include "GameTestSupport.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <functional>
#include <set>

using namespace openhogs;
using namespace openhogs::game;
using Catch::Approx;
using namespace openhogs::tests;


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

// While the hog runs, the camera swings round behind it: gradually (no
// jerk) and the short way round (never a full spin past 180 degrees).
TEST_CASE("ChaseCamera: swings round behind a direction, the short way", "[game][camera]") {
    const auto terrain = flatGround(-10000);
    ChaseCamera camera;
    camera.lookAlong({0, 0, -1});  // yaw 0
    camera.turnTowards({-1, 0, -0.01f}, 0.1);  // almost yaw -90: turn left
    const float yaw = camera.pose(terrain).yawDegrees;
    CHECK(yaw < 0.0f);
    CHECK(yaw > -45.0f);  // only part of the way after 0.1 s

    for (int i = 0; i < 200; ++i) {
        camera.turnTowards({-1, 0, 0}, 0.02);
    }
    CHECK(std::remainder(camera.pose(terrain).yawDegrees + 90.0f, 360.0f) == Approx(0.0f).margin(0.5));
}
