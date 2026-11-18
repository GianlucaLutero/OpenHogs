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

namespace {

// Team 0's sniper facing team 1's hog, which has one point of health left.
World sniperDuel(const MapCollision& map, TurnRules rules) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1500, 0, 1)};
    hogs[0].weapons = {WeaponId::SniperRifle};
    hogs[0].aimDegrees = 0.0f;
    hogs[1].health = 1.0f;
    return World(map, hogs, rules);
}

// Fires the active hog's weapon, then lets the shot and the turn play out
// until the next turn starts.
std::vector<GameEvent> shootAndWait(World& world) {
    static const auto animations = makeAnimations();
    PlayerInput fire;
    fire.fire = true;
    auto all = world.update(kStep, fire, animations);
    for (int i = 0; i < 1000 && !has(all, GameEventType::TurnStarted) && !has(all, GameEventType::MatchOver); ++i) {
        const auto events = world.update(kStep, PlayerInput{}, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

} // namespace

TEST_CASE("In a deathmatch killed hogs come back and kills score points", "[game][multiplayer]") {
    // DEATHMATCH LIMIT, as the manual says: killed pigs re-spawn until a
    // team has enough points, 2 for a kill and -1 for a loss. A hog that
    // stayed dead, or points counted wrongly, would end the match early.
    const auto map = flatMap();
    TurnRules rules;
    rules.deathmatchLimit = 3;
    World world = sniperDuel(map, rules);
    const glm::vec3 start = world.hogs()[1].position;

    const auto events = shootAndWait(world);
    REQUIRE(has(events, GameEventType::Died));
    CHECK(has(events, GameEventType::Respawned));
    CHECK(world.phase() == MatchPhase::Playing);  // 2 points: not there yet
    const Hog& back = world.hogs()[1];
    CHECK(back.alive);
    CHECK(back.health == back.maxHealth);
    CHECK(glm::length(back.position - start) < 1.0f);
    CHECK(world.deathmatchPoints(0) == 2);
    CHECK(world.deathmatchPoints(1) == -1);
}

TEST_CASE("A deathmatch is won by the first team to the limit", "[game][multiplayer]") {
    // With a limit of 2 one kill is enough; without a limit the same kill
    // ends the usual match because the other team is wiped out.
    const auto map = flatMap();
    TurnRules rules;
    rules.deathmatchLimit = 2;
    World deathmatch = sniperDuel(map, rules);
    CHECK(has(shootAndWait(deathmatch), GameEventType::MatchOver));
    CHECK(deathmatch.winner() == 0);
    CHECK(deathmatch.hogs()[1].alive);  // it came back before the end was counted

    World survival = sniperDuel(map, TurnRules{});
    CHECK(has(shootAndWait(survival), GameEventType::MatchOver));
    CHECK_FALSE(survival.hogs()[1].alive);
}

TEST_CASE("SELECT PIG gives ten seconds to choose the hog before the clock runs", "[game][multiplayer]") {
    // The manual: with SELECT PIG the player chooses the hog at the start
    // of each turn and has only 10 seconds to do it. The turn's own time
    // must not run meanwhile, and the choice must close afterwards.
    std::vector<Hog> hogs;
    const auto map = flatMap();
    for (int i = 0; i < 2; ++i) hogs.push_back(makeHog(map, static_cast<float>(i * 300), 0, 0));
    hogs.push_back(makeHog(map, 2000, 0, 1));
    TurnRules rules;
    rules.selectPig = true;
    TurnSystem turns(hogs, rules);

    turns.update(9.0, 0.0f, hogs);
    CHECK(turns.secondsLeft() == rules.turnSeconds);
    CHECK(turns.switchHog(1, hogs));
    turns.update(2.0, 0.0f, hogs);  // the ten seconds are over
    CHECK_FALSE(turns.canSwitchHog());
    turns.update(1.0, 0.0f, hogs);
    CHECK(turns.secondsLeft() == Approx(rules.turnSeconds - 1.0));

    // Without SELECT PIG the clock runs at once (and the hog can be
    // changed until it moves, as before).
    TurnSystem usual(hogs, TurnRules{});
    usual.update(9.0, 0.0f, hogs);
    CHECK(usual.secondsLeft() == Approx(TurnRules{}.turnSeconds - 9.0));
    CHECK(usual.canSwitchHog());
}
