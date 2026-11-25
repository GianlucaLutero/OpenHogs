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

// Moves the crosshair of an airstrike or special ops: one press of fire
// opens the aerial view, `move` is held for `steps`, a second press
// confirms the spot.
std::vector<GameEvent> chooseSpot(World& world, const MovementInput& move, int steps) {
    auto events = run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
    PlayerInput moving;
    moving.movement = move;
    run(world, moving, steps);
    const auto confirmed = run(world, firing(), 1);
    events.insert(events.end(), confirmed.begin(), confirmed.end());
    return events;
}

MovementInput forward() {
    MovementInput move;
    move.forward = true;
    return move;
}

TEST_CASE("Airstrike: the first press opens the aerial view and the keys move the crosshair, not the hog",
          "[game][weapons][reach]") {
    // The manual: such skills "present you with an aerial view of the map -
    // move your crosshair with the cursor keys". While the spot is chosen
    // the hog must stay put and nothing may be fired yet.
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::Airstrike, 1000, 0));
    auto events = run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
    REQUIRE(world.hogs()[0].strikeAim.has_value());
    PlayerInput moving;
    moving.movement = forward();
    moving.movement.turnLeft = true;
    events = run(world, moving, 25);
    const Hog& hog = world.hogs()[0];
    CHECK(hog.position.x == Approx(0.0f).margin(1.0f));  // the hog did not walk
    CHECK(hog.strikeAim->spot.x > 500.0f);                // forward: the way the hog faces (+x)
    CHECK(hog.strikeAim->spot.z < -500.0f);               // left of +x is -z
    CHECK(count(events, GameEventType::Fired) == 0);
    CHECK_FALSE(world.airship().has_value());
}

TEST_CASE("Airstrike: an airship flies over the chosen spot and its bombs hit there", "[game][weapons][reach]") {
    // The footage (part 17, 30:53-31:01): after the spot is confirmed an
    // airship comes in, lets five bombs go over the spot and flies off.
    // The enemy standing on the spot is hurt, the hog that called it is
    // not, and the turn waits for the airship before it passes on.
    const auto map = flatMap();
    const WeaponSpec& strike = weaponSpec(WeaponId::Airstrike);
    World world(map, shooterAndEnemy(map, WeaponId::Airstrike, 1320, 0));
    auto events = chooseSpot(world, forward(), 33);  // 0.66 s at 2000 units/s
    REQUIRE(world.airship().has_value());
    CHECK(world.airship()->spot.x == Approx(1320.0f).margin(50.0f));
    const auto during = run(world, PlayerInput{}, 50 * 4);
    events.insert(events.end(), during.begin(), during.end());
    CHECK(world.activeHog() == std::optional<std::size_t>(0));  // still coming in: the turn waits
    const auto later = run(world, PlayerInput{}, 50 * 10);
    events.insert(events.end(), later.begin(), later.end());
    CHECK_FALSE(world.airship().has_value());
    CHECK(count(events, GameEventType::Explosion) == strike.shots);
    CHECK(world.hogs()[1].health < kMaxHealth);
    CHECK(world.hogs()[0].health == Approx(kMaxHealth));
}

TEST_CASE("Airstrike: the aim keys turn the way the airship comes in", "[game][weapons][reach]") {
    // The manual: "use the Page up and Page down keys to rotate the angle
    // that your transport Airship will come in from"; the bombs then fall
    // in a line along that course, so turning it decides what the line
    // covers.
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::Airstrike, 1000, 0));
    run(world, firing(), 1);
    run(world, PlayerInput{}, 1);
    PlayerInput turning;
    turning.aimUp = true;
    run(world, turning, 50);  // 90 degrees at 90 per second
    run(world, firing(), 1);
    REQUIRE(world.airship().has_value());
    const glm::vec3 course = world.airship()->direction;
    CHECK(course.x == Approx(0.0f).margin(0.05f));
    CHECK(course.z == Approx(-1.0f).margin(0.05f));  // turned left from +x
}

TEST_CASE("Special ops drop the hog unhurt on the chosen spot, then the turn ends", "[game][weapons][reach]") {
    const auto map = flatMap();
    World world(map, shooterAndEnemy(map, WeaponId::SpecialOps, 0, 3000));
    chooseSpot(world, forward(), 25);  // 0.5 s at 2000 units/s
    run(world, PlayerInput{}, 50 * 12);
    const Hog& hog = world.hogs()[0];
    CHECK(hog.position.x == Approx(1000.0f).margin(100.0f));
    CHECK(hog.health == Approx(kMaxHealth));  // the parachute: no fall damage
    CHECK_FALSE(hog.parachute);
    CHECK(world.activeHog() == std::optional<std::size_t>(1));
}

TEST_CASE("The jetpack flies along the aim; fire again drops it and the turn goes on", "[game][weapons][reach]") {
    // As the Hogs of War Wiki describes it: the hog takes off in the
    // direction of its aim; pressing fire again lets go of the jetpack,
    // which falls away behind and explodes where it lands, while the hog
    // floats down unhurt under its parachute; the turn is not over.
    const auto map = flatMap();
    auto hogs = shooterAndEnemy(map, WeaponId::Jetpack, 0, 4000);
    hogs[0].aimDegrees = 30.0f;
    World world(map, hogs);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 50);
    const Hog& flying = world.hogs()[0];
    CHECK(flying.position.y > 200.0f);
    CHECK(flying.position.x > 300.0f);
    REQUIRE(flying.jetpackSecondsLeft > 0.0f);

    run(world, firing(), 1);
    CHECK(world.hogs()[0].jetpackSecondsLeft == 0.0f);
    CHECK(world.hogs()[0].parachute);
    const auto landing = run(world, PlayerInput{}, 50 * 8);
    CHECK(std::any_of(landing.begin(), landing.end(),
                      [](const GameEvent& event) { return event.type == GameEventType::Explosion; }));
    CHECK(world.hogs()[0].health == Approx(kMaxHealth));
    CHECK(world.activeHog() == std::optional<std::size_t>(0));  // still its turn
    CHECK(world.phase() == MatchPhase::Playing);
}

TEST_CASE("Flying a jetpack into the ground is a crash landing", "[game][weapons][reach]") {
    // The wiki: without the parachute the landing hurts (20 points).
    const auto map = flatMap();
    auto hogs = shooterAndEnemy(map, WeaponId::Jetpack, 0, 4000);
    hogs[0].aimDegrees = -20.0f;  // diving
    World world(map, hogs);
    run(world, firing(), 1);
    run(world, PlayerInput{}, 50 * 2);
    const Hog& hog = world.hogs()[0];
    CHECK(hog.jetpackSecondsLeft == 0.0f);
    CHECK(hog.health < kMaxHealth);
    CHECK(hog.health >= kMaxHealth - 21.0f);
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
    run(world, firing(), 1);  // it leaves at 45 degrees, its lowest aim
    PlayerInput leftAndDown;
    leftAndDown.movement.turnLeft = true;
    leftAndDown.aimDown = true;
    run(world, leftAndDown, 35);  // about 60 degrees down: under the horizon
    PlayerInput left;
    left.movement.turnLeft = true;
    run(world, left, 15);  // 90 degrees left in all, at 90 degrees per second
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
