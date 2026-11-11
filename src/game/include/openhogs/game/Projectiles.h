// Projectiles.h
//
// Everything a weapon sends flying or puts down: grenades, shells,
// rockets, bomblets, TNT and mines.
//
// stepProjectile() moves one projectile by one simulation step and says
// whether it keeps going, explodes or is gone (fell in water, left the
// map). The world then makes the explosions happen (see Combat.h).

#pragma once

#include "openhogs/game/GameEvents.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/Weapons.h"

#include <glm/glm.hpp>

#include <vector>

namespace openhogs::game {

// Mines only go off after this many seconds, so the hog that laid one can
// walk away.
inline constexpr float kMineArmingSeconds = 2.0f;

struct Projectile {
    WeaponId weapon = WeaponId::Grenade;
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float fuseSecondsLeft = 0.0f;  // thrown and placed: explodes at 0 (0 at start: no fuse)
    float ageSeconds = 0.0f;
    std::size_t owner = 0;         // the hog that fired it
    bool bomblet = false;          // a piece of a cluster grenade (does not split again)
    bool resting = false;          // lying on the ground
};

enum class ProjectileOutcome { Flying, Explodes, Gone };

// Creates the projectile a hog launches with `weapon` from `muzzle` along
// `direction` (unit vector) at `power` (0 to 1; ignored by weapons that do
// not use power).
Projectile launchProjectile(WeaponId weapon, std::size_t owner, const glm::vec3& muzzle,
                            const glm::vec3& direction, float power);

// Moves a projectile by one step. Shells explode on touching anything;
// grenades bounce and explode when their fuse runs out; TNT waits for its
// fuse; mines wait for a living hog to come near once armed. Anything that
// falls into water or leaves the map is gone.
ProjectileOutcome stepProjectile(Projectile& projectile, double seconds, const MapCollision& map,
                                 const std::vector<Hog>& hogs, std::vector<GameEvent>& events);

// The path a projectile launched this way would follow until it hits the
// ground or an obstacle, as points `secondsPerPoint` apart (at most
// `maxPoints`). Used to show the player where a throw will land.
std::vector<glm::vec3> predictTrajectory(const glm::vec3& start, const glm::vec3& velocity, float gravityFactor,
                                         const MapCollision& map, float secondsPerPoint = 0.05f,
                                         int maxPoints = 80);

} // namespace openhogs::game
