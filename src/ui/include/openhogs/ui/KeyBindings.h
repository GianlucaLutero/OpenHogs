// KeyBindings.h
//
// Which keyboard keys do what during a match, as the player chose them on
// the CONTROLS screen (the original lets every key be changed there: the
// row blinks until a key is pressed).
//
// Keys are kept by name, the names SDL gives to physical keys ("W", "Up",
// "Left Ctrl", "Space"...), so that this file needs no SDL and can be
// unit tested; the game turns the names into key codes. Physical keys are
// used rather than the letters they print, so the default layout stays in
// the same place on AZERTY or QWERTZ keyboards.
//
// The choice is saved in the [Controls] section of config/openhogs.ini,
// one line per control with its keys separated by commas:
//
//     [Controls]
//     Forward = W, Up
//     Fire    = Left Ctrl, X
//
// A control missing from the file keeps its default keys.

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::ui {

// Everything a key can do in a match, in the order of the CONTROLS screen.
enum class Control {
    Forward,
    Backward,
    TurnLeft,
    TurnRight,
    Fire,
    Jump,
    WeaponMenu,  // the original's WEAPONS MENU: opens and closes it
    PreviousWeapon,
    NextWeapon,
    Sight,     // the original's AIM / LOOK: the rifle sight
    AimDown,
    AimUp,
    NextHog,   // with Shift held: the previous hog
    EndTurn,
    Pause,
};
inline constexpr std::size_t kControlCount = 15;

// The key that always pauses a match, whatever the bindings say, so that a
// player who rebinds every key can still reach the pause menu. It cannot
// be given to a control (on the CONTROLS screen it cancels the change).
inline constexpr std::string_view kAlwaysPauseKey = "Escape";

// The [Controls] key of a control in openhogs.ini ("Forward", "AimUp"...).
std::string_view controlIniKey(Control control);

class KeyBindings {
public:
    // OpenHogs' layout: W/S/A/D or the arrows, Ctrl or X to fire, Space to
    // jump, C weapon menu, Q/E weapons, V sight, F/R angle, Tab hog, Enter
    // end turn, P pause.
    static KeyBindings defaults();

    // The defaults, with every control named in [Controls] replaced by the
    // keys written there.
    static KeyBindings fromIni(const core::IniFile& ini);

    const std::vector<std::string>& keys(Control control) const;

    // Gives `key` to `control` alone: the control's other keys are dropped,
    // and any other control that had this key loses it, so that one key
    // never does two things. A control that would be left with no key at
    // all takes the chosen control's old first key (the two swap).
    // kAlwaysPauseKey is refused (returns false).
    bool bind(Control control, std::string_view key);

    // The control a key belongs to, if any (names compared without regard
    // to case).
    std::optional<Control> controlFor(std::string_view key) const;

    // The keys as the CONTROLS screen writes them: "W / UP" ("-" if none).
    std::string describe(Control control) const;

    // Every control's [Controls] line, ready for core::updateIniFile().
    std::vector<std::pair<std::string, std::string>> iniValues() const;

private:
    std::array<std::vector<std::string>, kControlCount> keys_;
};

} // namespace openhogs::ui
