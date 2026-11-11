// Tests for weapons: the weapon table, damage and death, explosions,
// bullets, projectiles, and firing during a turn.
//
// Weapons decide who wins a match, so their rules must be exact and
// reproducible: the same shot must always do the same thing. The tests
// place hogs by hand on flat ground, at distances where the expected
// damage, range or bounce can be worked out on paper.

#include "openhogs/game/Combat.h"
#include "openhogs/game/Projectiles.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/game/World.h"
#include "openhogs/scene/CharacterLibrary.h"

#include "GameTestSupport.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/constants.hpp>

#include <set>

using namespace openhogs;
using namespace openhogs::game;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

// Two hogs of different teams, 400 units apart on flat ground, facing
// each other along X: hog 0 (team 0) at x = 0, hog 1 (team 1) at x = 400.
std::vector<Hog> duel(const MapCollision& map, std::vector<WeaponId> weapons) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 400, 0, 1)};
    hogs[1].yawRadians = glm::pi<float>();
    for (auto& hog : hogs) {
        hog.weapons = weapons;
        hog.aimDegrees = 0.0f;
    }
    return hogs;
}

std::vector<GameEvent> runWorld(World& world, const PlayerInput& input, int steps) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto events = world.update(kStep, input, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

PlayerInput fireKey() {
    PlayerInput input;
    input.fire = true;
    return input;
}

int count(const std::vector<GameEvent>& events, GameEventType type) {
    int n = 0;
    for (const auto& event : events) n += event.type == type ? 1 : 0;
    return n;
}

} // namespace

// Every weapon has the data its kind needs, every class carries only
// weapons that exist, and weapon ids are the game text entries, so the
// names shown come from the (translatable) text table.
TEST_CASE("Weapons: the table is complete and consistent", "[game][weapons]") {
    std::set<int> ids;
    for (const auto& spec : allWeapons()) {
        INFO(spec.fallbackName);
        CHECK(ids.insert(static_cast<int>(spec.id)).second);
        CHECK(static_cast<int>(spec.id) >= 97);  // TROTTER is text entry 97
        switch (spec.kind) {
        case WeaponKind::Melee:
        case WeaponKind::Bullet:
            CHECK(spec.range > 0.0f);
            CHECK(spec.damage > 0.0f);
            break;
        case WeaponKind::Thrown:
            CHECK(spec.fuseSeconds > 0.0f);
            CHECK(spec.launchSpeed > 0.0f);
            CHECK(spec.blastRadius > 0.0f);
            break;
        case WeaponKind::Shell:
            CHECK(spec.launchSpeed > 0.0f);
            CHECK(spec.blastRadius > 0.0f);
            break;
        case WeaponKind::Placed:
            CHECK((spec.fuseSeconds > 0.0f || spec.triggerRadius > 0.0f));
            CHECK(spec.blastRadius > 0.0f);
            break;
        }
    }
    for (const auto& characterClass : scene::characterClasses()) {
        const auto loadout = classLoadout(characterClass.id);
        CHECK_FALSE(loadout.empty());
        for (const auto weapon : loadout) {
            CHECK_NOTHROW(weaponSpec(weapon));
        }
    }
    CHECK(classLoadout("chef").size() == 1);  // unknown class: the fist
}

// Short falls (a jump) are harmless; longer ones hurt more the higher
// they are.
TEST_CASE("Combat: fall damage grows with the height of the fall", "[game][combat]") {
    CHECK(fallDamage(170.0f) == 0.0f);
    CHECK(fallDamage(kHarmlessFallHeight) == 0.0f);
    CHECK(fallDamage(kHarmlessFallHeight + 200.0f) == Approx(20.0f));
    CHECK(fallDamage(2000.0f) > fallDamage(1000.0f));
}

// Health never goes below zero, a hog dies exactly once, and the dead
// take no more damage (an explosion over a body must not "kill" it again).
TEST_CASE("Combat: damage, death and nothing more for the dead", "[game][combat]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0)};
    std::vector<GameEvent> events;

    damageHog(hogs, 0, 30.0f, events);
    CHECK(hogs[0].health == Approx(70.0f));
    CHECK(count(events, GameEventType::Hurt) == 1);

    damageHog(hogs, 0, 500.0f, events);
    CHECK(hogs[0].health == 0.0f);
    CHECK_FALSE(hogs[0].alive);
    CHECK(hogs[0].motion.state == MotionState::Dead);
    CHECK(count(events, GameEventType::Died) == 1);
    CHECK(events.back().amount == Approx(70.0f));  // only what it had left

    damageHog(hogs, 0, 10.0f, events);
    CHECK(count(events, GameEventType::Died) == 1);
}

