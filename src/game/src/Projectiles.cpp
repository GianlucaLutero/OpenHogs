// Projectiles.cpp - see Projectiles.h for an overview.

#include "openhogs/game/Projectiles.h"

#include "openhogs/game/HogMovement.h"

#include <algorithm>
#include <cmath>

namespace openhogs::game {

namespace {

// A shell does not hit the hog that fired it during its first moments
// (it starts inside its reach).
constexpr float kOwnerSafeSeconds = 0.3f;

// Below this vertical speed a bouncing grenade stops bouncing and rolls.
constexpr float kRestingSpeed = 80.0f;
// Share of its horizontal speed a grenade keeps at each bounce, and while
// rolling each second.
constexpr float kBounceFriction = 0.7f;
constexpr float kRollingFrictionPerSecond = 0.1f;
// Bounces slower than this make no sound.
constexpr float kAudibleBounceSpeed = 200.0f;

// Anything this far below the lowest point of the map is gone.
constexpr float kLowestHeight = -20000.0f;

bool touchesHog(const Projectile& projectile, const std::vector<Hog>& hogs) {
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        const Hog& hog = hogs[i];
        if (!hog.alive || (i == projectile.owner && projectile.ageSeconds < kOwnerSafeSeconds)) {
            continue;
        }
        const glm::vec3& p = projectile.position;
        const float dx = p.x - hog.position.x;
        const float dz = p.z - hog.position.z;
        if (dx * dx + dz * dz < kHogRadius * kHogRadius && p.y >= hog.position.y &&
            p.y <= hog.position.y + kHogHeight) {
            return true;
        }
    }
    return false;
}

bool insideObstacle(const MapCollision& map, const glm::vec3& point) {
    for (const auto& box : map.boxes()) {
        if (point.y >= box.bottom() && point.y <= box.top() && box.footprintDistance(point.x, point.z) == 0.0f) {
            return true;
        }
    }
    return false;
}

// Height a projectile rests on: the terrain or the top of a box under it.
float floorUnder(const MapCollision& map, const glm::vec3& point) {
    return map.groundHeight(point.x, point.z, point.y);
}

void report(std::vector<GameEvent>& events, GameEventType type, const Projectile& projectile) {
    GameEvent event;
    event.type = type;
    event.position = projectile.position;
    event.weapon = projectile.weapon;
    events.push_back(event);
}

ProjectileOutcome stepPlaced(Projectile& projectile, const WeaponSpec& spec, const std::vector<Hog>& hogs) {
    if (spec.triggerRadius > 0.0f) {
        if (projectile.ageSeconds < kMineArmingSeconds) {
            return ProjectileOutcome::Flying;
        }
        for (const auto& hog : hogs) {
            if (hog.alive && glm::length(hog.position - projectile.position) < spec.triggerRadius) {
                return ProjectileOutcome::Explodes;
            }
        }
    }
    return ProjectileOutcome::Flying;
}

} // namespace

Projectile launchProjectile(WeaponId weapon, std::size_t owner, const glm::vec3& muzzle,
                            const glm::vec3& direction, float power) {
    const WeaponSpec& spec = weaponSpec(weapon);
    Projectile projectile;
    projectile.weapon = weapon;
    projectile.owner = owner;
    projectile.position = muzzle;
    projectile.fuseSecondsLeft = spec.fuseSeconds;
    const float speed = spec.launchSpeed * (spec.usesPower ? std::clamp(power, 0.0f, 1.0f) : 1.0f);
    projectile.velocity = direction * speed;
    projectile.resting = spec.kind == WeaponKind::Placed;
    return projectile;
}

