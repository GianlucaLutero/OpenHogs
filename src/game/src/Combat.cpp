// Combat.cpp - see Combat.h for an overview.

#include "openhogs/game/Combat.h"

#include "openhogs/game/HogMovement.h"

#include <algorithm>
#include <cmath>

namespace openhogs::game {

namespace {

// Bullets are followed in steps this long: shorter than a hog is wide.
constexpr float kBulletStep = 20.0f;

// Share of an explosion's knockback that goes upwards, so hogs fly in an
// arc instead of sliding along the ground.
constexpr float kKnockbackLift = 0.6f;

bool insideHog(const Hog& hog, const glm::vec3& point) {
    const float dx = point.x - hog.position.x;
    const float dz = point.z - hog.position.z;
    return dx * dx + dz * dz < kHogRadius * kHogRadius && point.y >= hog.position.y &&
           point.y <= hog.position.y + kHogHeight;
}

bool insideBox(const CollisionBox& box, const glm::vec3& point) {
    return point.y >= box.bottom() && point.y <= box.top() && box.footprintDistance(point.x, point.z) == 0.0f;
}

} // namespace

float fallDamage(float height) {
    return height <= kHarmlessFallHeight ? 0.0f : (height - kHarmlessFallHeight) / kFallHeightPerDamage;
}

void damageHog(std::vector<Hog>& hogs, std::size_t index, float amount, std::vector<GameEvent>& events) {
    Hog& hog = hogs[index];
    if (!hog.alive || amount <= 0.0f) {
        return;
    }
    const float lost = std::min(amount, hog.health);
    hog.health -= lost;
    GameEvent hurt;
    hurt.type = GameEventType::Hurt;
    hurt.position = hogCentre(hog);
    hurt.hog = index;
    hurt.amount = lost;
    events.push_back(hurt);

    if (hog.health > 0.0f) {
        return;
    }
    hog.alive = false;
    hog.held.reset();
    hog.charging = false;
    GameEvent died = hurt;
    died.type = GameEventType::Died;
    events.push_back(died);

    if (hog.motion.state != MotionState::Airborne) {
        // A hog that dies in the water sinks (the executable's animation 74).
        const bool inWater = hog.motion.state == MotionState::Swimming;
        hog.motion.state = MotionState::Dead;
        hog.animation.play(inWater ? kDrowningAnimation : kFirstDyingAnimation + index % 3,
                           AnimationPlayer::Repeat::Once);
    }
}

void knockHog(Hog& hog, const glm::vec3& velocity) {
    hog.motion.state = MotionState::Airborne;
    hog.motion.velocity = velocity;
    hog.motion.highestPoint = hog.position.y;
    hog.motion.jumped = false;
    // Thrown hogs leave the ground at once, so the landing check of the
    // next step does not catch them still standing.
    hog.position.y += 1.0f;
    hog.animation.play(kFallAnimation, AnimationPlayer::Repeat::Loop);
}

void explode(std::vector<Hog>& hogs, const glm::vec3& centre, float radius, float damage, float knockback,
             std::vector<GameEvent>& events) {
    if (radius <= 0.0f) {
        return;
    }
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        Hog& hog = hogs[i];
        if (!hog.alive) {
            continue;
        }
        const glm::vec3 offset = hogCentre(hog) - centre;
        const float distance = glm::length(offset);
        if (distance >= radius) {
            continue;
        }
        const float strength = 1.0f - distance / radius;  // 1 at the centre, 0 at the edge

        // Away from the centre along the ground, plus a lift.
        glm::vec3 away(offset.x, 0.0f, offset.z);
        away = glm::length(away) > 1.0f ? glm::normalize(away) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 push = (away * (1.0f - kKnockbackLift) + glm::vec3(0.0f, kKnockbackLift, 0.0f)) *
                               (knockback * strength);
        if (knockback > 0.0f) {
            knockHog(hog, push);
        }
        damageHog(hogs, i, damage * strength, events);
    }
}

BulletHit traceBullet(const glm::vec3& start, const glm::vec3& direction, float range, const MapCollision& map,
                      const std::vector<Hog>& hogs, std::size_t shooter) {
    BulletHit hit;
    for (float travelled = kBulletStep; travelled <= range; travelled += kBulletStep) {
        const glm::vec3 point = start + direction * travelled;
        hit.point = point;
        for (std::size_t i = 0; i < hogs.size(); ++i) {
            if (i != shooter && hogs[i].alive && insideHog(hogs[i], point)) {
                hit.hog = i;
                return hit;
            }
        }
        if (!map.terrain().contains(point.x, point.z) || point.y <= map.terrain().heightAt(point.x, point.z)) {
            return hit;
        }
        for (const auto& box : map.boxes()) {
            if (insideBox(box, point)) {
                return hit;
            }
        }
    }
    hit.point = start + direction * range;
    return hit;
}

} // namespace openhogs::game
