// Tests for the buildings and vehicles hogs get into (game/Mounts.h and
// World). They protect the rules the Hogs of War Wiki gives for them:
// the hog inside is safe while the object takes the harm, comes out hurt
// when it is destroyed, uses the object's weapons (whose shots stay with
// the object), and tanks drive; getting in and out does not use up the
// turn. Also the actions every hog has: SKIP TURN, MAP VIEW, BINOCULARS.

#include "GameTestSupport.h"

#include "openhogs/game/Mounts.h"
#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>

using namespace openhogs;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> run(World& world, const PlayerInput& input, int steps) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto step = world.update(kStep, input, animations);
        all.insert(all.end(), step.begin(), step.end());
    }
    return all;
}

PlayerInput firing() {
    PlayerInput input;
    input.fire = true;
    return input;
}

bool carries(const Hog& hog, WeaponId weapon) {
    return std::find(hog.weapons.begin(), hog.weapons.end(), weapon) != hog.weapons.end();
}

// Selects `weapon` in the active hog's list by stepping through it.
void select(World& world, WeaponId weapon) {
    for (int i = 0; i < 20 && world.hogs()[*world.activeHog()].selectedWeapon() != weapon; ++i) {
        PlayerInput next;
        next.weaponStep = 1;
        run(world, next, 1);
    }
}

// A building (or vehicle) at x = 300 and a hog of team 0 beside it, with
// an enemy far off to the east facing it with a rocket launcher.
struct Scene {
    MapCollision map;
    std::vector<Hog> hogs;
};

Scene sceneWith(float health = 100.0f, float hogX = 120.0f) {
    CollisionBox box;
    box.centre = {300.0f, 100.0f, 0.0f};
    box.halfSize = {100.0f, 100.0f, 100.0f};
    box.object = 0;
    Scene scene{flatMap({box}), {}};
    scene.hogs = {makeHog(scene.map, hogX, 0, 0), makeHog(scene.map, 1800, 0, 1)};
    scene.hogs[0].weapons = {WeaponId::Rifle};
    scene.hogs[1].weapons = {WeaponId::RocketLauncher};
    scene.hogs[1].yawRadians = glm::pi<float>();
    scene.hogs[1].aimDegrees = 0.0f;
    (void)health;
    return scene;
}

World worldWith(MountKind kind, float health = 100.0f, float hogX = 120.0f) {
    Scene scene = sceneWith(health, hogX);
    World world(scene.map, scene.hogs);
    world.setDestructibles({{0, kind == MountKind::Tank ? ObjectClass::Vehicle : ObjectClass::Building, health, false}});
    world.setMounts({{0, kind}});
    return world;
}

void getIn(World& world) {
    const bool vehicle = mountSpec(world.mounts()[0].kind).vehicle;
    select(world, vehicle ? WeaponId::VehicleInOut : WeaponId::BuildingInOut);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
}

} // namespace

TEST_CASE("Which map objects a hog can get into", "[game][mounts]") {
    CHECK(mountKind("BIG_GUN") == MountKind::Artillery);
    CHECK(mountKind("pillbox") == MountKind::Pillbox);
    CHECK(mountKind("M_TENT2") == MountKind::MashTent);
    CHECK(mountKind("CARRY") == MountKind::TroopCarrier);
    CHECK_FALSE(mountKind("TREEG"));
    CHECK(mountSpec(MountKind::Artillery).highestAimDegrees == 45.0f);  // the wiki: horizontal to 45 degrees
}

TEST_CASE("Getting into a pillbox: its weapons, the turn goes on", "[game][mounts]") {
    World world = worldWith(MountKind::Pillbox);
    const Hog& hog = world.hogs()[0];
    REQUIRE(carries(hog, WeaponId::BuildingInOut));  // offered beside it
    CHECK(hog.weapons.back() == WeaponId::SkipTurn);
    getIn(world);
    CHECK(hog.sheltered());
    CHECK(carries(hog, WeaponId::HeavyMachineGun));
    CHECK(carries(hog, WeaponId::FlameThrower));
    CHECK_FALSE(carries(hog, WeaponId::Rifle));  // its own weapons are put by
    CHECK(world.activeHog() == std::optional<std::size_t>(0));
    CHECK(world.phase() == MatchPhase::Playing);

    // And out again, with its rifle back.
    select(world, WeaponId::BuildingInOut);
    run(world, firing(), 1);
    CHECK_FALSE(hog.sheltered());
    CHECK(carries(hog, WeaponId::Rifle));
    CHECK(world.activeHog() == std::optional<std::size_t>(0));
}