// Explosions hurt most at the centre and nothing beyond their radius, and
// throw hogs away from the centre (upwards too).
TEST_CASE("Combat: explosions fall off with distance and throw hogs away", "[game][combat]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0), makeHog(map, 200, 0), makeHog(map, 1000, 0)};
    std::vector<GameEvent> events;
    const glm::vec3 centre = hogCentre(hogs[0]);

    explode(hogs, centre, 400.0f, 40.0f, 900.0f, events);

    CHECK(hogs[0].health == Approx(60.0f));                  // at the centre: all 40
    CHECK(hogs[1].health == Approx(100.0f - 40.0f * 0.5f));  // half way out: half
    CHECK(hogs[2].health == 100.0f);                         // out of reach
    CHECK(hogs[1].motion.state == MotionState::Airborne);
    CHECK(hogs[1].motion.velocity.x > 0.0f);  // away from the centre
    CHECK(hogs[1].motion.velocity.y > 0.0f);
    CHECK(hogs[2].motion.state != MotionState::Airborne);
}

// A bullet stops at the first thing in its way: the nearer of two hogs,
// never the shooter, and walls or the ground protect what is behind them.
TEST_CASE("Combat: bullets hit the first thing along their line", "[game][combat]") {
    const auto map = flatMap({makeBox({600, 250, 1000}, {32, 250, 300})});
    std::vector<Hog> hogs{makeHog(map, 0, 0), makeHog(map, 300, 0), makeHog(map, 500, 0), makeHog(map, 800, 1000)};
    const glm::vec3 start = hogCentre(hogs[0]);

    const auto first = traceBullet(start, {1, 0, 0}, 5000.0f, map, hogs, 0);
    REQUIRE(first.hog.has_value());
    CHECK(*first.hog == 1u);

    // Along z = 1000 a wall at x = 600 stands before hog 3.
    const auto blocked = traceBullet({0, 150, 1000}, {1, 0, 0}, 5000.0f, map, hogs, 99);
    CHECK_FALSE(blocked.hog.has_value());
    CHECK(blocked.point.x < 600.0f);

    // Shooting down: stops at the ground.
    const auto ground = traceBullet({0, 150, -1000}, glm::normalize(glm::vec3(1, -1, 0)), 5000.0f, map, hogs, 99);
    CHECK_FALSE(ground.hog.has_value());
    CHECK(ground.point.y <= 1.0f);
}

// A shell explodes as soon as it touches the ground or a hog; the hog that
// fired it is safe for its first moments (the shell starts inside its reach).
TEST_CASE("Projectiles: shells explode on contact, not on their owner", "[game][projectiles]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0), makeHog(map, 500, 0)};
    std::vector<GameEvent> events;

    auto atOwner = launchProjectile(WeaponId::Bazooka, 0, hogCentre(hogs[0]), {1, 0, 0}, 1.0f);
    CHECK(stepProjectile(atOwner, kStep, map, hogs, events) == ProjectileOutcome::Flying);

    auto shell = launchProjectile(WeaponId::Bazooka, 0, hogCentre(hogs[0]) + glm::vec3(100, 0, 0), {1, 0, 0}, 1.0f);
    ProjectileOutcome outcome = ProjectileOutcome::Flying;
    for (int i = 0; i < 50 && outcome == ProjectileOutcome::Flying; ++i) {
        outcome = stepProjectile(shell, kStep, map, hogs, events);
    }
    CHECK(outcome == ProjectileOutcome::Explodes);
    CHECK(shell.position.x == Approx(500.0f).margin(kHogRadius + 60.0f));  // at hog 1

    auto down = launchProjectile(WeaponId::Mortar, 0, {0, 300, 2000}, {0, -1, 0}, 1.0f);
    outcome = ProjectileOutcome::Flying;
    for (int i = 0; i < 50 && outcome == ProjectileOutcome::Flying; ++i) {
        outcome = stepProjectile(down, kStep, map, hogs, events);
    }
    CHECK(outcome == ProjectileOutcome::Explodes);
    CHECK(down.position.y <= 1.0f);
}

