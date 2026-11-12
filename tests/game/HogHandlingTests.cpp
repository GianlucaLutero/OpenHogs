// Tests for how hogs hold their weapons and how they move and swim, as the
// original executable does it (block hog-handling).
//
// They protect what the player sees and feels: the weapon in the hand
// pointing the right way and following the aim, the weapon put away while
// running, the play area's limits, and water that is dangerous to swim in.

#include "GameTestSupport.h"

#include "openhogs/game/AnimationPlayer.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/game/World.h"
#include "openhogs/scene/ModelMesh.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace openhogs;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

MovementInput forwardInput() {
    MovementInput input;
    input.forward = true;
    return input;
}

// A flat terrain of the original's full size, 16 x 16 blocks centred on
// (0, 0), so the play area (one block short of each edge) can be reached.
formats::MapTerrain fullSizeTerrain() {
    formats::MapTerrain terrain;
    for (int blockZ = 0; blockZ < formats::kBlocksPerSide; ++blockZ) {
        for (int blockX = 0; blockX < formats::kBlocksPerSide; ++blockX) {
            formats::TerrainBlock block;
            block.xOffset = static_cast<std::int16_t>(-16384 + blockX * formats::kBlockSize);
            block.zOffset = static_cast<std::int16_t>(-16384 + blockZ * formats::kBlockSize);
            terrain.blocks.push_back(block);
        }
    }
    return terrain;
}

// A 2 x 2 block map with water for x >= `waterFrom`.
MapCollision mapWithWater(int waterFrom = 0) {
    auto terrain = makeTerrain([](int, int) { return 0; });
    for (auto& block : terrain.blocks) {
        for (int i = 0; i < static_cast<int>(block.tiles.size()); ++i) {
            const int x = block.xOffset + (i % formats::kTilesPerBlockSide) * formats::kTileSize;
            block.tiles[static_cast<std::size_t>(i)].isWater = x >= waterFrom;
        }
    }
    return MapCollision{TerrainHeightField(terrain), {}};
}

// A hog of `classObject` (e.g. "AC_ME") standing at (x, z) in `team`.
Hog hogOfClass(const MapCollision& map, const char* classObject, float x, float z, int team) {
    Hog hog = makeHog(map, x, z, team);
    hog.characterClass = hogClassForObject(classObject);
    return hog;
}

} // namespace

TEST_CASE("Hold: an aiming pose stays on its frame", "[game][animation][weapons]") {
    // Each frame of the original's aiming animations is the pose for one
    // angle: the hog must stay on the frame of its aim, not play through.
    const auto animations = makeAnimations();
    AnimationPlayer player;
    player.hold(24, 8.0f);
    for (int i = 0; i < 25; ++i) player.update(kStep, animations);
    CHECK(player.animation() == 24u);
    CHECK(player.frame() == Approx(8.0f));
    CHECK(player.repeat() == AnimationPlayer::Repeat::Hold);
}

TEST_CASE("Weapons: the aiming pose follows the angle", "[game][weapons]") {
    // Frame 0 points straight up, 32 level and 64 straight down
    // (FUN_00469360: frame = (0x400 - pitch) * 2 / 64).
    CHECK(aimPoseFrame(90.0f) == Approx(0.0f));
    CHECK(aimPoseFrame(0.0f) == Approx(32.0f));
    CHECK(aimPoseFrame(-90.0f) == Approx(64.0f));
    CHECK(aimPoseFrame(45.0f) == Approx(16.0f));
}

TEST_CASE("Weapons: each weapon has the original's model and animations", "[game][weapons]") {
    // From the executable's weapon table: the right model in the hand
    // (a Tommy gun is not a rifle) and guns firing from the aiming pose.
    CHECK(weaponSpec(WeaponId::MachineGun).model == "WE_MGUN");
    CHECK(weaponSpec(WeaponId::Shotgun).model == "WE_BLUND");
    CHECK(weaponSpec(WeaponId::SniperRifle).model == "WE_TELR");
    CHECK(weaponSpec(WeaponId::Rifle).aimAnimation == 24u);
    CHECK(weaponSpec(WeaponId::Rifle).drawAnimation == 13u);
    CHECK(weaponSpec(WeaponId::Rifle).useAnimation == kNoAnimation);
    CHECK(weaponSpec(WeaponId::Grenade).aimAnimation == kNoAnimation);
    CHECK(weaponSpec(WeaponId::Grenade).useAnimation == 19u);
    CHECK(weaponSpec(WeaponId::Mine).useAnimation == 77u);
}

TEST_CASE("Carried objects are rigid and turned half a turn on their bone", "[scene][weapons]") {
    // The original renderer draws a weapon as one piece on the hand bone,
    // ignoring the bones stored in its vertices, turned half a turn: without
    // that turn every gun pointed backwards.
    formats::Model model;
    model.vertices.push_back({10, 20, 30, 7});  // bone 7 in the file: ignored
    formats::ModelFace face;
    face.corners = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    model.faces.push_back(face);

    scene::Pose pose(15, glm::mat4(1.0f));
    pose[scene::kHandBone] = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 0.0f, 0.0f));
    const auto mesh = scene::buildCarriedModelMesh(model, {}, pose, scene::kHandBone);
    REQUIRE(mesh.size() == 1u);
    REQUIRE_FALSE(mesh[0].vertices.empty());
    checkClose(mesh[0].vertices[0].position, {100.0f - 10.0f, 20.0f, -30.0f});
}

