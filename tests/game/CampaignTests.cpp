// Tests for game::Campaign and game::Ranks: the missions, the squad, points,
// promotions, losses and saving.
//
// The campaign is the long game: a wrong rule (points, losses, the mission
// order) or a save file that does not load back spoils hours of play, and
// may only show up many missions later.

#include "openhogs/game/Campaign.h"
#include "openhogs/game/Ranks.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

#include "GameTestSupport.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace openhogs::game;
using namespace openhogs::tests;
using openhogs::core::IniFile;

namespace {

const std::vector<std::string> kNames{"NOBBY", "GINGER", "DEN", "MONTY", "BASIL", "PONSONBY", "PERCY", "SMITH", "JONES"};

Campaign britishCampaign() {
    return Campaign(0, kNames);
}

MissionReport won(std::vector<std::size_t> deaths = {}) {
    return {true, std::move(deaths)};
}

} // namespace

TEST_CASE("The 25 missions follow the game's order, one map each", "[game][campaign]") {
    // Taken from the executable's tables: mission 1 is The War Foundation
    // on ESTU (the only mission with three hogs), the last Well, Well, Well!
    // on FINAL (six enemies); every mission has its own map and islands go
    // in order.
    const auto missions = campaignMissions();
    REQUIRE(missions.size() == 25);
    CHECK(campaignMission(1).map == "ESTU");
    CHECK(campaignMission(1).nameEntry == 36);  // THE WAR FOUNDATION
    CHECK(campaignMission(2).map == "ROAD");
    CHECK(campaignMission(22).map == "KEEP");   // ASSASSINATION
    CHECK(campaignMission(22).nameEntry == 31);
    CHECK(campaignMission(25).map == "FINAL");
    CHECK(campaignMission(25).nameEntry == 35);  // WELL, WELL, WELL!
    CHECK(campaignMission(25).island == 5);
    std::set<std::string_view> maps;
    int island = 0;
    for (const auto& mission : missions) {
        maps.insert(mission.map);
        CHECK(mission.island >= island);
        island = mission.island;
    }
    CHECK(maps.size() == 25);
    CHECK_THROWS_AS(campaignMission(26), std::out_of_range);
}

TEST_CASE("Islands are conquered by missions 5, 10, 15, 20, 24 and 25", "[game][campaign]") {
    // These missions are worth five extra points each.
    std::vector<int> conquering;
    for (int number = 1; number <= kMissionCount; ++number) {
        if (conquersIsland(number)) conquering.push_back(number);
    }
    CHECK(conquering == std::vector<int>{5, 10, 15, 20, 24, 25});
}

TEST_CASE("A new campaign has eight grunts named after the nation's hogs", "[game][campaign]") {
    // The squad of the manual: eight grunts, no points, mission 1 next.
    const auto campaign = britishCampaign();
    REQUIRE(campaign.squad().size() == kSquadSize);
    CHECK(campaign.squad()[0].name == "NOBBY");
    CHECK(campaign.squad()[7].name == "SMITH");
    for (const auto& member : campaign.squad()) CHECK(member.rank == RankId::Grunt);
    CHECK(campaign.points() == 0);
    CHECK(campaign.nextMission() == 1);
    CHECK(campaign.deploymentSize(3) == 3);  // mission 1's map has three places
    CHECK(campaign.deploymentSize(9) == kDeployedHogs);
    CHECK_THROWS(Campaign(kTeamLardNation, kNames));  // Team Lard is not playable
}

TEST_CASE("Finishing a mission earns a point, one more if nobody died", "[game][campaign]") {
    // The manual's rule; points are what promotions are bought with.
    auto campaign = britishCampaign();
    auto outcome = campaign.recordMission(won(), 3);
    CHECK(outcome.completed);
    CHECK(outcome.survivalBonus);
    CHECK(outcome.pointsEarned == 2);
    CHECK(campaign.nextMission() == 2);

    outcome = campaign.recordMission(won({1}), 5);
    CHECK_FALSE(outcome.survivalBonus);
    CHECK(outcome.pointsEarned == 1);
    CHECK(campaign.points() == 3);
}

