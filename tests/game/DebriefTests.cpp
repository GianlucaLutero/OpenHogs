// Tests for game::debriefRows and game::rankBadges: the debriefing page
// after a campaign mission.
//
// The page is the player's only picture of what the mission cost: a hog
// shown with a gravestone must be the one the campaign really replaced,
// and the badges must say the rank the hog really has.

#include "openhogs/game/Debrief.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::game;

namespace {

std::vector<SquadMember> squadOf(std::size_t size) {
    std::vector<SquadMember> squad;
    for (std::size_t i = 0; i < size; ++i) squad.push_back({"Hog" + std::to_string(i + 1), RankId::Grunt});
    return squad;
}

} // namespace

TEST_CASE("The first hogs to fall beyond two are shown lost for good", "[game][debrief]") {
    // As Campaign::recordMission: of four deaths, the first two cost the
    // hogs for good (gravestones), the last two only hurt (bandages).
    // Hogs that survived, or stayed behind, look fine.
    MissionReport report;
    report.won = true;
    report.deathOrder = {3, 0, 1, 2};
    const auto rows = debriefRows(squadOf(8), report, 5, true);
    REQUIRE(rows.size() == kDebriefRows);
    CHECK(rows[3].face == DebriefFace::Lost);
    CHECK(rows[0].face == DebriefFace::Lost);
    CHECK(rows[1].face == DebriefFace::Hurt);
    CHECK(rows[2].face == DebriefFace::Hurt);
    CHECK(rows[4].face == DebriefFace::Fine);
    CHECK(rows[3].name == "Hog4");
}

TEST_CASE("A failed mission loses nobody for good", "[game][debrief]") {
    // A failed mission is played again with the same squad, so nobody may
    // be shown under a gravestone, however many fell.
    MissionReport report;
    report.deathOrder = {0, 1, 2, 3};
    const auto rows = debriefRows(squadOf(4), report, 4, false);
    REQUIRE(rows.size() == 4);
    for (const auto& row : rows) CHECK(row.face == DebriefFace::Hurt);
}

TEST_CASE("Rank badges show the career and the rank within it", "[game][debrief]") {
    // A grunt has no badge; the first rank of a career shows its badge
    // alone, the next ones add one or two pips; the officers have their own.
    CHECK(rankBadges(RankId::Grunt).empty());
    CHECK(rankBadges(RankId::Gunner) == std::vector<std::string>{"pcHeavy.bmp"});
    CHECK(rankBadges(RankId::Medic) == std::vector<std::string>{"pcMedic.bmp", "pcPip1.bmp"});
    CHECK(rankBadges(RankId::Spy) == std::vector<std::string>{"pcSniper.bmp", "pcPip2.bmp"});
    CHECK(rankBadges(RankId::Saboteur) == std::vector<std::string>{"pcEngine.bmp", "pcPip2.bmp"});
    CHECK(rankBadges(RankId::Commando) == std::vector<std::string>{"pcCmmndo.bmp"});
    CHECK(rankBadges(RankId::Legend) == std::vector<std::string>{"pcLegend.bmp"});
}