TEST_CASE("Inside, the building takes the harm; destroyed, it throws the hog out hurt", "[game][mounts]") {
    World safe = worldWith(MountKind::Pillbox, 100.0f);
    getIn(safe);
    select(safe, WeaponId::SkipTurn);
    run(safe, firing(), 1);
    run(safe, PlayerInput{}, 50 * 3);
    REQUIRE(safe.activeHog() == std::optional<std::size_t>(1));
    run(safe, firing(), 1);  // the enemy's rocket strikes the pillbox
    run(safe, PlayerInput{}, 50 * 3);
    CHECK(safe.hogs()[0].health == Approx(kMaxHealth));
    CHECK(safe.destructibles()[0].health < 100.0f);
    CHECK(safe.hogs()[0].sheltered());

    World weak = worldWith(MountKind::Pillbox, 5.0f);
    getIn(weak);
    select(weak, WeaponId::SkipTurn);
    run(weak, firing(), 1);
    run(weak, PlayerInput{}, 50 * 3);
    run(weak, firing(), 1);
    const auto events = run(weak, PlayerInput{}, 50 * 3);
    CHECK(weak.destructibles()[0].destroyed);
    CHECK_FALSE(weak.hogs()[0].sheltered());
    CHECK(weak.hogs()[0].health == Approx(kMaxHealth - mountSpec(MountKind::Pillbox).damageWhenDestroyed));
    CHECK(std::any_of(events.begin(), events.end(), [](const GameEvent& e) { return e.type == GameEventType::HogDismounted; }));
}

TEST_CASE("A tank drives, its box with it", "[game][mounts]") {
    World world = worldWith(MountKind::Tank);
    getIn(world);
    REQUIRE(world.hogs()[0].sheltered());
    CHECK(carries(world.hogs()[0], WeaponId::Bazooka));
    PlayerInput drive;
    drive.movement.forward = true;
    run(world, drive, 50);
    const Mount& tank = world.mounts()[0];
    CHECK(glm::length(tank.moved) > 300.0f);
    CHECK(world.hogs()[0].position.x == Approx(300.0f + tank.moved.x).margin(1.0f));
    const auto box = std::find_if(world.map().boxes().begin(), world.map().boxes().end(),
                                  [](const CollisionBox& b) { return b.object == 0; });
    REQUIRE(box != world.map().boxes().end());
    CHECK(box->centre.x == Approx(300.0f + tank.moved.x).margin(1.0f));
}

TEST_CASE("A hog the map puts inside a building starts in it; a medic tent heals it", "[game][mounts]") {
    Scene scene = sceneWith(100.0f, 300.0f);  // standing inside the box
    scene.hogs[0].health = 20.0f;
    World world(scene.map, scene.hogs);
    world.setMounts({{0, MountKind::MedicTent}});
    CHECK(world.hogs()[0].sheltered());
    world.endTurn();
    run(world, PlayerInput{}, 1);
    world.endTurn();  // back to hog 0: the tent heals it
    run(world, PlayerInput{}, 1);
    CHECK(world.hogs()[0].health == Approx(70.0f));
}

TEST_CASE("Map view and binoculars switch on and off without using up the turn", "[game][mounts]") {
    Scene scene = sceneWith();
    scene.hogs[0].weapons = {WeaponId::MapView, WeaponId::Binoculars};
    World world(scene.map, scene.hogs);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
    CHECK(world.mapView());
    CHECK(world.phase() == MatchPhase::Playing);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
    CHECK_FALSE(world.mapView());

    select(world, WeaponId::Binoculars);
    run(world, firing(), 1);
    CHECK(world.binocularView());
    CHECK(world.activeHog() == std::optional<std::size_t>(0));
}
