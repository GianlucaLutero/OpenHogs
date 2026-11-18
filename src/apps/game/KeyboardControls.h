// KeyboardControls.h
//
// The player's key bindings (ui::KeyBindings, chosen on the CONTROLS
// screen) turned into SDL key codes for a match: whether a control's key
// is held, and which control a pressed key belongs to.
//
// Bindings name physical keys (SDL scancode names such as "W" or
// "Left Ctrl"); a name SDL does not know, e.g. a typo in openhogs.ini, is
// reported once and ignored.

#pragma once

#include "openhogs/ui/KeyBindings.h"

#include <SDL3/SDL_scancode.h>

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace openhogs::gameapp {

class KeyboardControls {
public:
    explicit KeyboardControls(const ui::KeyBindings& bindings = ui::KeyBindings::defaults());

    // Whether any key of `control` is down in `keyboardState`
    // (SDL_GetKeyboardState).
    bool held(ui::Control control, const bool* keyboardState) const;

    // The control a pressed key belongs to, if any.
    std::optional<ui::Control> controlFor(SDL_Scancode key) const;

private:
    std::array<std::vector<SDL_Scancode>, ui::kControlCount> keys_;
};

// The name a key is saved under (SDL's name of the physical key).
std::string keyName(SDL_Scancode key);

} // namespace openhogs::gameapp
