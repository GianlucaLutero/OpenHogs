// Tests for medals and awards (block mission-scripts, part 4): the
// campaign's medals on the results page and the special award of a
// skirmish. They protect the scores as the original counts them: the
// player's promotion points come from these medals, and the battle results
// decide who WINS!! a match between friends.

#include "GameTestSupport.h"

#include "openhogs/game/BattleAwards.h"
#include "openhogs/game/Campaign.h"
#include "openhogs/game/World.h"

#include <glm/gtc/constants.hpp>

using namespace openhogs;
using namespace openhogs::tests;

namespace {

// Hogs of teams 0 and 1 with the given deeds.
Hog hogOf(int team, int mostKills = 0, float damage = 0.0f, int kills = 0, int friendsKilled = 0) {
    Hog hog;
    hog.team = team;
    hog.record.mostKillsInATurn = mostKills;
    hog.record.enemyDamage = damage;
    hog.record.enemiesKilled = kills;
    hog.record.friendsKilled = friendsKilled;
    return hog;
}

std::optional<SpecialAwardKind> awardFor(std::vector<Hog> hogs) {
    const auto results = battleResults(hogs, {0, 1}, std::nullopt);
    if (!results.award) return std::nullopt;
    return results.award->kind;
}

const std::vector<std::string> kNames{"NOBBY", "GINGER", "DEN", "MONTY", "BASIL", "PONSONBY", "PERCY", "SMITH", "JONES"};

MissionReport won(std::vector<std::size_t> deaths = {}, int points = 0) {
    MissionReport report;
    report.won = true;
    report.deathOrder = std::move(deaths);
    report.pickedUpPoints = points;
    return report;
}

} // namespace

TEST_CASE("The special award follows the executable's order", "[game][awards]") {
    // Several kills in one turn beat damage, except that 351 damage beats a
    // double kill; with nothing better, damage gives the sadism award or
    // the pig of the battle; nobody gets anything without hurting an enemy.
    CHECK(awardFor({hogOf(0, 4), hogOf(1, 3, 500.0f)}) == SpecialAwardKind::Annihilator);
    CHECK(awardFor({hogOf(0, 1), hogOf(1, 3)}) == SpecialAwardKind::TripleRipReaper);
    CHECK(awardFor({hogOf(0, 2), hogOf(1, 0, 351.0f)}) == SpecialAwardKind::MasterSadist);
    CHECK(awardFor({hogOf(0, 2, 350.0f), hogOf(1)}) == SpecialAwardKind::DoubleDeathDealer);
    CHECK(awardFor({hogOf(0, 1, 176.0f), hogOf(1, 0, 20.0f)}) == SpecialAwardKind::SadismAward);
    CHECK(awardFor({hogOf(0, 1, 175.9f)}) == SpecialAwardKind::PigOfTheBattle);
    CHECK_FALSE(awardFor({hogOf(0), hogOf(1)}).has_value());
}

TEST_CASE("Battle scores add kills, victory and the award; a shared best score wins nothing", "[game][awards]") {
    // Team 0 killed three and lost one of its own, won the match (+3) and
    // its hog took the triple award (+3): 3 - 1 + 3 + 3 = 8.
    std::vector<Hog> hogs{hogOf(0, 3, 0.0f, 3, 1), hogOf(1, 0, 50.0f, 1)};
    auto results = battleResults(hogs, {0, 1}, 0);
    REQUIRE(results.teams.size() == 2);
    CHECK(results.teams[0].total() == 8);
    CHECK(results.teams[1].total() == 1);
    REQUIRE(results.award.has_value());
    CHECK(results.award->hog == 0);
    CHECK(results.bestTeam == 0);

    // Level scores: nobody WINS!!.
    std::vector<Hog> level{hogOf(0, 0, 0.0f, 2), hogOf(1, 0, 0.0f, 2)};
    CHECK_FALSE(battleResults(level, {0, 1}, std::nullopt).bestTeam.has_value());
}

TEST_CASE("A hog is credited with what happens in its turn", "[game][awards]") {
    // The rifle shot that kills the enemy counts as a kill and as damage
    // dealt by the shooter, and the turn's kills are kept when the next
    // turn starts.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 400, 0, 1), makeHog(map, 400, 600, 1)};
    hogs[1].yawRadians = glm::pi<float>();
    hogs[0].weapons = {WeaponId::Rifle};
    hogs[0].aimDegrees = 0.0f;
    hogs[1].health = 10.0f;
    World world(map, hogs);
    PlayerInput fire;
    fire.fire = true;
    static const auto animations = makeAnimations();
    world.update(kStep, fire, animations);
    for (int i = 0; i < 50 * 8; ++i) world.update(kStep, PlayerInput{}, animations);
    const HogRecord& record = world.hogs()[0].record;
    CHECK(record.enemiesKilled == 1);
    CHECK(record.enemyDamage == Approx(10.0f));
    CHECK(record.mostKillsInATurn == 1);
    CHECK(record.killsThisTurn == 0);
    CHECK(world.hogs()[1].record.enemiesKilled == 0);
}

TEST_CASE("Campaign medals follow the executable", "[game][campaign][awards]") {
    // The island bonus comes after every fifth mission (not after 24);
    // each mission offers the special medals of the executable's table;
    // the last mission forgives two deaths for the survival medal and
    // mission 15 starts with one special medal.
    std::vector<int> bonus;
    for (int number = 1; number <= kMissionCount; ++number) {
        if (earnsIslandBonus(number)) bonus.push_back(number);
    }
    CHECK(bonus == std::vector<int>{5, 10, 15, 20, 25});
    CHECK(specialMedals(8) == 0);
    CHECK(specialMedals(16) == 2);
    CHECK(specialMedals(24) == 5);

    Campaign campaign(0, kNames);
    for (int i = 1; i < 15; ++i) campaign.recordMission(won({0}), 5);
    REQUIRE(campaign.nextMission() == 15);
    auto outcome = campaign.recordMission(won({0}, 0), 5);
    CHECK(outcome.specialMedalsWon == 1);
    CHECK(outcome.specialMedalsOffered == 1);
    CHECK(outcome.pointsEarned == 1 + 1 + 5);  // level, the free special medal, island

    for (int i = 16; i < kMissionCount; ++i) campaign.recordMission(won(), 5);
    REQUIRE(campaign.nextMission() == kMissionCount);
    outcome = campaign.recordMission(won({0, 1}), 5);
    CHECK(outcome.survivalBonus);
}