TEST_CASE("The last mission of an island is worth five more points", "[game][campaign]") {
    // Conquering Hogshead (mission 5) adds the island bonus.
    auto campaign = britishCampaign();
    for (int i = 0; i < 4; ++i) campaign.recordMission(won({0}), 5);
    REQUIRE(campaign.nextMission() == 5);
    const auto outcome = campaign.recordMission(won(), 5);
    CHECK(outcome.islandConquered);
    CHECK(outcome.pointsEarned == 1 + 1 + 5);
}

TEST_CASE("Two deaths are forgiven; the third costs the first to die", "[game][campaign]") {
    // Losing three hogs loses the first one for good, four lose the first
    // two; they are replaced by new grunts with unused names.
    auto campaign = britishCampaign();
    const auto outcome = campaign.recordMission(won({1, 2}), 5);
    CHECK(outcome.lostForGood.empty());
    CHECK(campaign.squad()[1].name == "GINGER");
    CHECK(campaign.squad()[2].name == "DEN");
}

TEST_CASE("Hogs lost for good are replaced by new grunts", "[game][campaign]") {
    // A fallen hog's name is never given to a recruit: it would look as if
    // the hog came back without its rank.
    auto campaign = britishCampaign();
    campaign.recordMission(won(), 3);  // 2 points
    REQUIRE(campaign.promote(3, RankId::Gunner));
    const auto outcome = campaign.recordMission(won({3, 0, 4, 2}), 5);
    CHECK(outcome.completed);
    CHECK(outcome.lostForGood == std::vector<std::string>{"MONTY", "NOBBY"});
    CHECK(campaign.squad()[3].name == "JONES");  // the nation's ninth name
    CHECK(campaign.squad()[3].rank == RankId::Grunt);
    CHECK(campaign.squad()[0].name == "PIG 1");  // the names have run out
    CHECK(campaign.squad()[4].name == "BASIL");  // the third and fourth deaths are forgiven
}

TEST_CASE("Losing every hog or the battle fails the mission without changes", "[game][campaign]") {
    // A failed mission is played again: no points, no losses, same mission.
    auto campaign = britishCampaign();
    auto outcome = campaign.recordMission(won({0, 1, 2}), 3);
    CHECK_FALSE(outcome.completed);
    outcome = campaign.recordMission({false, {}}, 3);
    CHECK_FALSE(outcome.completed);
    CHECK(campaign.nextMission() == 1);
    CHECK(campaign.points() == 0);
    CHECK(campaign.squad()[0].name == "NOBBY");
}

TEST_CASE("Promotions follow the career tree and cost points", "[game][campaign]") {
    // A grunt picks one of four careers; then each career climbs to
    // Commando and Hero. Higher ranks cost more, and nobody can skip one.
    CHECK(promotionsFrom(RankId::Grunt).size() == 4);
    CHECK(promotionsFrom(RankId::Gunner) == std::vector<RankId>{RankId::Bombardier});
    CHECK(promotionsFrom(RankId::Saboteur) == std::vector<RankId>{RankId::Commando});
    CHECK(promotionsFrom(RankId::Commando) == std::vector<RankId>{RankId::Hero});
    CHECK(promotionsFrom(RankId::Hero).empty());
    CHECK(promotionCost(RankId::Scout) == 1);
    CHECK(promotionCost(RankId::Hero) == 5);

    auto campaign = britishCampaign();
    CHECK_FALSE(campaign.promote(0, RankId::Gunner));  // no points yet
    campaign.recordMission(won(), 3);                  // 2 points
    CHECK_FALSE(campaign.promote(0, RankId::Bombardier));  // not from Grunt
    CHECK(campaign.promote(0, RankId::Gunner));
    CHECK(campaign.points() == 1);
    CHECK_FALSE(campaign.promote(0, RankId::Bombardier));  // costs 2
    CHECK(campaign.squad()[0].rank == RankId::Gunner);
}

