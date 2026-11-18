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

// A homing missile flies straight this long, then turns towards its prey
// at most this fast.
constexpr float kHomingDelaySeconds = 0.5f;
constexpr float kHomingDegreesPerSecond = 180.0f;
constexpr float kSteepestClimbDegrees = 80.0f;

// Anything this far below the lowest point of the map is gone.
constexpr float kLowestHeight = -20000.0f;

bool touchesHog(const Projectile& projectile, const std::vector<Hog>& hogs) {
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        const Hog& hog = hogs[i];
        if (!hog.alive || hog.sheltered() || (i == projectile.owner && projectile.ageSeconds < kOwnerSafeSeconds)) {
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

// The nearest living enemy of the projectile's owner, if any.
const Hog* nearestEnemy(const Projectile& projectile, const std::vector<Hog>& hogs) {
    if (projectile.owner >= hogs.size()) return nullptr;
    const int team = hogs[projectile.owner].team;
    const Hog* nearest = nullptr;
    float best = 0.0f;
    for (const auto& hog : hogs) {
        if (!hog.alive || hog.team == team || hog.hidden() || hog.sheltered()) continue;
        const float distance = glm::length(hog.position - projectile.position);
        if (!nearest || distance < best) {
            nearest = &hog;
            best = distance;
        }
    }
    return nearest;
}

// Turns `velocity` towards `wanted` by at most `maxRadians`, keeping its speed.
glm::vec3 turnTowards(const glm::vec3& velocity, const glm::vec3& wanted, float maxRadians) {
    const float speed = glm::length(velocity);
    if (speed < 1.0f || glm::length(wanted) < 1.0f) return velocity;
    const glm::vec3 from = velocity / speed;
    const glm::vec3 to = glm::normalize(wanted);
    const float angle = std::acos(std::clamp(glm::dot(from, to), -1.0f, 1.0f));
    if (angle <= maxRadians) return to * speed;
    // Spherical step from `from` towards `to`.
    const float t = maxRadians / angle;
    const float s = std::sin(angle);
    const glm::vec3 direction = (std::sin((1.0f - t) * angle) * from + std::sin(t * angle) * to) / s;
    return glm::normalize(direction) * speed;
}

} // namespace

bool explodesOnContact(const WeaponSpec& spec) {
    // Shells, bombs and a dropped jetpack burst where they hit.
    return spec.kind == WeaponKind::Shell || spec.kind == WeaponKind::Airstrike || spec.kind == WeaponKind::Jetpack;
}

void steerProjectile(Projectile& projectile, float yawRadians, float pitchRadians) {
    glm::vec3& v = projectile.velocity;
    const float speed = glm::length(v);
    if (speed < 1.0f) return;
    const float horizontal = glm::length(glm::vec2(v.x, v.z));
    float yaw = std::atan2(v.x, v.z) + yawRadians;
    float pitch = std::atan2(v.y, horizontal) + pitchRadians;
    const float steepest = glm::radians(kSteepestClimbDegrees);
    pitch = std::clamp(pitch, -steepest, steepest);
    v = glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)) * speed;
}

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
    if (spec.guidance == Guidance::Homing && projectile.ageSeconds > kHomingDelaySeconds) {
        if (const Hog* prey = nearestEnemy(projectile, hogs)) {
            const glm::vec3 aim = prey->position + glm::vec3(0.0f, kHogHeight / 2.0f, 0.0f) - projectile.position;
            projectile.velocity = turnTowards(projectile.velocity, aim, glm::radians(kHomingDegreesPerSecond) * dt);
        }
    }
    projectile.position += projectile.velocity * dt;
    glm::vec3& p = projectile.position;

    if (!map.terrain().contains(p.x, p.z) || p.y < kLowestHeight) {
        return ProjectileOutcome::Gone;
    }
    const bool isShell = explodesOnContact(spec);

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