ProjectileOutcome stepProjectile(Projectile& projectile, double seconds, const MapCollision& map,
                                 const std::vector<Hog>& hogs, std::vector<GameEvent>& events) {
    const WeaponSpec& spec = weaponSpec(projectile.weapon);
    const auto dt = static_cast<float>(seconds);
    projectile.ageSeconds += dt;

    // Fuses burn whatever the projectile is doing.
    if (projectile.fuseSecondsLeft > 0.0f) {
        projectile.fuseSecondsLeft -= dt;
        if (projectile.fuseSecondsLeft <= 0.0f) {
            return ProjectileOutcome::Explodes;
        }
    }
    if (spec.kind == WeaponKind::Placed) {
        return stepPlaced(projectile, spec, hogs);
    }

    const glm::vec3 previous = projectile.position;
    if (projectile.resting) {
        // Rolling to a stop on the ground.
        projectile.velocity *= std::pow(kRollingFrictionPerSecond, dt);
        projectile.velocity.y = 0.0f;
    } else {
        projectile.velocity.y -= kGravity * spec.gravityFactor * dt;
    }
    projectile.position += projectile.velocity * dt;
    glm::vec3& p = projectile.position;

    if (!map.terrain().contains(p.x, p.z) || p.y < kLowestHeight) {
        return ProjectileOutcome::Gone;
    }
    const bool isShell = spec.kind == WeaponKind::Shell;

    if (touchesHog(projectile, hogs)) {
        if (isShell) return ProjectileOutcome::Explodes;
        // A grenade bounces off a hog.
        p = previous;
        projectile.velocity.x *= -spec.bounciness;
        projectile.velocity.z *= -spec.bounciness;
    }
    if (insideObstacle(map, p)) {
        if (isShell) return ProjectileOutcome::Explodes;
        p = previous;
        projectile.velocity.x *= -spec.bounciness;
        projectile.velocity.z *= -spec.bounciness;
    }

    const float floor = floorUnder(map, p);
    if (p.y > floor) {
        if (projectile.resting && p.y > floor + kMaxStepHeight) {
            projectile.resting = false;  // rolled off an edge
        }
        if (projectile.resting) p.y = floor;
        return ProjectileOutcome::Flying;
    }

    // Reached the ground.
    if (map.terrain().tileAt(p.x, p.z).isWater) {
        report(events, GameEventType::Splash, projectile);
        return ProjectileOutcome::Gone;
    }
    if (isShell) {
        return ProjectileOutcome::Explodes;
    }
    p.y = floor;
    if (!projectile.resting && projectile.velocity.y < 0.0f) {
        if (-projectile.velocity.y > kAudibleBounceSpeed) {
            report(events, GameEventType::Bounce, projectile);
        }
        projectile.velocity.y = -projectile.velocity.y * spec.bounciness;
        projectile.velocity.x *= kBounceFriction;
        projectile.velocity.z *= kBounceFriction;
        if (projectile.velocity.y < kRestingSpeed) {
            projectile.velocity.y = 0.0f;
            projectile.resting = true;
        }
    }
    return ProjectileOutcome::Flying;
}

std::vector<glm::vec3> predictTrajectory(const glm::vec3& start, const glm::vec3& velocity, float gravityFactor,
                                         const MapCollision& map, float secondsPerPoint, int maxPoints) {
    std::vector<glm::vec3> points;
    glm::vec3 position = start;
    glm::vec3 speed = velocity;
    // Several small steps between points, so fast shots do not skip
    // through thin obstacles.
    constexpr int kStepsPerPoint = 5;
    const float dt = secondsPerPoint / kStepsPerPoint;
    for (int point = 0; point < maxPoints; ++point) {
        for (int step = 0; step < kStepsPerPoint; ++step) {
            speed.y -= kGravity * gravityFactor * dt;
            position += speed * dt;
            const bool stopped = !map.terrain().contains(position.x, position.z) ||
                                 position.y <= map.groundHeight(position.x, position.z, position.y) ||
                                 insideObstacle(map, position);
            if (stopped) {
                points.push_back(position);
                return points;
            }
        }
        points.push_back(position);
    }
    return points;
}

} // namespace openhogs::game
