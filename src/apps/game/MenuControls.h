// MenuControls.h
//
// The keys of the menus and the sounds they make, shared by the menus
// before a match (Application) and the pause menu (Game):
//
//   Up / W, Down / S      move the selection
//   Left / A, Right / D   change a slider
//   Enter, Space          pick
//   Esc, Backspace        back
//
// Sounds come from the menu bank FESounds/Fesounds.srl. Which click goes
// with which action is OpenHogs' choice (docs/open-decisions.md).

#pragma once

#include "SoundPlayer.h"

#include "openhogs/ui/FrontEnd.h"
#include "openhogs/ui/Menu.h"

#include <SDL3/SDL_keycode.h>

#include <optional>

namespace openhogs::gameapp {

inline std::optional<ui::MenuInput> menuInputForKey(SDL_Keycode key) {
    switch (key) {
    case SDLK_UP:
    case SDLK_W: return ui::MenuInput::Up;
    case SDLK_DOWN:
    case SDLK_S: return ui::MenuInput::Down;
    case SDLK_LEFT:
    case SDLK_A: return ui::MenuInput::Left;
    case SDLK_RIGHT:
    case SDLK_D: return ui::MenuInput::Right;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE: return ui::MenuInput::Confirm;
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE: return ui::MenuInput::Back;
    default: return std::nullopt;
    }
}

inline void playMenuSound(SoundPlayer& sounds, ui::MenuSound sound) {
    switch (sound) {
    case ui::MenuSound::Move: sounds.play(SoundBankId::FrontEnd, "click1"); break;
    case ui::MenuSound::Select: sounds.play(SoundBankId::FrontEnd, "click5"); break;
    case ui::MenuSound::Back: sounds.play(SoundBankId::FrontEnd, "Indu006"); break;
    case ui::MenuSound::None: break;
    }
}

} // namespace openhogs::gameapp
