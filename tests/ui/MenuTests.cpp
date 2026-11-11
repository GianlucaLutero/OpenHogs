// Tests for the menus: ui::Menu, ui::FrontEnd, ui::PauseMenu, and the
// timed messages of ui::MessageBanner.
//
// These decide where every key press in a menu leads. A mistake can start
// the wrong level, quit the game by accident, lose a volume change, or
// leave the player stuck on a page with no way back.

#include "openhogs/ui/FrontEnd.h"
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/MessageBanner.h"
#include "openhogs/ui/PauseMenu.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;

namespace {

MenuItem action(int id, bool enabled = true) {
    MenuItem item;
    item.id = id;
    item.label = "ITEM " + std::to_string(id);
    item.enabled = enabled;
    return item;
}

MenuItem slider(int id, int value) {
    MenuItem item = action(id);
    item.kind = MenuItem::Kind::Slider;
    item.value = value;
    item.maxValue = 10;
    return item;
}

FrontEnd makeFrontEnd() {
    return FrontEnd(UiStrings{}, {"BOOM", "BRIDGE", "CAMP"}, VolumeLevels{});
}

FrontEndResult press(FrontEnd& frontEnd, std::initializer_list<MenuInput> inputs) {
    FrontEndResult last;
    for (const auto input : inputs) last = frontEnd.handle(input);
    return last;
}

} // namespace

TEST_CASE("The selection skips disabled items and wraps around", "[ui][menu]") {
    // ONE PLAYER is shown greyed out until the campaign exists: the cursor
    // must never land on it, and moving past either end must wrap like
    // the original's menus.
    Menu menu("TITLE", {action(0, false), action(1), action(2)});
    CHECK(menu.selectedItem().id == 1);  // starts on the first usable item

    CHECK(menu.handle(MenuInput::Down).type == MenuResponse::Type::Moved);
    CHECK(menu.selectedItem().id == 2);
    menu.handle(MenuInput::Down);
    CHECK(menu.selectedItem().id == 1);  // wrapped, skipping item 0
    menu.handle(MenuInput::Up);
    CHECK(menu.selectedItem().id == 2);
}

TEST_CASE("Confirm activates actions; left and right change sliders within limits", "[ui][menu]") {
    // Sliders must stop at 0 and at their maximum, and confirming a slider
    // must not count as picking an action.
    Menu menu("VOLUME", {slider(5, 9), action(6)});
    CHECK(menu.handle(MenuInput::Confirm).type == MenuResponse::Type::None);

    CHECK(menu.handle(MenuInput::Right).type == MenuResponse::Type::Changed);
    CHECK(menu.item(5).value == 10);
    CHECK(menu.handle(MenuInput::Right).type == MenuResponse::Type::None);  // already full

    menu.handle(MenuInput::Down);
    CHECK(menu.handle(MenuInput::Left).type == MenuResponse::Type::None);  // not a slider
    const auto response = menu.handle(MenuInput::Confirm);
    CHECK(response.type == MenuResponse::Type::Activated);
    CHECK(response.itemId == 6);
}

TEST_CASE("Long menus scroll so the selected item stays visible", "[ui][menu]") {
    // The level list can hold more maps than fit on screen.
    std::vector<MenuItem> items;
    for (int i = 0; i < 20; ++i) items.push_back(action(i));
    Menu menu("LEVELS", items);
    CHECK(menu.firstVisible(9) == 0);
    for (int i = 0; i < 15; ++i) menu.handle(MenuInput::Down);
    const auto first = menu.firstVisible(9);
    CHECK(first <= 15);
    CHECK(first + 9 > 15);
    for (int i = 0; i < 4; ++i) menu.handle(MenuInput::Down);  // the last item
    CHECK(menu.firstVisible(9) == 11);                         // the list ends at the bottom
}

TEST_CASE("DEATH MATCH: picking a level starts a match against the computer", "[ui][frontend]") {
    // ONE PLAYER -> DEATH MATCH is a skirmish: the player has the team that
    // plays first, the computer all the others.
    auto frontEnd = makeFrontEnd();
    CHECK(frontEnd.screen() == FrontEndScreen::Main);
    press(frontEnd, {MenuInput::Confirm});  // ONE PLAYER
    CHECK(frontEnd.screen() == FrontEndScreen::OnePlayer);
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm});  // DEATH MATCH (LOAD GAME is greyed out: no save)
    CHECK(frontEnd.screen() == FrontEndScreen::SelectLevel);

    const auto chosen = press(frontEnd, {MenuInput::Down, MenuInput::Confirm});
    CHECK(chosen.request == FrontEndResult::Request::LevelChosen);
    CHECK(chosen.map == "BRIDGE");

    const auto start = frontEnd.setLevelTeams({2, 0, 5});
    CHECK(start.request == FrontEndResult::Request::StartMatch);
    CHECK(start.map == "BRIDGE");
    CHECK(start.settings.computerTeams == std::vector<int>{0, 5});
}

