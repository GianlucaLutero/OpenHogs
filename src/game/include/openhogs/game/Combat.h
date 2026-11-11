// Combat.h
//
// Damage, death, explosions and bullets: how weapons affect hogs.
//
// All the numbers here are OpenHogs' own until the original's are known
// (docs/open-decisions.md).

#pragma once

#include "openhogs/game/GameEvents.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/MapCollision.h"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

namespace openhogs::game {

// Falls shorter than this do not hurt (a jump lands from about 170).
inline constexpr float kHarmlessFallHeight = 400.0f;
// Above that, one point of damage per this many units of height.
inline constexpr float kFallHeightPerDamage = 10.0f;

// Health lost when landing after falling `height` world units.
float fallDamage(float height);

// Takes `amount` of health from hog `index`, reporting Hurt (and Died when
// it reaches 0). A dead hog plays one of its dying animations; one still
// in the air finishes its fall first. Dead hogs take no more damage.
void damageHog(std::vector<Hog>& hogs, std::size_t index, float amount, std::vector<GameEvent>& events);

// Throws a hog through the air with `velocity` (knockback).
void knockHog(Hog& hog, const glm::vec3& velocity);

// An explosion at `centre`: every living hog within `radius` loses up to
// `damage` health (all of it at the centre, none at the edge) and is
// thrown away from the centre, up to `knockback` units per second.
void explode(std::vector<Hog>& hogs, const glm::vec3& centre, float radius, float damage, float knockback,
             std::vector<GameEvent>& events);

struct BulletHit {
    glm::vec3 point{0.0f};           // where the bullet stopped
    std::optional<std::size_t> hog;  // the hog it hit, if any
};

// Follows a bullet from `start` along `direction` (unit vector) for up to
// `range` units and returns the first thing it hits: a living hog (other
// than `shooter`), an obstacle box or the ground.
BulletHit traceBullet(const glm::vec3& start, const glm::vec3& direction, float range, const MapCollision& map,
                      const std::vector<Hog>& hogs, std::size_t shooter);

} // namespace openhogs::game
