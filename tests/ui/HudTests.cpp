// Tests for ui::UiStrings and ui::Hud: the texts of the interface and what
// the HUD tells the players.
//
// The HUD is how the players know whose turn it is, how hurt their hog is
// and how much time and movement is left; the texts must follow the
// language pack. Wrong numbers here mislead every decision in a match.

#include "openhogs/ui/Hud.h"
#include "openhogs/ui/UiStrings.h"

#include "../game/GameTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using namespace openhogs::game;
using namespace openhogs::tests;

namespace {

// Two hogs on flat ground: team 0 (british) plays first, team 2 (french).
World makeWorld() {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 2)};
    return World(map, hogs);
}

} // namespace

TEST_CASE("Interface texts come from the text tables, English when missing", "[ui][strings]") {
    // A language pack replaces fetext and gtext: its words must appear in
    // the menus. Empty or missing entries must not leave blank buttons.
    std::vector<std::string> frontEnd(700, " ");
    frontEnd[10] = "MULTIGIOCATORE";
    frontEnd[25] = "MAIALI DI SUA MAESTÀ";
    std::vector<std::string> game(200);
    game[165] = "VITTORIA PER >S!!";
    game[63] = "SOLDATO";

    const auto strings = loadUiStrings(frontEnd, game);
    CHECK(strings.multiPlayer == "MULTIGIOCATORE");
    CHECK(strings.teamName(0) == "MAIALI DI SUA MAESTÀ");
    CHECK(strings.victory == "VITTORIA PER >S!!");
    CHECK(strings.className("grunt") == "SOLDATO");
    CHECK(strings.options == "OPTIONS");    // blank entry: English
    CHECK(strings.paused == "-GAME PAUSED-");  // missing entry: English

    const auto english = loadUiStrings({}, {});
    CHECK(english.mainMenu == "MAIN MENU");
    CHECK(english.className("unknown") == "UNKNOWN");
}

TEST_CASE("The HUD shows the active hog's team, class, health and weapon", "[ui][hud]") {
    // The panel must describe the hog being played, not another one.
    auto world = makeWorld();
    std::vector<std::string> gameText(200);
    gameText[static_cast<std::size_t>(WeaponId::Trotter)] = "ZAMPATA";

    const auto info = describeTurn(world, UiStrings{}, gameText);
    REQUIRE(info);
    CHECK(info->team == 0);
    CHECK(info->teamName == "TOMMY'S TROTTERS");
    CHECK(info->className == "GRUNT");
    CHECK(info->health == 100);
    CHECK(info->healthFraction == 1.0f);
    CHECK(info->weapon == world.hogs()[0].selectedWeapon());
    CHECK(info->secondsLeft == 60);
    CHECK(info->movementFraction == 1.0f);
    CHECK_FALSE(info->power);
    CHECK(info->turnNumber == 1);
    if (info->weapon == WeaponId::Trotter) {
        CHECK(info->weaponName == "ZAMPATA");
    }
}

TEST_CASE("Health is rounded up so a living hog never shows 0", "[ui][hud]") {
    // A hog with 0.3 health left is still alive and still playable;
    // showing 0 would tell the player it is dead.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 2)};
    hogs[0].health = 0.3f;
    World world(map, hogs);
    const auto info = describeTurn(world, UiStrings{}, {});
    REQUIRE(info);
    CHECK(info->health == 1);
}

TEST_CASE("Weapon names fall back to English without a text table", "[ui][hud]") {
    // Without gtext (a broken install) the HUD must still name the weapon.
    CHECK(weaponDisplayName(WeaponId::Grenade, {}) == weaponSpec(WeaponId::Grenade).fallbackName);
}

TEST_CASE("Turn and match messages name the right team", "[ui][hud]") {
    // "GET READY >S..." names the team whose turn starts; the end message
    // names the winner, or says it is a draw.
    auto world = makeWorld();
    const UiStrings strings;
    CHECK(turnMessage(world, strings) == "GET READY TOMMY'S TROTTERS...");
    // The computer's turns are announced as the enemy's.
    CHECK(turnMessage(world, strings, true) == "THE ENEMY RETALIATES...");

    GameEvent over;
    over.type = GameEventType::MatchOver;
    over.team = 2;
    CHECK(messageForEvent(over, world, strings) == "VICTORY TO GARLIC GRUNTS!!");
    over.team = -1;
    CHECK(messageForEvent(over, world, strings) == "IT'S A DRAW!!");

    GameEvent footstep;
    CHECK_FALSE(messageForEvent(footstep, world, strings));
}

TEST_CASE("The HUD is hidden once the match is over", "[ui][hud]") {
    // After the last enemy falls the victory message stands alone; a
    // stale turn panel would suggest the match goes on.
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 2)};
    hogs[1].health = 0.0f;
    hogs[1].alive = false;
    World world(map, hogs);
    static const auto animations = makeAnimations();
    for (int i = 0; i < 200 && world.phase() != MatchPhase::Over; ++i) {
        world.update(kStep, PlayerInput{}, animations);
    }
    REQUIRE(world.phase() == MatchPhase::Over);
    CHECK_FALSE(describeTurn(world, UiStrings{}, {}));
}