TEST_CASE("MULTI-PLAYER: each team gets a nation and a player, CPU or nobody", "[ui][frontend]") {
    // The PLAYERS page has two rows per team of the map: its nation, and who
    // plays it (PLAYER, CPU or OFF). START GAME passes the choices on.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm, MenuInput::Confirm});  // MULTI-PLAYER, BOOM
    REQUIRE(frontEnd.setLevelTeams({0, 1, 2}).request == FrontEndResult::Request::None);
    REQUIRE(frontEnd.screen() == FrontEndScreen::Players);
    CHECK(frontEnd.menu().selectedItem().label == "START GAME");  // ready to go: everybody human
    CHECK(frontEnd.menu().item(0).label == "TEAM A");
    CHECK(frontEnd.menu().item(0).choiceText() == "TOMMY'S TROTTERS");  // team 0's usual nation

    press(frontEnd, {MenuInput::Up, MenuInput::Left});  // the third team: who plays -> OFF (wraps round)
    CHECK(frontEnd.menu().selectedItem().choiceText() == "OFF");
    press(frontEnd, {MenuInput::Up, MenuInput::Up, MenuInput::Right});  // the second team -> CPU
    CHECK(frontEnd.menu().selectedItem().choiceText() == "CPU");
    press(frontEnd, {MenuInput::Up, MenuInput::Right, MenuInput::Right});  // its nation, two steps on
    CHECK(frontEnd.menu().selectedItem().choiceText() == "SOW-A-KRAUTS");

    for (int i = 0; i < 4; ++i) press(frontEnd, {MenuInput::Down});  // START GAME
    const auto start = press(frontEnd, {MenuInput::Confirm});
    CHECK(start.request == FrontEndResult::Request::StartMatch);
    CHECK(start.map == "BOOM");
    CHECK(start.settings.computerTeams == std::vector<int>{1});
    CHECK(start.settings.conditions.removedTeams == std::vector<int>{2});
    CHECK(start.settings.teamNations.at(1) == 3);
    CHECK(start.settings.teamNations.at(0) == 0);

    // Back from PLAYERS returns to the level list.
    frontEnd.setLevelTeams({0, 1});
    press(frontEnd, {MenuInput::Back});
    CHECK(frontEnd.screen() == FrontEndScreen::SelectLevel);
}

TEST_CASE("A match needs at least two teams in play", "[ui][frontend]") {
    // With every team but one switched off there is nobody to fight.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm, MenuInput::Confirm});
    frontEnd.setLevelTeams({0, 1});
    press(frontEnd, {MenuInput::Up, MenuInput::Left});  // the second team -> OFF
    const auto start = press(frontEnd, {MenuInput::Down, MenuInput::Confirm});
    CHECK(start.request == FrontEndResult::Request::None);
    CHECK(frontEnd.screen() == FrontEndScreen::Players);
}

TEST_CASE("Field conditions set hogs per team, turn time, health and sudden death", "[ui][frontend]") {
    // The original's FIELD CONDITIONS page; the defaults leave the map as it is.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm, MenuInput::Confirm});
    frontEnd.setLevelTeams({0, 1});
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm});  // FIELD CONDITIONS
    REQUIRE(frontEnd.screen() == FrontEndScreen::FieldConditions);
    CHECK(frontEnd.menu().item(1).choiceText() == "60");  // turn time
    press(frontEnd, {MenuInput::Right, MenuInput::Right, MenuInput::Right});  // 3 hogs per team
    press(frontEnd, {MenuInput::Down, MenuInput::Left, MenuInput::Left});     // 30 s
    press(frontEnd, {MenuInput::Down, MenuInput::Right, MenuInput::Right});   // double health
    press(frontEnd, {MenuInput::Down, MenuInput::Right});                     // sudden death at 10
    press(frontEnd, {MenuInput::Back});
    REQUIRE(frontEnd.screen() == FrontEndScreen::Players);
    const auto start = press(frontEnd, {MenuInput::Up, MenuInput::Confirm});  // START GAME
    REQUIRE(start.request == FrontEndResult::Request::StartMatch);
    CHECK(start.settings.conditions.hogsPerTeam == 3);
    CHECK(start.settings.turnSeconds == 30.0);
    CHECK(start.settings.conditions.healthFactor == 2.0f);
    CHECK(start.settings.suddenDeathRound == 10);
}

TEST_CASE("A level without teams cannot be started", "[ui][frontend]") {
    // Some maps are scenery only; picking one must not start an empty match.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm, MenuInput::Confirm});  // MULTI-PLAYER, BOOM
    CHECK(frontEnd.setLevelTeams({}).request == FrontEndResult::Request::None);
    CHECK(frontEnd.screen() == FrontEndScreen::SelectLevel);
}

TEST_CASE("ONE PLAYER: NEW GAME asks for a nation, LOAD GAME needs a saved campaign", "[ui][frontend]") {
    // A campaign is fought for one of the six nations; LOAD GAME must not be
    // offered when there is nothing to load.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Confirm});
    CHECK_FALSE(frontEnd.menu().item(1).enabled);  // LOAD GAME
    press(frontEnd, {MenuInput::Confirm});  // NEW GAME
    REQUIRE(frontEnd.screen() == FrontEndScreen::SelectTeam);
    CHECK(frontEnd.menu().items().size() == 7);  // six nations and BACK
    const auto chosen = press(frontEnd, {MenuInput::Down, MenuInput::Confirm});
    CHECK(chosen.request == FrontEndResult::Request::NewCampaign);
    CHECK(chosen.nation == 1);

    frontEnd.setSavedCampaign(true);
    CHECK(frontEnd.menu().selectedItem().id == 1);  // LOAD GAME first when there is a save
    CHECK(press(frontEnd, {MenuInput::Confirm}).request == FrontEndResult::Request::LoadCampaign);
}