// A grenade bounces lower and lower, comes to rest, and explodes when its
// fuse runs out, wherever it is.
TEST_CASE("Projectiles: grenades bounce, settle and explode on their fuse", "[game][projectiles]") {
    const auto map = flatMap();
    std::vector<Hog> hogs;
    std::vector<GameEvent> events;
    auto grenade = launchProjectile(WeaponId::Grenade, 0, {0, 400, 0}, glm::normalize(glm::vec3(1, 1, 0)), 0.5f);

    ProjectileOutcome outcome = ProjectileOutcome::Flying;
    int steps = 0;
    bool rested = false;
    while (outcome == ProjectileOutcome::Flying && steps < 500) {
        outcome = stepProjectile(grenade, kStep, map, hogs, events);
        rested = rested || grenade.resting;
        ++steps;
    }
    CHECK(outcome == ProjectileOutcome::Explodes);
    CHECK(steps * kStep == Approx(weaponSpec(WeaponId::Grenade).fuseSeconds).margin(2 * kStep));
    CHECK(count(events, GameEventType::Bounce) >= 1);
    CHECK(rested);
    CHECK(grenade.position.y == Approx(0.0f).margin(1.0f));
}

// Anything falling into water is lost with a splash, and anything leaving
// the map is gone (it must not explode out of sight or fly for ever).
TEST_CASE("Projectiles: water and the edge of the map", "[game][projectiles]") {
    auto terrain = makeTerrain([](int, int) { return 0; });
    for (auto& block : terrain.blocks) {
        for (auto& tile : block.tiles) tile.isWater = true;
    }
    const MapCollision wet{TerrainHeightField(terrain), {}};
    std::vector<Hog> hogs;
    std::vector<GameEvent> events;

    auto shell = launchProjectile(WeaponId::Mortar, 0, {0, 200, 0}, {0, -1, 0}, 1.0f);
    ProjectileOutcome outcome = ProjectileOutcome::Flying;
    for (int i = 0; i < 50 && outcome == ProjectileOutcome::Flying; ++i) {
        outcome = stepProjectile(shell, kStep, wet, hogs, events);
    }
    CHECK(outcome == ProjectileOutcome::Gone);
    CHECK(count(events, GameEventType::Splash) == 1);

    auto away = launchProjectile(WeaponId::RocketLauncher, 0, {1900, 500, 0}, {1, 0, 0}, 1.0f);
    outcome = ProjectileOutcome::Flying;
    for (int i = 0; i < 50 && outcome == ProjectileOutcome::Flying; ++i) {
        outcome = stepProjectile(away, kStep, flatMap(), hogs, events);
    }
    CHECK(outcome == ProjectileOutcome::Gone);
}

// A mine only goes off once armed (so its owner can walk away), and then
// as soon as a living hog comes near; TNT simply waits for its fuse.
TEST_CASE("Projectiles: mines arm and trigger, TNT waits for its fuse", "[game][projectiles]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 50, 0)};
    std::vector<GameEvent> events;

    auto mine = launchProjectile(WeaponId::Mine, 0, {0, 0, 0}, {0, 0, 0}, 0.0f);
    int steps = 0;
    ProjectileOutcome outcome = ProjectileOutcome::Flying;
    while (outcome == ProjectileOutcome::Flying && steps < 500) {
        outcome = stepProjectile(mine, kStep, map, hogs, events);
        ++steps;
    }
    CHECK(outcome == ProjectileOutcome::Explodes);
    CHECK(steps * kStep == Approx(kMineArmingSeconds).margin(2 * kStep));

    std::vector<Hog> nobody;
    auto tnt = launchProjectile(WeaponId::Tnt, 0, {0, 0, 0}, {0, 0, 0}, 0.0f);
    steps = 0;
    outcome = ProjectileOutcome::Flying;
    while (outcome == ProjectileOutcome::Flying && steps < 500) {
        outcome = stepProjectile(tnt, kStep, map, nobody, events);
        ++steps;
    }
    CHECK(steps * kStep == Approx(weaponSpec(WeaponId::Tnt).fuseSeconds).margin(2 * kStep));
}

