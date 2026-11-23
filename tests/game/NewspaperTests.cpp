// Tests for game::newspaperFor: the front page printed after a campaign
// victory.
//
// The papers' headlines are baked into the bitmaps, so a wrong choice
// prints "OUR LADS ROGERED" over a flawless victory, or the wrong nation's
// masthead; nothing else in the game would show the mistake.

#include "openhogs/game/Newspaper.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::game;

namespace {

NewspaperPage pageFor(int missionNumber, int hogsLost, int medals, int nation = 0) {
    return newspaperFor({nation, missionNumber, hogsLost, medals});
}

} // namespace

TEST_CASE("The headline follows the hogs lost and the medals won", "[game][newspaper]") {
    // The five photos and their story groups are the original's verdicts
    // on the battle; mission 2 (ROAD) is an ordinary mission, story variant 2.
    CHECK(pageFor(2, 0, 2).picture == "Language/Tims/Papers/pic01.bmp");
    CHECK(pageFor(2, 0, 2).story == "Language/Tims/Papers/text3.bmp");
    CHECK(pageFor(2, 0, 1).picture == "Language/Tims/Papers/pic02.bmp");
    CHECK(pageFor(2, 0, 1).story == "Language/Tims/Papers/text7.bmp");
    CHECK(pageFor(2, 2, 5).picture == "Language/Tims/Papers/pic03.bmp");
    CHECK(pageFor(2, 2, 5).story == "Language/Tims/Papers/text11.bmp");
    CHECK(pageFor(2, 3, 0).picture == "Language/Tims/Papers/pic04.bmp");
    CHECK(pageFor(2, 3, 0).story == "Language/Tims/Papers/text15.bmp");
    CHECK(pageFor(2, 4, 0).picture == "Language/Tims/Papers/pic05.bmp");
    CHECK(pageFor(2, 4, 0).story == "Language/Tims/Papers/text18.bmp");
}

TEST_CASE("Consecutive missions print different stories", "[game][newspaper]") {
    // The mission number picks one of four stories, so the same kind of
    // victory twice in a row does not repeat the words.
    CHECK(pageFor(2, 0, 0).story != pageFor(3, 0, 0).story);
    CHECK(pageFor(3, 1, 0).story != pageFor(4, 1, 0).story);
}

TEST_CASE("Island ends get their own story unless the squad was wiped out", "[game][newspaper]") {
    // Mission 5 (RUMBLE, level 2) ends the first island: "HOGSHEAD FALLS!";
    // the final mission prints the last of these stories. The photo still
    // judges the battle. Losing all five counted hogs drops the special.
    CHECK(pageFor(5, 0, 0).story == "Language/Tims/Papers/text21.bmp");
    CHECK(pageFor(5, 3, 0).picture == "Language/Tims/Papers/pic04.bmp");
    CHECK(pageFor(25, 1, 0).story == "Language/Tims/Papers/text26.bmp");
    CHECK(pageFor(5, 5, 0).story == "Language/Tims/Papers/text17.bmp");
}

TEST_CASE("Each nation reads its own paper", "[game][newspaper]") {
    // The nations follow scene::characterTeams, where Japan comes before
    // Russia; the paper files do not follow that order on disk.
    CHECK(pageFor(2, 0, 0, 0).paper == "Language/Tims/Papers/British.bmp");
    CHECK(pageFor(2, 0, 0, 4).paper == "Language/Tims/Papers/Japan.bmp");
    CHECK(pageFor(2, 0, 0, 5).paper == "Language/Tims/Papers/Russian.bmp");
}