TEST_CASE("Choices go round with left and right", "[ui][menu]") {
    // With two words (PLAYER / CPU) either key must swap them.
    MenuItem choice = action(1);
    choice.kind = MenuItem::Kind::Choice;
    choice.choices = {"PLAYER", "CPU"};
    Menu menu("PLAYERS", {choice});
    CHECK(menu.handle(MenuInput::Left).type == MenuResponse::Type::Changed);
    CHECK(menu.item(1).choiceText() == "CPU");
    menu.handle(MenuInput::Left);
    CHECK(menu.item(1).choiceText() == "PLAYER");
    CHECK(menu.handle(MenuInput::Confirm).type == MenuResponse::Type::None);  // not an action
}

TEST_CASE("Back always leads back to the main menu", "[ui][frontend]") {
    // Every page must have a way out, by Esc or by its BACK entry.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Confirm, MenuInput::Back});
    CHECK(frontEnd.screen() == FrontEndScreen::Main);

    press(frontEnd, {MenuInput::Down, MenuInput::Down, MenuInput::Confirm});  // OPTIONS
    CHECK(frontEnd.screen() == FrontEndScreen::Options);
    // BACK is the last entry, after the four sliders.
    const auto result = press(frontEnd, {MenuInput::Up, MenuInput::Confirm});
    CHECK(frontEnd.screen() == FrontEndScreen::Main);
    CHECK(result.sound == MenuSound::Back);
}

TEST_CASE("Quitting asks first, and a hasty double Enter does not quit", "[ui][frontend]") {
    // Esc on the main menu, or QUIT, opens ARE YOU SURE? with NO selected:
    // the game must never close because a key was pressed twice.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Back});
    CHECK(frontEnd.screen() == FrontEndScreen::ConfirmQuit);
    CHECK(press(frontEnd, {MenuInput::Confirm}).request == FrontEndResult::Request::None);
    CHECK(frontEnd.screen() == FrontEndScreen::Main);

    press(frontEnd, {MenuInput::Back});
    CHECK(press(frontEnd, {MenuInput::Up, MenuInput::Confirm}).request == FrontEndResult::Request::Quit);
}

TEST_CASE("Volume sliders report every change", "[ui][frontend]") {
    // The application applies the new volumes at once, so the player hears
    // the difference while moving the slider.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Down, MenuInput::Down, MenuInput::Confirm});  // OPTIONS, on MASTER
    const auto result = press(frontEnd, {MenuInput::Left, MenuInput::Left});
    CHECK(result.request == FrontEndResult::Request::VolumesChanged);
    CHECK(frontEnd.volumes().master == kMaxVolumeLevel - 2);

    press(frontEnd, {MenuInput::Down, MenuInput::Left});  // SFX
    CHECK(frontEnd.volumes().effects == kMaxVolumeLevel - 1);
    CHECK(frontEnd.volumes().master == kMaxVolumeLevel - 2);
}

TEST_CASE("The pause menu resumes, or aborts only after confirmation", "[ui][pause]") {
    // Aborting throws the match away, so it needs a YES; Esc and CONTINUE
    // must both get back into the match.
    PauseMenu pause{UiStrings{}};
    CHECK(pause.handle(MenuInput::Back).request == PauseResult::Request::Resume);

    pause.reset();
    CHECK(pause.handle(MenuInput::Confirm).request == PauseResult::Request::Resume);

    pause.reset();
    pause.handle(MenuInput::Down);  // ABORT SKIRMISH
    CHECK(pause.handle(MenuInput::Confirm).request == PauseResult::Request::None);
    CHECK(pause.confirming());
    CHECK(pause.handle(MenuInput::Confirm).request == PauseResult::Request::None);  // NO is selected
    CHECK_FALSE(pause.confirming());

    pause.handle(MenuInput::Confirm);  // ABORT SKIRMISH again
    pause.handle(MenuInput::Up);       // YES
    CHECK(pause.handle(MenuInput::Confirm).request == PauseResult::Request::Abort);
}

TEST_CASE("Messages disappear after their time; a new one replaces the old", "[ui][banner]") {
    // "GET READY ..." must not stay over the next turn, and the end of the
    // match message must stay until the players leave.
    MessageBanner banner;
    banner.show("GET READY", 2.0);
    banner.update(1.0);
    REQUIRE(banner.current());
    CHECK(banner.progress() == 0.5);
    banner.show("VICTORY", -1.0);
    CHECK(*banner.current() == "VICTORY");
    banner.update(100.0);
    CHECK(banner.current());

    banner.show("GET READY", 2.0);
    banner.update(2.5);
    CHECK_FALSE(banner.current());
}