// The aiming arc shown to the player must end where a real throw would
// land: here the classic range of a 45-degree throw from ground level,
// v^2 / g.
TEST_CASE("Projectiles: the predicted arc lands where physics says", "[game][projectiles]") {
    const auto map = flatMap();
    const float speed = 1000.0f;
    const glm::vec3 velocity = glm::normalize(glm::vec3(1, 1, 0)) * speed;
    const auto arc = predictTrajectory({-1500, 1, 0}, velocity, 1.0f, map, 0.02f, 200);
    REQUIRE_FALSE(arc.empty());
    CHECK(arc.back().x == Approx(-1500.0f + speed * speed / kGravity).margin(15.0f));
    CHECK(arc.back().y <= 1.0f);
}

// Holding fire charges a throw; releasing it launches the grenade with
// that much power, the turn moves to its Resolving phase (no more control),
// and once everything has settled the next team plays.
TEST_CASE("World: charge, throw, settle, next turn", "[game][world][weapons]") {
    const auto map = flatMap();
    World world(map, duel(map, {WeaponId::Grenade}));

    runWorld(world, fireKey(), 38);  // 0.76 s held: about half power
    CHECK(world.hogs()[0].charging);
    CHECK(world.hogs()[0].power == Approx(0.76f / kSecondsToFullPower).margin(0.02f));
    CHECK(world.projectiles().empty());

    auto events = runWorld(world, PlayerInput{}, 1);  // release
    CHECK(count(events, GameEventType::Fired) == 1);
    REQUIRE(world.projectiles().size() == 1);
    CHECK(glm::length(world.projectiles()[0].velocity) ==
          Approx(weaponSpec(WeaponId::Grenade).launchSpeed * 0.5f).margin(60.0f));
    CHECK(world.phase() == MatchPhase::Resolving);

    // No control while the shot resolves: the hog does not walk.
    const float x = world.hogs()[0].position.x;
    PlayerInput walk;
    walk.movement.forward = true;
    runWorld(world, walk, 10);
    CHECK(world.hogs()[0].position.x == x);

    events = runWorld(world, PlayerInput{}, 50 * 8);
    CHECK(count(events, GameEventType::Explosion) == 1);
    CHECK(count(events, GameEventType::TurnStarted) == 1);
    CHECK(world.activeHog() == 1u);
    CHECK(world.phase() == MatchPhase::Playing);
}

// A rifle shot hits the hog in the line of fire for the weapon's damage;
// a burst fires all its shots, one after another.
TEST_CASE("World: bullets and bursts damage the target", "[game][world][weapons]") {
    const auto map = flatMap();
    World rifle(map, duel(map, {WeaponId::Rifle}));
    runWorld(rifle, fireKey(), 1);
    runWorld(rifle, PlayerInput{}, 5);
    CHECK(rifle.hogs()[1].health == Approx(100.0f - weaponSpec(WeaponId::Rifle).damage));

    World burst(map, duel(map, {WeaponId::RifleBurst}));
    const auto events = runWorld(burst, fireKey(), 1);
    const auto later = runWorld(burst, PlayerInput{}, 30);
    CHECK(count(events, GameEventType::Fired) + count(later, GameEventType::Fired) == 3);
    CHECK(burst.hogs()[1].health < 100.0f);
}

// Melee only reaches a hog right in front: not one behind the attacker,
// not one out of reach.
TEST_CASE("World: melee hits only in front and within reach", "[game][world][weapons]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 120, 0, 1), makeHog(map, -120, 0, 1),
                          makeHog(map, 0, 600, 1)};
    hogs[0].weapons = {WeaponId::Sword};
    World world(map, hogs);

    runWorld(world, fireKey(), 1);

    CHECK(world.hogs()[1].health == Approx(100.0f - weaponSpec(WeaponId::Sword).damage));
    CHECK(world.hogs()[2].health == 100.0f);  // behind
    CHECK(world.hogs()[3].health == 100.0f);  // too far
}

