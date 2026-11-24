// Tests for ui::CampaignMapAnimation: the campaign map shown before a
// mission (the executable's FUN_00482c30, FUN_00483010, FUN_004833b0).
//
// The map is the player's view of the campaign; if its steps came in the
// wrong order, too fast or could not be skipped, every mission would start
// with a broken or tedious screen.

#include "openhogs/ui/CampaignMap.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>

using namespace openhogs::ui;
using Catch::Matchers::WithinAbs;

namespace {

void run(CampaignMapAnimation& map, double seconds) {
    for (double t = 0.0; t < seconds - 1e-9; t += 0.01) map.update(0.01);
}

} // namespace

TEST_CASE("The campaign map shows the map, zooms into the island, then waits", "[ui][campaignmap]") {
    // Two seconds of map, 32 ticks of 50 ms of zoom, then up to ten
    // seconds on the island, as the executable times them.
    CampaignMapAnimation map(7);
    CHECK(map.phase() == CampaignMapPhase::Map);
    run(map, 1.95);
    CHECK(map.phase() == CampaignMapPhase::Map);
    run(map, 0.1);
    CHECK(map.phase() == CampaignMapPhase::Zoom);
    run(map, 1.45);
    CHECK(map.phase() == CampaignMapPhase::Zoom);
    run(map, 0.2);
    CHECK(map.phase() == CampaignMapPhase::Island);
    run(map, 9.8);
    CHECK(map.phase() == CampaignMapPhase::Island);
    run(map, 0.3);
    CHECK(map.done());
}

TEST_CASE("A key skips the map and the island, but not the zoom", "[ui][campaignmap]") {
    // In the original a key ends the two seconds of map and the island's
    // wait; the zoom between them always plays out.
    CampaignMapAnimation map(1);
    map.keyPressed();
    CHECK(map.phase() == CampaignMapPhase::Zoom);
    map.keyPressed();
    CHECK(map.phase() == CampaignMapPhase::Zoom);
    run(map, 1.7);
    CHECK(map.phase() == CampaignMapPhase::Island);
    map.keyPressed();
    CHECK(map.done());
}

TEST_CASE("The zoom follows the executable's curve and the territory flickers before it", "[ui][campaignmap]") {
    // The curve (VA 0x4D4A40) starts slowly and holds at the end; the next
    // mission's territory blinks white every 16 ms on the map.
    CampaignMapAnimation map(3);
    run(map, 0.02);
    CHECK(map.territoryFlashing());
    run(map, 0.02);
    CHECK_FALSE(map.territoryFlashing());
    map.keyPressed();
    CHECK_THAT(map.zoomProgress(), WithinAbs(0.0, 1e-6));
    run(map, 0.405);  // tick 8
    CHECK_THAT(map.zoomProgress(), WithinAbs(0.14, 1e-6));
    run(map, 1.0);    // tick 28
    CHECK_THAT(map.zoomProgress(), WithinAbs(1.0, 1e-6));
}

TEST_CASE("Flags are planted one every three ticks, then the arrows bounce", "[ui][campaignmap]") {
    // FUN_004833b0: a flag on each mission of the island every 150 ms; the
    // arrows round the next mission only once all are up, bouncing by the
    // table 0, 1, 2, 4, 7, 12 and back.
    CampaignMapAnimation map(12);  // Trotsville: missions 11 to 15
    map.keyPressed();
    run(map, 1.65);
    REQUIRE(map.phase() == CampaignMapPhase::Island);
    CHECK(map.flagsShown() == 0);
    run(map, 0.105);  // ticks 2
    CHECK(map.flagsShown() == 1);
    CHECK_FALSE(map.arrowsShown());
    run(map, 0.6);    // tick 14
    CHECK(map.flagsShown() == 5);
    CHECK(map.arrowsShown());
    int highest = 0;
    for (int i = 0; i < 10; ++i) {
        run(map, 0.05);
        highest = std::max(highest, map.arrowBounce());
    }
    CHECK(highest == 12);
}

TEST_CASE("Each mission opens its own island's picture", "[ui][campaignmap]") {
    // Five missions an island, Arstria only four: mission 25 has the Isle
    // of Swill to itself.
    CHECK(mapIslandOfMission(1) == 0);
    CHECK(mapIslandOfMission(5) == 0);
    CHECK(mapIslandOfMission(6) == 1);
    CHECK(mapIslandOfMission(20) == 3);
    CHECK(mapIslandOfMission(21) == 4);
    CHECK(mapIslandOfMission(24) == 4);
    CHECK(mapIslandOfMission(25) == 5);
    CHECK(mapIslandMissions(4).last == 24);
    CHECK(mapIslandMissions(5).first == 25);
}
