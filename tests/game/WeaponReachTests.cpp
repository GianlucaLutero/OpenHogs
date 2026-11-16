// Tests for the weapons added by block weapons-complete (part 3): weapons
// that reach far (airstrikes, special ops), fly (jetpack), steer (homing
// and guided missiles) or leave something behind (mine shell), and the
// confusion gas. They protect how these change a turn: where the danger
// lands, who moves where, and when the turn is over.

#include "GameTestSupport.h"

#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

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

// Holds fire long enough to charge `power`, then lets go.
std::vector<GameEvent> fireAtPower(World& world, float power) {
    const int steps = static_cast<int>(std::lround(power * kSecondsToFullPower / kStep));
    auto events = run(world, firing(), steps);
    const auto release = run(world, PlayerInput{}, 1);
    events.insert(events.end(), release.begin(), release.end());
    return events;
}

int count(const std::vector<GameEvent>& events, GameEventType type) {
    return static_cast<int>(std::count_if(events.begin(), events.end(), [type](const GameEvent& e) { return e.type == type; }));
}

// The shooter at the origin facing +x; one enemy at (x, z).
std::vector<Hog> shooterAndEnemy(const MapCollision& map, WeaponId weapon, float x, float z) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, x, z, 1)};
    hogs[0].weapons = {weapon};
    hogs[1].weapons = {WeaponId::Rifle};
    hogs[0].aimDegrees = 0.0f;
    return hogs;
}

} // namespace

TEST_CASE("An airstrike drops its bombs on the spot the power chooses", "[game][weapons][reach]") {
    // Five bombs fall across a spot 15% of the strike's reach ahead:
    // the enemy standing there is hurt, the hog that called it is not.
    const auto map = flatMap();
    const WeaponSpec& strike = weaponSpec(WeaponId::Airstrike);
    World world(map, shooterAndEnemy(map, WeaponId::Airstrike, strike.range * 0.15f, 0));
    auto events = fireAtPower(world, 0.15f);
    const auto later = run(world, PlayerInput{}, 50 * 6);
    events.insert(events.end(), later.begin(), later.end());
    CHECK(count(events, GameEventType::Explosion) == strike.shots);
    CHECK(world.hogs()[1].health < kMaxHealth);
    CHECK(world.hogs()[0].health == Approx(kMaxHealth));
}

TEST_CASE("Special ops drop the hog unhurt on the chosen spot, then the turn ends", "[game][weapons][reach]") {
    const auto map = flatMap();
    const WeaponSpec& drop = weaponSpec(WeaponId::SpecialOps);
    World world(map, shooterAndEnemy(map, WeaponId::SpecialOps, 0, 3000));
    fireAtPower(world, 0.5f);
    run(world, PlayerInput{}, 50 * 12);
    const Hog& hog = world.hogs()[0];
    CHECK(hog.position.x == Approx(drop.range * 0.5f).margin(100.0f));
    CHECK(hog.health == Approx(kMaxHealth));  // the parachute: no fall damage
    CHECK_FALSE(hog.parachute);
    CHECK(world.activeHog() == std::optional<std::size_t>(1));
}

TEST_CASE("The jetpack flies where it is steered and the turn ends after the parachute", "[game][weapons][reach]") {
    // Climbing and flying forward takes the hog up and ahead; when the fuel
    // is gone it floats down unhurt and the other team plays.
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::Jetpack, 0, 4000));
    run(world, firing(), 1);
    PlayerInput fly;
    fly.movement.forward = true;
    fly.movement.jump = true;
    run(world, fly, 50);
    CHECK(world.hogs()[0].position.y > 300.0f);
    CHECK(world.hogs()[0].position.x > 300.0f);
    CHECK(world.activeHog() == std::optional<std::size_t>(0));  // still its turn while it flies
    run(world, PlayerInput{}, 50 * 15);
    CHECK(world.hogs()[0].health == Approx(kMaxHealth));
    CHECK(world.activeHog() == std::optional<std::size_t>(1));
}

TEST_CASE("A homing missile finds an enemy off its line of fire", "[game][weapons][reach]") {
    // Fired straight ahead, it turns to the enemy standing off to the side.
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::HomingMissile, 1500, -900));
    run(world, firing(), 1);
    run(world, PlayerInput{}, 50 * 6);
    CHECK(world.hogs()[1].health < kMaxHealth);
}

TEST_CASE("A guided missile follows the player's steering", "[game][weapons][reach]") {
    // Turned a quarter left after firing, it reaches an enemy it could not
    // have hit flying straight.
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::GuidedMissile, 960, -1800));
    run(world, firing(), 1);
    PlayerInput left;
    left.movement.turnLeft = true;
    run(world, left, 50);  // 90 degrees at 90 degrees per second
    run(world, PlayerInput{}, 50 * 6);
    CHECK(world.hogs()[1].health < kMaxHealth);
}

TEST_CASE("A mine shell leaves a mine where it bursts", "[game][weapons][reach]") {
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::MineShell, 0, 5000));
    world.hogs();
    fireAtPower(world, 0.4f);
    run(world, PlayerInput{}, 50 * 3);
    const auto& left = world.projectiles();
    CHECK(std::any_of(left.begin(), left.end(), [](const Projectile& p) { return p.weapon == WeaponId::Mine; }));
}

TEST_CASE("A confused hog plays its next turn with reversed controls", "[game][weapons][effects]") {
    // Asked to go forward, it walks back; only for that turn.
    const auto map = flatMap();
    auto hogs = shooterAndEnemy(map, WeaponId::SkipTurn, 2000, 0);
    hogs[1].confused = true;  // facing +x like the others
    World world(map, hogs);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 50 * 3);  // hog 0 passes; hog 1's turn
    REQUIRE(world.activeHog() == std::optional<std::size_t>(1));
    CHECK(world.hogs()[1].controlsReversed);
    PlayerInput forward;
    forward.movement.forward = true;
    run(world, forward, 25);
    CHECK(world.hogs()[1].position.x < 2000.0f);
}
