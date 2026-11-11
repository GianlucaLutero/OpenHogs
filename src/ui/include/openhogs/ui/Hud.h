// Hud.h
//
// What the HUD of a match shows, worked out from the state of the world:
// whose turn it is, the active hog's class, health and weapon, the time
// and movement left, and the power of a throw being charged. Also the
// match messages ("GET READY ...", "VICTORY TO ...!!") that follow game
// events.
//
// The renderer only draws this information, so everything the player is
// told can be unit tested here without a window.

#pragma once

#include "openhogs/game/GameEvents.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/game/World.h"
#include "openhogs/ui/UiStrings.h"

#include <optional>
#include <string>
#include <vector>

namespace openhogs::ui {

struct HudInfo {
    int team = 0;  // nation index (for the team colour)
    std::string teamName;
    std::string className;
    int health = 0;               // whole points, rounded up (1 while alive)
    float healthFraction = 0.0f;  // 0 .. 1
    game::WeaponId weapon = game::WeaponId::Trotter;
    std::string weaponName;
    int secondsLeft = 0;            // rounded up
    float movementFraction = 0.0f;  // 0 .. 1 of the turn's allowance
    std::optional<float> power;     // 0 .. 1 while charging a throw
    int turnNumber = 0;
};

// The name of a weapon in the game's language: gtext entry number = weapon
// id. English fallback when the table lacks it.
std::string weaponDisplayName(game::WeaponId weapon, const std::vector<std::string>& gameText);

// The HUD of the active hog, or nothing when there is no turn to show
// (a map without hogs, or the match is over).
std::optional<HudInfo> describeTurn(const game::World& world, const UiStrings& strings,
                                    const std::vector<std::string>& gameText);

// The message of the start of the current turn: "GET READY >S..." for a
// team played by a person, "THE ENEMY RETALIATES..." for the computer.
std::optional<std::string> turnMessage(const game::World& world, const UiStrings& strings, bool computerTurn = false);

// The message a game event deserves, if any: the start of a turn (see
// turnMessage) or the end of the match.
std::optional<std::string> messageForEvent(const game::GameEvent& event, const game::World& world,
                                           const UiStrings& strings, bool computerTurn = false);

} // namespace openhogs::ui
