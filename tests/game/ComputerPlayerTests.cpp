// Tests for game::ComputerPlayer and game::planShot: the computer opponent.
//
// The computer must make sensible shots (the right weapon, aimed at an
// enemy, not at its own team), get closer when it cannot reach anyone, and
// always finish its turn: a computer that hesitates forever or blows up its
// own hogs makes the game unplayable against it.

#include "openhogs/game/ComputerPlayer.h"

#include "GameTestSupport.h"

#include <glm/gtc/constants.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::game;
using namespace openhogs::tests;

namespace {

// Two hogs of different teams `distance` apart along X on flat ground,
// both carrying `weapons`. Hog 0 (team 0) plays first.
std::vector<Hog> standoff(const MapCollision& map, float distance, std::vector<WeaponId> weapons) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, distance, 0, 1)};
    for (auto& hog : hogs) hog.weapons = weapons;
    hogs[0].yawRadians = glm::half_pi<float>();  // facing away (-Z): the computer must turn
    return hogs;
}

// An opponent that makes no mistakes, so outcomes are certain.
AiSettings perfect() {
    AiSettings settings;
    settings.aimErrorDegrees = 0.0f;
    settings.powerError = 0.0f;
    settings.thinkSeconds = 0.1;
    return settings;
}

// Lets the computer play until its shot is fired (or the turn ends) and
// the world has settled; returns everything that happened.
std::vector<GameEvent> playComputerTurn(World& world, ComputerPlayer& computer, int maxSteps = 3000) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    const int firstTurn = world.turns()->turnNumber();
    for (int i = 0; i < maxSteps && world.turns()->turnNumber() == firstTurn && world.phase() != MatchPhase::Over;
         ++i) {
        const auto decision = computer.update(world, kStep);
        if (decision.endTurn) world.endTurn();
        const auto events = world.update(kStep, decision.input, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

bool wasHurt(const std::vector<GameEvent>& events, std::size_t hog) {
    for (const auto& event : events) {
        if ((event.type == GameEventType::Hurt || event.type == GameEventType::Died) && event.hog == hog) return true;
    }
    return false;
}

} // namespace

TEST_CASE("yawTowards gives the yaw that faces a direction", "[game][ai]") {
    // Every plan turns the hog with this; a sign error would make the
    // computer shoot away from its target.
    for (const glm::vec3 direction : {glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(-1, 0, 1)}) {
        Hog hog;
        hog.yawRadians = yawTowards(direction);
        checkClose(hogFacing(hog), glm::normalize(direction));
    }
}

TEST_CASE("Next to an enemy the computer hits it with a melee weapon", "[game][ai]") {
    // With only fists, the only sensible plan is a punch at the hog in reach.
    const auto map = flatMap();
    World world(map, standoff(map, 80, {WeaponId::Trotter}));
    const auto plan = planShot(world, 0);
    REQUIRE(plan);
    CHECK(plan->weapon == WeaponId::Trotter);
    CHECK(plan->target == 1);
    Hog turned = world.hogs()[0];
    turned.yawRadians = plan->yawRadians;
    checkClose(hogFacing(turned), glm::vec3(1, 0, 0));
}

TEST_CASE("With a clear line of fire the computer shoots its rifle", "[game][ai]") {
    // A rifle shot at a visible enemy beats a punch it cannot reach.
    const auto map = flatMap();
    World world(map, standoff(map, 900, {WeaponId::Trotter, WeaponId::Rifle}));
    const auto plan = planShot(world, 0);
    REQUIRE(plan);
    CHECK(plan->weapon == WeaponId::Rifle);
    CHECK(plan->target == 1);
}

TEST_CASE("The computer does not shoot bullets through walls", "[game][ai]") {
    // A wall between the hogs stops bullets; planning a rifle shot through
    // it would waste the turn.
    const auto map = flatMap({makeBox({450, 200, 0}, {40, 400, 400})});
    World world(map, standoff(map, 900, {WeaponId::Rifle}));
    const auto plan = planShot(world, 0);
    CHECK_FALSE(plan);
}

TEST_CASE("A planned throw lands on the enemy", "[game][ai]") {
    // The throw search uses the real flight rules: playing the plan out
    // must actually hurt the target.
    const auto map = flatMap();
    World world(map, standoff(map, 1000, {WeaponId::Grenade}));
    ComputerPlayer computer(perfect());
    const auto events = playComputerTurn(world, computer);
    CHECK(wasHurt(events, 1));
    CHECK_FALSE(wasHurt(events, 0));
}

TEST_CASE("The computer will not throw at an enemy standing next to its own hog", "[game][ai]") {
    // The friendly hog would take as much damage as the enemy: a grenade
    // there is a bad trade.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 1), makeHog(map, 1060, 0, 0)};
    for (auto& hog : hogs) hog.weapons = {WeaponId::Grenade};
    World world(map, hogs);
    const auto plan = planShot(world, 0);
    CHECK((!plan || plan->target != 1 || plan->score <= 0.0f));
}

TEST_CASE("A whole computer turn with a rifle hits the enemy", "[game][ai]") {
    // From thinking to turning, choosing the weapon, aiming and firing,
    // through the same inputs a player uses.
    const auto map = flatMap();
    World world(map, standoff(map, 900, {WeaponId::Trotter, WeaponId::Rifle}));
    ComputerPlayer computer(perfect());
    const auto events = playComputerTurn(world, computer);
    CHECK(wasHurt(events, 1));
    CHECK(world.turns()->activeTeam() == 1);  // the turn passed to the other team
}

TEST_CASE("Out of reach, the computer runs towards the enemy", "[game][ai]") {
    // With only fists and the enemy far away, standing still would be
    // pointless: it must use its movement to get closer, then end its turn.
    const auto map = flatMap();
    World world(map, standoff(map, 3000, {WeaponId::Trotter}));
    ComputerPlayer computer(perfect());
    const float before = glm::length(world.hogs()[1].position - world.hogs()[0].position);
    playComputerTurn(world, computer);
    const float after = glm::length(world.hogs()[1].position - world.hogs()[0].position);
    CHECK(after < before - 1000.0f);
    CHECK(world.turns()->activeTeam() == 1);
}

TEST_CASE("With nothing useful to do, the computer ends its turn", "[game][ai]") {
    // A hog with only explosives to put down and no movement left must not
    // hold up the match until the clock runs out.
    TurnRules rules;
    rules.movementAllowance = 0.0f;
    const auto map = flatMap();
    World world(map, standoff(map, 3000, {WeaponId::Tnt}), rules);
    ComputerPlayer computer(perfect());
    const int steps = static_cast<int>(5.0 / kStep);
    playComputerTurn(world, computer, steps);
    CHECK(world.turns()->activeTeam() == 1);
}
