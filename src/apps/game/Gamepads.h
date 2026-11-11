// Gamepads.h
//
// Game controllers (any pad SDL knows: Xbox, PlayStation, Switch...) for
// the menus and the matches, alongside the keyboard. In a match the pads
// work like the keyboard: whoever's turn it is plays with any of them, so
// up to four players pass the pad around (or each keep their own) on the
// same PC.
//
//   Menus:  D-pad move and change, A / Start pick, B back
//   Match:  left stick or D-pad   run, walk back, turn
//           A                     jump
//           X or right trigger    fire (hold to throw further)
//           right stick up/down   aim       right stick left/right: camera
//           LB / RB               previous / next weapon
//           Y                     end the turn       Back: another hog
//           Start                 pause menu
//
// (Button names follow the Xbox layout; SDL maps every pad to it.)

#pragma once

#include "openhogs/game/World.h"
#include "openhogs/ui/Menu.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include <optional>
#include <vector>

namespace openhogs::gameapp {

// What a pad button means in a match (button presses; held controls are
// read with Gamepads::addHeldControls).
enum class PadAction { None, Jump, PreviousWeapon, NextWeapon, EndTurn, SwitchHog, Pause };

class Gamepads {
public:
    // Starts SDL's gamepad support; without it there are simply no pads.
    Gamepads();
    ~Gamepads();
    Gamepads(const Gamepads&) = delete;
    Gamepads& operator=(const Gamepads&) = delete;

    // Opens and closes pads as they are plugged in and out. Returns true if
    // the event was about a pad being added or removed.
    bool handleDeviceEvent(const SDL_Event& event);

    // The menu command of a pad button press, if it has one.
    static std::optional<ui::MenuInput> menuInputForButton(Uint8 button);
    static PadAction actionForButton(Uint8 button);

    // Adds what the pads hold right now (sticks, D-pad, fire) to `input`.
    void addHeldControls(game::PlayerInput& input) const;

    // How far the camera should turn, -1 (left) to 1 (right), from the
    // right sticks.
    float cameraTurn() const;

private:
    float strongestAxis(SDL_GamepadAxis axis) const;
    bool anyButton(SDL_GamepadButton button) const;

    bool available_ = false;
    std::vector<SDL_Gamepad*> pads_;
};

} // namespace openhogs::gameapp