// When the last hog of the other team dies, the match is over and the
// surviving team wins; dead hogs no longer get turns.
TEST_CASE("World: killing the last enemy wins the match", "[game][world][weapons]") {
    const auto map = flatMap();
    auto hogs = duel(map, {WeaponId::SniperRifle});
    hogs[1].health = 10.0f;
    World world(map, hogs);

    runWorld(world, fireKey(), 1);
    const auto events = runWorld(world, PlayerInput{}, 50 * 5);

    CHECK_FALSE(world.hogs()[1].alive);
    CHECK(world.phase() == MatchPhase::Over);
    REQUIRE(world.winner().has_value());
    CHECK(*world.winner() == 0);
    CHECK(count(events, GameEventType::MatchOver) == 1);
}

// A map with one team only (the training missions) does not end as soon
// as it starts.
TEST_CASE("World: a single team is not an instant victory", "[game][world][weapons]") {
    const auto map = flatMap();
    World world(map, {makeHog(map, 0, 0, 0), makeHog(map, 500, 0, 0)});
    runWorld(world, PlayerInput{}, 50);
    CHECK(world.phase() == MatchPhase::Playing);
}

// A cluster grenade breaks into bomblets when it explodes, and they
// explode in turn.
TEST_CASE("World: cluster grenades release bomblets", "[game][world][weapons]") {
    const auto map = flatMap();
    World world(map, duel(map, {WeaponId::ClusterGrenade}));
    runWorld(world, fireKey(), 10);
    runWorld(world, PlayerInput{}, 1);

    const auto events = runWorld(world, PlayerInput{}, 50 * 8);
    CHECK(count(events, GameEventType::Explosion) == 1 + weaponSpec(WeaponId::ClusterGrenade).bomblets);
}

// An explosion sets off the explosives lying within its radius, so TNT
// next to a mine field can clear it.
TEST_CASE("World: explosions set off nearby explosives", "[game][world][weapons]") {
    const auto map = flatMap();
    auto hogs = duel(map, {WeaponId::Tnt});
    hogs[1].position.x = 1500.0f;
    World world(map, hogs);

    runWorld(world, fireKey(), 1);  // TNT at the hog's feet, 4 s fuse
    // Walking is not possible while the shot resolves, so the hog stays by
    // its TNT; it is the explosion we are after.
    const auto events = runWorld(world, PlayerInput{}, 50 * 6);
    CHECK(count(events, GameEventType::Explosion) == 1);
    CHECK(world.hogs()[0].health < 100.0f);
}

// Weapons are picked in a cycle, but not in the middle of charging a
// throw; while charging the hog may turn to aim but not walk away.
TEST_CASE("World: changing weapon and moving while charging", "[game][world][weapons]") {
    const auto map = flatMap();
    World world(map, duel(map, {WeaponId::Trotter, WeaponId::Grenade, WeaponId::Rifle}));

    PlayerInput next;
    next.weaponStep = 1;
    runWorld(world, next, 1);
    CHECK(world.hogs()[0].selectedWeapon() == WeaponId::Grenade);

    runWorld(world, fireKey(), 1);  // starts charging the grenade
    PlayerInput chargeAndMove = fireKey();
    chargeAndMove.weaponStep = 1;
    chargeAndMove.movement.forward = true;
    chargeAndMove.movement.turnLeft = true;
    const float yaw = world.hogs()[0].yawRadians;
    runWorld(world, chargeAndMove, 10);
    CHECK(world.hogs()[0].selectedWeapon() == WeaponId::Grenade);
    CHECK(world.hogs()[0].position.x == 0.0f);
    CHECK(world.hogs()[0].yawRadians != yaw);
}

// A hog thrown off a high platform by an explosion, or walking off it,
// is hurt by the fall when it lands.
TEST_CASE("World: long falls hurt", "[game][world][weapons]") {
    // A platform 1000 high for x < 200.
    const auto map = flatMap({makeBox({-800, 500, 0}, {1000, 500, 1000})});
    std::vector<Hog> hogs{makeHog(map, -100, 0, 0), makeHog(map, 1500, 0, 1)};
    hogs[0].position.y = 1000.0f;
    World world(map, hogs);

    PlayerInput walk;
    walk.movement.forward = true;
    runWorld(world, walk, 100);

    CHECK(world.hogs()[0].position.y == 0.0f);
    CHECK(world.hogs()[0].health == Approx(100.0f - fallDamage(1000.0f)).margin(1.0f));
}
