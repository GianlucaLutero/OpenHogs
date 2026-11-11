// GameEvents.h
//
// Things that happen during a simulation step and that the application
// shows or plays: a footstep, a shot, an explosion, a hog getting hurt...
// The game module only reports them; it does not know about sounds or
// effects.

#pragma once

#include "openhogs/formats/MapTerrain.h"
#include "openhogs/game/Weapons.h"

#include <glm/glm.hpp>

#include <cstddef>

namespace openhogs::game {

enum class GameEventType {
    Footstep,   // surface: what the hog walks on
    Landed,     // after a jump or a fall
    Splash,     // something went into water
    Fired,      // weapon: what was fired, position: where from
    Bounce,     // a grenade hit the ground
    BulletHit,  // position: where the bullet stopped
    Explosion,  // weapon: what exploded
    Hurt,       // hog: who; amount: how much health it lost
    Died,       // hog: who
    TurnStarted,
    MatchOver,  // team: the winner, or -1 for a draw
};

struct GameEvent {
    GameEventType type = GameEventType::Footstep;
    glm::vec3 position{0.0f};
    formats::SurfaceType surface = formats::SurfaceType::Grass;
    WeaponId weapon = WeaponId::Trotter;
    std::size_t hog = 0;
    float amount = 0.0f;
    int team = -1;
};

} // namespace openhogs::game
