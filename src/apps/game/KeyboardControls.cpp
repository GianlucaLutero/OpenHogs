// KeyboardControls.cpp - see KeyboardControls.h for an overview.

#include "KeyboardControls.h"

#include <SDL3/SDL_keyboard.h>

#include <algorithm>
#include <iostream>

namespace openhogs::gameapp {

KeyboardControls::KeyboardControls(const ui::KeyBindings& bindings) {
    for (std::size_t i = 0; i < ui::kControlCount; ++i) {
        const auto control = static_cast<ui::Control>(i);
        for (const auto& name : bindings.keys(control)) {
            const SDL_Scancode key = SDL_GetScancodeFromName(name.c_str());
            if (key == SDL_SCANCODE_UNKNOWN) {
                std::cerr << "Warning: [Controls] " << ui::controlIniKey(control) << ": unknown key '" << name
                          << "' ignored\n";
                continue;
            }
            keys_[i].push_back(key);
        }
    }
}

bool KeyboardControls::held(ui::Control control, const bool* keyboardState) const {
    const auto& keys = keys_[static_cast<std::size_t>(control)];
    return std::any_of(keys.begin(), keys.end(), [&](SDL_Scancode key) { return keyboardState[key]; });
}

std::optional<ui::Control> KeyboardControls::controlFor(SDL_Scancode key) const {
    for (std::size_t i = 0; i < ui::kControlCount; ++i) {
        if (std::find(keys_[i].begin(), keys_[i].end(), key) != keys_[i].end()) return static_cast<ui::Control>(i);
    }
    return std::nullopt;
}

std::string keyName(SDL_Scancode key) {
    return SDL_GetScancodeName(key);
}

} // namespace openhogs::gameapp
