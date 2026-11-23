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
    REQUIRE(items.size() == kSquadSize + 2);  // the hogs, START MISSION, SAVE TEAM
    CHECK(items[0].label == "* NOBBY");
    CHECK(items[2].label == "* DEN");
    CHECK(items[3].label == "MONTY");
    CHECK(items[0].detail == "GRUNT");
    CHECK(menu.menu().selectedItem().label == "START MISSION");
    CHECK(menu.lines().front() == "MISSION 00 : MISSION 0");  // the Boot Camp; no text table: fallback name
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

TEST_CASE("The debriefing page's keys do what is printed on them", "[ui][campaign]") {
    // The pictures print SPACE CONTINUE after a victory and SPACE RETRY /
    // ESCAPE EDIT SQUAD after a defeat; a key doing something else would
    // send the player into the wrong mission or out of the campaign.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.showResults(campaign, {1, false, campaign.recordMission({false, {}}, 3)});
    CHECK(menu.answerDebrief(MenuInput::Down, campaign).request == CampaignMenuResult::Request::None);
    CHECK(menu.answerDebrief(MenuInput::Confirm, campaign).request == CampaignMenuResult::Request::StartMission);
    menu.answerDebrief(MenuInput::Back, campaign);
    CHECK(menu.screen() == CampaignScreen::Squad);

    menu.showResults(campaign, {1, false, campaign.recordMission({true, {}}, 3)});
    menu.answerDebrief(MenuInput::Back, campaign);
    CHECK(menu.screen() == CampaignScreen::Squad);
}

namespace {

std::vector<SaveSlot> slotsWithSecondUsed() {
    std::vector<SaveSlot> slots(kSaveSlots);
    slots[1].used = true;
    slots[1].name = "THE PORKERS";
    slots[1].nextMission = 4;
    return slots;
}

} // namespace

TEST_CASE("SAVE TEAM saves in the chosen slot and comes back to the squad", "[ui][campaign]") {
    // The executable's SAVE ARMY (action 25): any slot, empty or not, is
    // written at once and the squad page comes back.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.setSlots(slotsWithSecondUsed());
    menu.showSquad(campaign);
    moveTo(menu, campaign, 107);  // SAVE TEAM
    press(menu, campaign, {MenuInput::Confirm});
    REQUIRE(menu.screen() == CampaignScreen::Save);
    CHECK(menu.menu().items()[0].label == "---");
    CHECK(menu.menu().items()[1].label == "THE PORKERS");
    const auto result = press(menu, campaign, {MenuInput::Down, MenuInput::Down, MenuInput::Confirm});
    CHECK(result.request == CampaignMenuResult::Request::SaveToSlot);
    CHECK(result.slot == 2);
    CHECK(menu.screen() == CampaignScreen::Squad);
}

TEST_CASE("LOAD offers only the slots that hold a campaign", "[ui][campaign]") {
    // The executable's cursor skips empty slots on the loading board, so
    // loading can never pick a slot with nothing in it; Esc leaves.
    CampaignMenu menu(UiStrings{});
    menu.showLoad(slotsWithSecondUsed());
    CHECK(menu.screen() == CampaignScreen::Load);
    CHECK(menu.menu().selectedItem().id == 1);
    CHECK(menu.handleLoad(MenuInput::Down).request == CampaignMenuResult::Request::None);
    CHECK(menu.menu().selectedItem().id == 1);
    const auto result = menu.handleLoad(MenuInput::Confirm);
    CHECK(result.request == CampaignMenuResult::Request::LoadFromSlot);
    CHECK(result.slot == 1);
    CHECK(menu.handleLoad(MenuInput::Back).request == CampaignMenuResult::Request::Leave);
}

TEST_CASE("A new team is named before its squad page, and NAUGHTY PIGS gives 99 points", "[ui][campaign]") {
    // SELECT TEAM leads to PLEASE NAME YOUR TEAM (screen 18), which starts
    // from the nation's name; the executable's cheat name sets the points.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.showNameTeam(campaign);
    REQUIRE(menu.typingName());
    CHECK(menu.nameEntry().current() == "TOMMY'S TROTTERS");
    CHECK(menu.nameEntry().name().empty());
    for (const char letter : std::string("naughty pigs")) menu.type(letter, campaign);
    const auto result = press(menu, campaign, {MenuInput::Confirm});
    CHECK(result.request == CampaignMenuResult::Request::Changed);
    CHECK(menu.screen() == CampaignScreen::Squad);
    CHECK(campaign.teamName() == "NAUGHTY PIGS");
    CHECK(campaign.points() == kNaughtyPigsPoints);
    CHECK(menu.menu().title() == "NAUGHTY PIGS");
}

TEST_CASE("RENAME on a hog's page gives the hog the typed name", "[ui][campaign]") {
    // The hog page's third entry (screen 19) leads to PLEASE NAME YOUR
    // PIG; Esc there keeps the old name.
    Campaign campaign(0, kNames);
    CampaignMenu menu(UiStrings{});
    menu.showSquad(campaign);
    moveTo(menu, campaign, 2);  // DEN
    press(menu, campaign, {MenuInput::Confirm});
    moveTo(menu, campaign, 108);  // RENAME
    press(menu, campaign, {MenuInput::Confirm});
    REQUIRE(menu.screen() == CampaignScreen::NameHog);
    menu.type('Z', campaign);
    press(menu, campaign, {MenuInput::Back});
    CHECK(menu.screen() == CampaignScreen::Hog);
    CHECK(campaign.squad()[2].name == "DEN");

    press(menu, campaign, {MenuInput::Confirm});  // RENAME is still chosen
    REQUIRE(menu.screen() == CampaignScreen::NameHog);
    menu.type('Z', campaign);
    CHECK(press(menu, campaign, {MenuInput::Confirm}).request == CampaignMenuResult::Request::Changed);
    CHECK(campaign.squad()[2].name == "Z");  // the typed name replaces the old one
    CHECK(menu.screen() == CampaignScreen::Hog);
}
