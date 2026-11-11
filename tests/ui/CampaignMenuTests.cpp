// Tests for ui::CampaignMenu: the campaign pages between missions.
//
// These pages are where the player spends promotion points and picks the
// team: a promotion that is offered but not paid for, a swap that does not
// stick, or a results page that leads nowhere would break the campaign.

#include "openhogs/ui/CampaignMenu.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using namespace openhogs::game;

namespace {

const std::vector<std::string> kNames{"NOBBY", "GINGER", "DEN", "MONTY", "BASIL", "PONSONBY", "PERCY", "SMITH", "JONES"};

CampaignMenuResult press(CampaignMenu& menu, Campaign& campaign, std::initializer_list<MenuInput> inputs) {
    CampaignMenuResult last;
    for (const auto input : inputs) last = menu.handle(input, campaign);
    return last;
}

// Selects the item with `id` by moving down (the menus wrap round).
void moveTo(CampaignMenu& menu, Campaign& campaign, int id) {
    for (std::size_t i = 0; i < 20 && menu.menu().selectedItem().id != id; ++i) {
        menu.handle(MenuInput::Down, campaign);
    }
}

} // namespace

TEST_CASE("The squad page marks the hogs that go on the mission", "[ui][campaign]") {
    // The first hogs of the squad are the team; the page must say which.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{}, [](const Campaign&) { return std::size_t{3}; });
    menu.showSquad(campaign);
    const auto& items = menu.menu().items();
    REQUIRE(items.size() == kSquadSize + 2);  // the hogs, START MISSION, BACK
    CHECK(items[0].label == "* NOBBY");
    CHECK(items[2].label == "* DEN");
    CHECK(items[3].label == "MONTY");
    CHECK(items[0].detail == "GRUNT");
    CHECK(menu.menu().selectedItem().label == "START MISSION");
    CHECK(menu.lines().front() == "MISSION 01 : MISSION 1");  // no text table: fallback name
    CHECK(press(menu, campaign, {MenuInput::Confirm}).request == CampaignMenuResult::Request::StartMission);
}

TEST_CASE("Promoting a hog spends the points and asks to save", "[ui][campaign]") {
    // Hog -> PROMOTE -> a career: the campaign changes and must be saved.
    Campaign campaign(0, kNames);
    campaign.recordMission({true, {}}, 3);  // 2 points
    CampaignMenu menu(UiStrings{});
    menu.showSquad(campaign);
    moveTo(menu, campaign, 0);
    press(menu, campaign, {MenuInput::Confirm});  // NOBBY
    REQUIRE(menu.screen() == CampaignScreen::Hog);
    press(menu, campaign, {MenuInput::Confirm});  // PROMOTE
    REQUIRE(menu.screen() == CampaignScreen::Promotion);
    CHECK(menu.menu().title() == "CAREER PATH");
    const auto result = press(menu, campaign, {MenuInput::Confirm});  // the first career: heavy weapons
    CHECK(result.request == CampaignMenuResult::Request::Changed);
    CHECK(campaign.squad()[0].rank == RankId::Gunner);
    CHECK(campaign.points() == 1);
    CHECK(menu.screen() == CampaignScreen::Squad);
}

TEST_CASE("Without points, promotions are shown but cannot be picked", "[ui][campaign]") {
    // The player can look at the careers before earning points.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.showSquad(campaign);
    moveTo(menu, campaign, 0);
    press(menu, campaign, {MenuInput::Confirm});
    CHECK_FALSE(menu.menu().item(102).enabled);  // PROMOTE greyed out
}

TEST_CASE("Swapping brings a reserve into the team", "[ui][campaign]") {
    // Hog -> SWAP POSITION -> another hog: their places change.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.showSquad(campaign);
    moveTo(menu, campaign, 0);
    press(menu, campaign, {MenuInput::Confirm});  // NOBBY
    moveTo(menu, campaign, 103);                  // SWAP POSITION
    press(menu, campaign, {MenuInput::Confirm});
    REQUIRE(menu.screen() == CampaignScreen::Swap);
    moveTo(menu, campaign, 7);
    CHECK(press(menu, campaign, {MenuInput::Confirm}).request == CampaignMenuResult::Request::Changed);
    CHECK(campaign.squad()[0].name == "SMITH");
    CHECK(campaign.squad()[7].name == "NOBBY");
}

TEST_CASE("After a won mission the results page leads back to the squad", "[ui][campaign]") {
    // CONTINUE returns to the squad page for the next mission.
    Campaign campaign(0, kNames);
    const auto outcome = campaign.recordMission({true, {}}, 3);
    CampaignMenu menu(UiStrings{});
    menu.showResults(campaign, {1, false, outcome});
    CHECK(menu.screen() == CampaignScreen::Results);
    CHECK(menu.lines()[1] == "LEVEL COMPLETE");
    CHECK(menu.lines()[2] == "SURVIVAL BONUS");
    press(menu, campaign, {MenuInput::Confirm});
    CHECK(menu.screen() == CampaignScreen::Squad);
}

TEST_CASE("After a failed mission the player can retry at once", "[ui][campaign]") {
    // RETRY starts the same mission again; EDIT SQUAD goes to the squad.
    Campaign campaign(0, kNames);
    const auto outcome = campaign.recordMission({false, {}}, 3);
    CampaignMenu menu(UiStrings{});
    menu.showResults(campaign, {1, false, outcome});
    CHECK(menu.lines()[1] == "MISSION FAILED!");
    CHECK(press(menu, campaign, {MenuInput::Confirm}).request == CampaignMenuResult::Request::StartMission);

    menu.showResults(campaign, {1, true, {}});
    CHECK(menu.lines()[1] == "MISSION ABORTED");
    press(menu, campaign, {MenuInput::Down, MenuInput::Confirm});
    CHECK(menu.screen() == CampaignScreen::Squad);
}
