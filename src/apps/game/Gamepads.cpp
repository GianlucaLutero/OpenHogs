// Gamepads.cpp - see Gamepads.h for an overview.

#include "Gamepads.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// Sticks are -32768..32767; below this share of a full push they are
// treated as centred, so a worn stick does not creep.
constexpr float kDeadZone = 0.35f;
constexpr float kAxisScale = 1.0f / 32767.0f;

} // namespace

Gamepads::Gamepads() {
    available_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!available_) {
        std::cerr << "Warning: no gamepad support (" << SDL_GetError() << ")\n";
        return;
    }
    // Pads already plugged in also arrive as "added" events; nothing to do here.
}

Gamepads::~Gamepads() {
    for (SDL_Gamepad* pad : pads_) {
        SDL_CloseGamepad(pad);
    }
    if (available_) {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }
}

bool Gamepads::handleDeviceEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        if (SDL_Gamepad* pad = SDL_OpenGamepad(event.gdevice.which)) {
            pads_.push_back(pad);
            std::cout << "Gamepad connected: " << SDL_GetGamepadName(pad) << "\n";
        }
        return true;
    }
    if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        const auto gone = std::find_if(pads_.begin(), pads_.end(), [&](SDL_Gamepad* pad) {
            return SDL_GetGamepadID(pad) == event.gdevice.which;
        });
        if (gone != pads_.end()) {
            SDL_CloseGamepad(*gone);
            pads_.erase(gone);
        }
        return true;
    }
    return false;
}

std::optional<ui::MenuInput> Gamepads::menuInputForButton(Uint8 button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return ui::MenuInput::Up;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return ui::MenuInput::Down;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return ui::MenuInput::Left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return ui::MenuInput::Right;
    case SDL_GAMEPAD_BUTTON_SOUTH:
    case SDL_GAMEPAD_BUTTON_START: return ui::MenuInput::Confirm;
    case SDL_GAMEPAD_BUTTON_EAST: return ui::MenuInput::Back;
    default: return std::nullopt;
    }
}

PadAction Gamepads::actionForButton(Uint8 button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return PadAction::Jump;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return PadAction::PreviousWeapon;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return PadAction::NextWeapon;
    case SDL_GAMEPAD_BUTTON_NORTH: return PadAction::EndTurn;
    case SDL_GAMEPAD_BUTTON_BACK: return PadAction::SwitchHog;
    case SDL_GAMEPAD_BUTTON_START: return PadAction::Pause;
    default: return PadAction::None;
    }
}

float Gamepads::strongestAxis(SDL_GamepadAxis axis) const {
    // With several pads, the one pushed furthest wins.
    float strongest = 0.0f;
    for (SDL_Gamepad* pad : pads_) {
        const float value = static_cast<float>(SDL_GetGamepadAxis(pad, axis)) * kAxisScale;
        if (std::abs(value) > std::abs(strongest)) strongest = value;
    }
    return std::abs(strongest) < kDeadZone ? 0.0f : strongest;
}

bool Gamepads::anyButton(SDL_GamepadButton button) const {
    return std::any_of(pads_.begin(), pads_.end(), [button](SDL_Gamepad* pad) { return SDL_GetGamepadButton(pad, button); });
}

void Gamepads::addHeldControls(game::PlayerInput& input) const {
    if (pads_.empty()) return;
    // Stick up is negative in SDL.
    const float moveY = strongestAxis(SDL_GAMEPAD_AXIS_LEFTY);
    const float moveX = strongestAxis(SDL_GAMEPAD_AXIS_LEFTX);
    input.movement.forward |= moveY < 0.0f || anyButton(SDL_GAMEPAD_BUTTON_DPAD_UP);
    input.movement.backward |= moveY > 0.0f || anyButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    input.movement.turnLeft |= moveX < 0.0f || anyButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    input.movement.turnRight |= moveX > 0.0f || anyButton(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

    const float aim = strongestAxis(SDL_GAMEPAD_AXIS_RIGHTY);
    input.aimUp |= aim < 0.0f;
    input.aimDown |= aim > 0.0f;
    input.fire |= anyButton(SDL_GAMEPAD_BUTTON_WEST) || strongestAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 0.0f;
}

float Gamepads::cameraTurn() const {
    return strongestAxis(SDL_GAMEPAD_AXIS_RIGHTX);
}

} // namespace openhogs::gameapp