TEST_CASE("World: the weapon is taken out standing, aimed, put away running", "[game][world][weapons]") {
    // As the original: standing still the hog takes its weapon out, then
    // holds the aiming pose for its angle; running puts the weapon away.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1500, 0, 1)};
    hogs[0].weapons = {WeaponId::Rifle};
    World world(map, hogs);
    const auto animations = makeAnimations();

    world.update(kStep, PlayerInput{}, animations);
    REQUIRE(world.hogs()[0].drawnWeapon == WeaponId::Rifle);
    CHECK(world.hogs()[0].animation.animation() == weaponSpec(WeaponId::Rifle).drawAnimation);

    for (int i = 0; i < 50; ++i) world.update(kStep, PlayerInput{}, animations);
    const Hog& aiming = world.hogs()[0];
    REQUIRE(aiming.held.has_value());
    CHECK(aiming.held->repeat == AnimationPlayer::Repeat::Hold);
    CHECK(aiming.animation.animation() == weaponSpec(WeaponId::Rifle).aimAnimation);
    CHECK(aiming.animation.frame() == Approx(aimPoseFrame(aiming.aimDegrees)));

    world.update(kStep, PlayerInput{forwardInput()}, animations);
    world.update(kStep, PlayerInput{forwardInput()}, animations);
    CHECK_FALSE(world.hogs()[0].drawnWeapon.has_value());
}

TEST_CASE("Movement: the controlled hog stays inside the play area", "[game][movement]") {
    // The executable stops the player's hog one block short of the map's
    // edge; only hogs thrown by blasts can reach the edge (and fall off).
    const MapCollision map{TerrainHeightField(fullSizeTerrain()), {}};
    std::vector<Hog> hogs{makeHog(map, 12000.0f, 0.0f)};
    const auto animations = makeAnimations();
    MovementInput input = forwardInput();
    input.keepInPlayArea = true;
    for (int i = 0; i < 100; ++i) moveHog(hogs, 0, input, true, kStep, map, animations);
    CHECK(hogs[0].position.x <= kPlayAreaHalfSize);
    CHECK(hogs[0].position.x > kPlayAreaHalfSize - 50.0f);

    input.keepInPlayArea = false;
    for (int i = 0; i < 100; ++i) moveHog(hogs, 0, input, true, kStep, map, animations);
    CHECK(hogs[0].position.x > kPlayAreaHalfSize + 100.0f);
}

TEST_CASE("Movement: no jumping off a wall tile", "[game][movement]") {
    // "Can't jump from this tile type": wall tiles are too steep to jump
    // from, so a hog cannot hop up a trench wall.
    auto terrain = makeTerrain([](int, int) { return 0; });
    for (auto& block : terrain.blocks) {
        for (auto& tile : block.tiles) tile.isWall = true;
    }
    const MapCollision map{TerrainHeightField(terrain), {}};
    std::vector<Hog> hogs{makeHog(map, 0, 0)};
    MovementInput jump;
    jump.jump = true;
    const auto events = moveHog(hogs, 0, jump, true, kStep, map, makeAnimations());
    CHECK_FALSE(events.tookOff);
}

TEST_CASE("World: swimming hurts, except the officers; drowned hogs sink", "[game][world][movement]") {
    // In the original only Commandos (ace) and Heroes (legend) swim safely;
    // the others lose health every second in the water, and a hog that dies
    // there plays the sinking animation. Water everywhere but the first
    // tiles, so nobody reaches the shore during the test.
    const auto map = mapWithWater(-1536);
    std::vector<Hog> hogs{makeHog(map, -1800, 0, 0), hogOfClass(map, "GR_ME", 1800, 0, 1),
                          hogOfClass(map, "AC_ME", 1800, 1200, 1)};
    World world(map, hogs);
    const auto animations = makeAnimations();
    for (int i = 0; i < 51; ++i) world.update(kStep, PlayerInput{}, animations);  // just over a second

    CHECK(world.hogs()[1].health == Approx(kMaxHealth - kSwimDamagePerSecond));
    CHECK(world.hogs()[2].health == Approx(kMaxHealth));

    // Swimming towards the far shore takes longer than the grunt's health.
    for (int i = 0; i < 50 * 12 && world.hogs()[1].alive; ++i) world.update(kStep, PlayerInput{}, animations);
    CHECK_FALSE(world.hogs()[1].alive);
    CHECK(world.hogs()[1].animation.animation() == kDrowningAnimation);
}

TEST_CASE("World: a hog in the water that nobody controls swims to the shore", "[game][world][movement]") {
    // A hog knocked into the water swims back to the nearest dry land by
    // itself, as in the original.
    const auto map = mapWithWater();
    std::vector<Hog> hogs{makeHog(map, -1500, 0, 0), hogOfClass(map, "AC_ME", 600, 0, 1)};
    hogs[1].yawRadians = glm::radians(90.0f);  // facing away from the shore
    World world(map, hogs);
    const auto animations = makeAnimations();
    for (int i = 0; i < 50 * 6; ++i) world.update(kStep, PlayerInput{}, animations);
    CHECK(world.hogs()[1].position.x < 0.0f);
    CHECK(world.hogs()[1].motion.state != MotionState::Swimming);
}
