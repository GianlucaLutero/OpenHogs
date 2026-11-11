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

TEST_CASE("Picking a level from the main menu starts that level", "[ui][frontend]") {
    // MULTI-PLAYER -> SELECT LEVEL -> a map is the way into a match.
    auto frontEnd = makeFrontEnd();
    CHECK(frontEnd.screen() == FrontEndScreen::Main);
    press(frontEnd, {MenuInput::Confirm});  // MULTI-PLAYER (ONE PLAYER is disabled)
    CHECK(frontEnd.screen() == FrontEndScreen::SelectLevel);

    const auto result = press(frontEnd, {MenuInput::Down, MenuInput::Confirm});
    CHECK(result.request == FrontEndResult::Request::StartMatch);
    CHECK(result.map == "BRIDGE");
}

TEST_CASE("Back always leads back to the main menu", "[ui][frontend]") {
    // Every page must have a way out, by Esc or by its BACK entry.
    auto frontEnd = makeFrontEnd();
    press(frontEnd, {MenuInput::Confirm, MenuInput::Back});
    CHECK(frontEnd.screen() == FrontEndScreen::Main);

    press(frontEnd, {MenuInput::Down, MenuInput::Confirm});  // OPTIONS
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
    press(frontEnd, {MenuInput::Down, MenuInput::Confirm});  // OPTIONS, on MASTER
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
