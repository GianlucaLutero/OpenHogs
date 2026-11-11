// Tests for game::Levels and game::Cutscenes: the level table and the
// videos' schedule taken from the executable.
//
// A level number off by one gives every later level the wrong map, name
// and sky (it happened: the first campaign table missed level 20, KEEP);
// a wrong schedule plays the island videos at the wrong moments.

#include "openhogs/game/Cutscenes.h"
#include "openhogs/game/Levels.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace openhogs::game;

TEST_CASE("Levels keep the executable's numbering", "[game][levels]") {
    // Spot checks against the pointer array and the gtext names: level n's
    // name is gtext entry 11 + n.
    REQUIRE(allLevels().size() == 53);
    CHECK(level(0).map == "ROAD");
    CHECK(level(10).map == "CAMP");
    CHECK(level(20).map == "KEEP");
    CHECK(level(25).map == "ESTU");
    CHECK(level(25).nameEntry == 36);
    CHECK(level(52).map == "ONEWAY");
    std::set<std::string_view> maps;
    for (const auto& info : allLevels()) maps.insert(info.map);
    CHECK(maps.size() == allLevels().size());
    CHECK_THROWS_AS(level(53), std::out_of_range);
}

TEST_CASE("Skies match the levels' themes", "[game][levels]") {
    // The sky field decides the sky dome, the weather and the night sounds;
    // these levels' themes are obvious from their names.
    CHECK(findLevel("fjords")->sky == SkyType::Cold);   // GLACIER GUNS
    CHECK(findLevel("LUNAR1")->sky == SkyType::Space);  // PIGS IN SPACE
    CHECK(findLevel("BUTE")->sky == SkyType::Space);    // MOON BUTTES
    CHECK(findLevel("PLAY1")->sky == SkyType::Toy);     // PLAY PEN
    CHECK(findLevel("CREEPY2")->sky == SkyType::Night1);// GRAVEYARD SHIFT
    CHECK(findLevel("DESVAL")->sky == SkyType::Desert); // I SPY... (desert valley)
    CHECK_FALSE(findLevel("GENSNOW"));
    CHECK(isNightSky(SkyType::Sunset));
    CHECK_FALSE(isNightSky(SkyType::Sunny));
}

TEST_CASE("Videos follow the original's schedule", "[game][cutscenes]") {
    // Intro at start-up; one video per conquered island; two at the end.
    CHECK(introCutscenes().size() == 4);
    CHECK(campaignCutscenes(2).empty());
    CHECK(campaignCutscenes(6) == std::vector<std::string>{"Language/Streams/fmv 03.bik"});
    CHECK(campaignCutscenes(21) == std::vector<std::string>{"Language/Streams/fmv 06.bik"});
    CHECK(campaignCutscenes(25) == std::vector<std::string>{"Language/Streams/fmv 07.bik"});
    CHECK(campaignCutscenes(26).size() == 2);
    CHECK(campaignCutscenes(7).empty());
}
