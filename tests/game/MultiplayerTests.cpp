// Tests for the multiplayer match settings: game::applyFieldConditions,
// sudden death, and turns ended from outside the world.
//
// These settings change who and what is on the battlefield; a team that
// should be off but still plays, or a sudden death that never comes, makes
// the chosen settings meaningless.

#include "openhogs/game/FieldConditions.h"
#include "openhogs/game/World.h"

#include "GameTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::game;
using namespace openhogs::tests;

namespace {

std::vector<Hog> threeTeams(const MapCollision& map) {
    std::vector<Hog> hogs;
    for (int team = 0; team < 3; ++team) {
        for (int i = 0; i < 4; ++i) {
            hogs.push_back(makeHog(map, static_cast<float>(team * 1000), static_cast<float>(i * 200), team));
        }
    }
    return hogs;
}

int countTeam(const std::vector<Hog>& hogs, int team) {
    return static_cast<int>(std::count_if(hogs.begin(), hogs.end(), [team](const Hog& hog) { return hog.team == team; }));
}

std::vector<GameEvent> step(World& world, int steps = 1) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto events = world.update(kStep, PlayerInput{}, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

bool has(const std::vector<GameEvent>& events, GameEventType type) {
    return std::any_of(events.begin(), events.end(), [type](const GameEvent& event) { return event.type == type; });
}

} // namespace

TEST_CASE("Teams switched off leave the battlefield", "[game][multiplayer]") {
    // OFF on the PLAYERS page: that team's hogs are not in the match.
    const auto map = flatMap();
    auto hogs = threeTeams(map);
    FieldConditions conditions;
    conditions.removedTeams = {1};
    applyFieldConditions(hogs, conditions);
    CHECK(countTeam(hogs, 0) == 4);
    CHECK(countTeam(hogs, 1) == 0);
    CHECK(countTeam(hogs, 2) == 4);
}

TEST_CASE("Hogs per team keeps each team's first hogs", "[game][multiplayer]") {
    // PIGS on the FIELD CONDITIONS page; teams with fewer hogs keep them all.
    const auto map = flatMap();
    auto hogs = threeTeams(map);
    hogs.pop_back();  // team 2 has three
    FieldConditions conditions;
    conditions.hogsPerTeam = 3;
    applyFieldConditions(hogs, conditions);
    CHECK(countTeam(hogs, 0) == 3);
    CHECK(countTeam(hogs, 2) == 3);
    CHECK(hogs[0].position.z == 0.0f);  // the first ones stay
}

TEST_CASE("Starting health can be halved or doubled", "[game][multiplayer]") {
    // HEALTH on the FIELD CONDITIONS page changes full health, so the HUD's
    // bars still start full.
    const auto map = flatMap();
    auto hogs = threeTeams(map);
    FieldConditions conditions;
    conditions.healthFactor = 0.5f;
    applyFieldConditions(hogs, conditions);
    CHECK(hogs[0].maxHealth == kMaxHealth / 2.0f);
    CHECK(hogs[0].health == hogs[0].maxHealth);
}

TEST_CASE("Sudden death leaves every hog with one point of health", "[game][multiplayer]") {
    // When the chosen round begins, the next hit kills.
    const auto map = flatMap();
    TurnRules rules;
    rules.suddenDeathRound = 2;
    World world(map, {makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 1)}, rules);

    world.endTurn();  // team 1's first turn: still round 1
    auto events = step(world);
    CHECK_FALSE(has(events, GameEventType::SuddenDeath));
    CHECK(world.hogs()[0].health == kMaxHealth);

    world.endTurn();  // team 0 again: round 2
    events = step(world);
    CHECK(has(events, GameEventType::SuddenDeath));
    CHECK(world.hogs()[0].health == 1.0f);
    CHECK(world.hogs()[1].health == 1.0f);
}

TEST_CASE("A turn ended with Enter is announced like any other", "[game][multiplayer]") {
    // The "GET READY" message and sound come from the TurnStarted event;
    // ending a turn by key (or by the computer) must report it too.
    const auto map = flatMap();
    World world(map, {makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 1)});
    world.endTurn();
    CHECK(has(step(world), GameEventType::TurnStarted));
    CHECK_FALSE(has(step(world), GameEventType::TurnStarted));  // reported once
}