TEST_CASE("Every rank has a body model and weapons", "[game][campaign]") {
    // A rank without a model or weapons would leave a hog invisible or
    // unarmed in its mission.
    std::set<std::string_view> keys;
    for (const auto& rank : allRanks()) {
        CHECK(openhogs::scene::findCharacterClass(rank.classId) != nullptr);
        CHECK_FALSE(rank.weapons.empty());
        CHECK(rank.maxHealth > 0.0f);
        CHECK(findRank(rank.key) == rank.id);
        keys.insert(rank.key);
    }
    CHECK(keys.size() == allRanks().size());
}

TEST_CASE("Each island pits the player against every other nation", "[game][campaign]") {
    // Five missions, five enemies; never the player's own nation; the last
    // mission is against Team Lard.
    const Campaign campaign(2, kNames);  // French
    std::set<int> enemies;
    for (int number = 1; number <= 5; ++number) {
        const int enemy = campaign.enemyNation(number, 0);
        CHECK(enemy != 2);
        enemies.insert(enemy);
    }
    CHECK(enemies.size() == 5);
    CHECK(campaign.enemyNation(25, 0) == kTeamLardNation);
    CHECK(campaign.enemyNation(24, 1) != campaign.enemyNation(24, 0));  // several enemy teams differ
}

TEST_CASE("A campaign saved and loaded is the same campaign", "[game][campaign]") {
    // Progress lives in the save file between sessions.
    auto campaign = britishCampaign();
    campaign.recordMission(won(), 3);
    campaign.promote(2, RankId::Sapper);
    campaign.swap(0, 7);

    const auto loaded = loadCampaign(IniFile::parse(campaign.save()));
    CHECK(loaded.nation() == 0);
    CHECK(loaded.nextMission() == 2);
    CHECK(loaded.points() == campaign.points());
    REQUIRE(loaded.squad().size() == kSquadSize);
    for (std::size_t i = 0; i < kSquadSize; ++i) {
        CHECK(loaded.squad()[i].name == campaign.squad()[i].name);
        CHECK(loaded.squad()[i].rank == campaign.squad()[i].rank);
    }
}

TEST_CASE("Broken save files are rejected", "[game][campaign]") {
    // A damaged or hand-edited file must give a clear error, not a squad of
    // nameless hogs or an impossible mission.
    CHECK_THROWS_AS(loadCampaign(IniFile::parse("[Other]\nA = 1\n")), openhogs::core::FormatError);
    auto text = britishCampaign().save();
    const auto broken = text.replace(text.find("grunt"), 5, "admiral");
    CHECK_THROWS_AS(loadCampaign(IniFile::parse(broken)), openhogs::core::FormatError);
    CHECK_THROWS_AS(loadCampaign(IniFile::parse("[Campaign]\nNation = 9\n")), openhogs::core::FormatError);
}

TEST_CASE("The squad takes the places of the map's player hogs", "[game][campaign]") {
    // The first hogs of the squad replace the map's hogs marked for the
    // player, with their ranks' health and weapons; extra places are
    // removed; the enemy is untouched.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 100, 0, 0), makeHog(map, 200, 0, 0),
                          makeHog(map, 900, 0, 1)};
    for (std::size_t i = 0; i < 3; ++i) hogs[i].playerControlled = true;
    CHECK(playerSlots(hogs) == 3);

    auto campaign = britishCampaign();
    campaign.recordMission(won(), 3);
    campaign.promote(1, RankId::Gunner);
    const int team = deploySquad(hogs, campaign, 2);

    CHECK(team == 0);
    REQUIRE(hogs.size() == 3);  // one player place removed
    CHECK(hogs[0].name == "NOBBY");
    CHECK(hogs[0].maxHealth == rankSpec(RankId::Grunt).maxHealth);
    CHECK(hogs[0].health == hogs[0].maxHealth);
    CHECK(hogs[1].rank == RankId::Gunner);
    CHECK(hogs[1].weapons == rankSpec(RankId::Gunner).weapons);
    CHECK(hogs[1].squadIndex == 1u);
    CHECK(hogs[2].team == 1);
    CHECK_FALSE(hogs[2].squadIndex);
}
