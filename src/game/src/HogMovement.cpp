// HogMovement.cpp - see HogMovement.h for an overview.

#include "openhogs/game/HogMovement.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace openhogs::game {

namespace {

using Repeat = AnimationPlayer::Repeat;

// Two footsteps per walk or swim cycle: when it starts and halfway.
void countFootsteps(const Hog& hog, const std::vector<formats::Animation>& animations, MovementEvents& events) {
    const auto animation = hog.animation.animation();
    if (animation >= animations.size()) {
        return;
    }
    const auto frames = static_cast<float>(animations[animation].frames.size());
    if (hog.animation.passedFrame(0.0f)) ++events.footsteps;
    if (hog.animation.passedFrame(frames / 2.0f)) ++events.footsteps;
}

float horizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
}

// True if moving the hog `self` to `feet` would push it into another hog.
// Moving away from a hog is always allowed: some multiplayer maps place
// hogs on top of each other, and they must be able to step apart.
bool hitsOtherHog(const std::vector<Hog>& hogs, std::size_t self, const glm::vec3& feet) {
    const glm::vec3& from = hogs[self].position;
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        if (i == self || !hogs[i].alive) continue;  // the dead are no obstacle
        const glm::vec3& other = hogs[i].position;
        const bool sameLevel = std::abs(other.y - feet.y) < kHogHeight;
        const float after = horizontalDistance(other, feet);
        if (sameLevel && after < 2.0f * kHogRadius && after < horizontalDistance(other, from)) {
            return true;
        }
    }
    return false;
}

// Where a hog walking from `from` by `step` (horizontal) ends up, or
// `from` if the way is blocked: a slope too steep, an obstacle, another
// hog. If the full step is blocked, each of its two components is tried
// alone, so a hog walking at an angle into a wall slides along it.
// Whether a point is inside the play area of the controlled hog
// (kPlayAreaHalfSize around the centre of the map).
bool insidePlayArea(const MapCollision& map, float x, float z) {
    const auto& terrain = map.terrain();
    const float centreX = (terrain.minX() + terrain.maxX()) / 2.0f;
    const float centreZ = (terrain.minZ() + terrain.maxZ()) / 2.0f;
    return std::abs(x - centreX) <= kPlayAreaHalfSize && std::abs(z - centreZ) <= kPlayAreaHalfSize;
}

glm::vec3 walk(const std::vector<Hog>& hogs, std::size_t self, const glm::vec3& from, const glm::vec3& step,
               const MapCollision& map, bool keepInPlayArea) {
    const auto tryStep = [&](const glm::vec3& move, glm::vec3& result) {
        glm::vec3 to = from + move;
        if (!map.terrain().contains(to.x, to.z)) {
            return false;  // the edge of the map
        }
        if (keepInPlayArea && !insidePlayArea(map, to.x, to.z)) {
            return false;
        }
        const float ground = map.groundHeight(to.x, to.z, from.y);
        if (ground > from.y + kMaxStepHeight) {
            return false;  // a step too high
        }
        // Walking on the terrain (not on a box): slopes and wall tiles.
        const bool onTerrainBefore = std::abs(from.y - map.terrain().heightAt(from.x, from.z)) < 1.0f;
        const bool onTerrainAfter = ground == map.terrain().heightAt(to.x, to.z);
        if (onTerrainBefore && onTerrainAfter && !map.canWalkTerrain(from, to)) {
            return false;
        }
        // Stepping down a small height follows the ground; a bigger drop
        // is left to the caller, who makes the hog fall.
        to.y = ground >= from.y - kMaxStepHeight ? ground : from.y;
        if (map.hitsObstacle(to) || hitsOtherHog(hogs, self, to)) {
            return false;
        }
        result = to;
        return true;
    };

    glm::vec3 result = from;
    if (tryStep(step, result)) return result;
    if (tryStep({step.x, 0.0f, 0.0f}, result)) return result;
    if (tryStep({0.0f, 0.0f, step.z}, result)) return result;
    return from;
}

void startFall(Hog& hog, const glm::vec3& horizontalVelocity) {
    hog.motion.state = MotionState::Airborne;
    hog.motion.velocity = horizontalVelocity;
    hog.motion.highestPoint = hog.position.y;
    hog.motion.jumped = false;
    hog.animation.play(kFallAnimation, Repeat::Loop);
}

void moveInAir(std::vector<Hog>& hogs, std::size_t self, double seconds, const MapCollision& map,
               bool keepInPlayArea, MovementEvents& events) {
    Hog& hog = hogs[self];
    const auto dt = static_cast<float>(seconds);
    hog.motion.velocity.y -= kGravity * dt;

    // Horizontal part first: a wall stops the forward motion, not the fall.
    glm::vec3 next = hog.position + glm::vec3(hog.motion.velocity.x, 0.0f, hog.motion.velocity.z) * dt;
    // The player's own jump stays in the play area; a hog thrown by a blast
    // may fly anywhere (off the map, it falls to its death).
    const bool outOfBounds = keepInPlayArea && hog.motion.jumped && !insidePlayArea(map, next.x, next.z);
    if (!map.terrain().contains(next.x, next.z) || outOfBounds || map.hitsObstacle(next) ||
        hitsOtherHog(hogs, self, next)) {
        next = hog.position;
        hog.motion.velocity.x = 0.0f;
        hog.motion.velocity.z = 0.0f;
    }
    events.distanceMoved = glm::length(glm::vec2(next.x - hog.position.x, next.z - hog.position.z));

    next.y += hog.motion.velocity.y * dt;
    hog.motion.highestPoint = std::max(hog.motion.highestPoint, next.y);

    if (hog.motion.jumped && hog.animation.animation() == kJumpStartAnimation && hog.animation.finished()) {
        hog.animation.play(kJumpMiddleAnimation, Repeat::Loop);
    }

    const float ground = map.groundHeight(next.x, next.z, hog.position.y);
    if (hog.motion.velocity.y <= 0.0f && next.y <= ground) {
        next.y = ground;
        events.landed = true;
        events.fallHeight = hog.motion.highestPoint - ground;
        hog.motion.velocity = glm::vec3(0.0f);
        if (hog.alive) {
            hog.motion.state = MotionState::Landing;
            hog.animation.play(kJumpEndAnimation, Repeat::Once);
        } else {
            // Killed in the air: it dies where it lands (sinking in water).
            hog.motion.state = MotionState::Dead;
            hog.position = next;
            const bool inWater = isInWater(hog, map);
            hog.animation.play(inWater ? kDrowningAnimation : kFirstDyingAnimation + self % 3, Repeat::Once);
        }
    }
    hog.position = next;
}

} // namespace

std::string_view footstepSoundName(formats::SurfaceType surface) {
    using formats::SurfaceType;
    switch (surface) {
    case SurfaceType::Mud: return "FT_MUD";
    case SurfaceType::Grass: return "FT_GRASS";
    case SurfaceType::Metal: return "FT_METAL";
    case SurfaceType::Wood: return "FT_WOOD";
    case SurfaceType::Water: return "FT_WATER";
    case SurfaceType::Stone: return "FT_STONE";
    case SurfaceType::Rock: return "FT_ROCK";
    case SurfaceType::Sand: return "FT_SAND";
    case SurfaceType::Ice: return "FT_ICE";
    case SurfaceType::Snow: return "FT_SNOW";
    case SurfaceType::Quagmire: return "FT_QUAG";
    case SurfaceType::Lava: return "FT_LAVA";
    }
    return "FT_GRASS";  // unknown surface numbers in a replaced map
}

bool isInWater(const Hog& hog, const MapCollision& map) {
    return map.terrain().tileAt(hog.position.x, hog.position.z).isWater;
}

MovementEvents moveHog(std::vector<Hog>& hogs, std::size_t hogIndex, const MovementInput& input, bool canMove,
                       double seconds, const MapCollision& map, const std::vector<formats::Animation>& animations) {
    MovementEvents events;
    Hog& hog = hogs[hogIndex];
    const auto dt = static_cast<float>(seconds);
    const bool wasInWater = isInWater(hog, map);

    hog.animation.update(seconds, animations);

    if (hog.motion.state == MotionState::Dead) {
        // Lies still once its dying animation has played.
    } else if (hog.motion.state == MotionState::Airborne) {
        moveInAir(hogs, hogIndex, seconds, map, input.keepInPlayArea, events);
    } else if (hog.motion.state == MotionState::Landing) {
        if (hog.animation.finished()) {
            hog.motion.state = MotionState::Standing;
        }
    } else {
        // On the ground: turning is always allowed.
        const int turn = (input.turnLeft ? 1 : 0) - (input.turnRight ? 1 : 0);
        hog.yawRadians += static_cast<float>(turn) * glm::radians(kTurnDegreesPerSecond) * dt;
        hog.yawRadians = std::remainder(hog.yawRadians, glm::two_pi<float>());

        const bool inWater = wasInWater;
        const int direction = canMove ? (input.forward ? 1 : 0) - (input.backward ? 1 : 0) : 0;

        // No jump out of water, nor off a wall tile (the executable: "Can't
        // jump from this tile type").
        const bool onWall = map.terrain().tileAt(hog.position.x, hog.position.z).isWall;
        if (canMove && input.jump && !inWater && !onWall) {
            hog.motion.state = MotionState::Airborne;
            hog.motion.jumped = true;
            hog.motion.highestPoint = hog.position.y;
            const float forward = input.forward ? kJumpForwardSpeed : 0.0f;
            hog.motion.velocity = hogFacing(hog) * forward + glm::vec3(0.0f, kJumpUpSpeed, 0.0f);
            hog.animation.play(kJumpStartAnimation, Repeat::Once);
            events.tookOff = true;
        } else if (direction != 0) {
            float speed = direction > 0 ? kRunSpeed : kWalkBackSpeed;
            if (inWater) speed *= kSwimSpeedFactor;
            const glm::vec3 step = hogFacing(hog) * (speed * dt * static_cast<float>(direction));
            const glm::vec3 before = hog.position;
            hog.position = walk(hogs, hogIndex, before, step, map, input.keepInPlayArea);
            events.distanceMoved = glm::length(glm::vec2(hog.position.x - before.x, hog.position.z - before.z));

            const bool blocked = events.distanceMoved < 0.01f;
            if (blocked && !inWater) {
                // Against a wall: stand instead of running on the spot.
                hog.motion.state = MotionState::Standing;
            } else {
                hog.motion.state = inWater ? MotionState::Swimming
                                           : (direction > 0 ? MotionState::Running : MotionState::WalkingBack);
                const std::size_t animation =
                    inWater ? kSwimAnimation : (direction > 0 ? kRunAnimation : kWalkBackAnimation);
                hog.animation.play(animation, Repeat::Loop);
                countFootsteps(hog, animations, events);
            }

            // Walked off an edge: fall, keeping the walking speed.
            const float ground = map.groundHeight(hog.position.x, hog.position.z, hog.position.y);
            if (ground < hog.position.y - kMaxStepHeight) {
                const glm::vec3 moved = hog.position - before;
                startFall(hog, dt > 0.0f ? glm::vec3(moved.x, 0.0f, moved.z) / dt : glm::vec3(0.0f));
            }
        } else if (inWater) {
            hog.motion.state = MotionState::Swimming;
            hog.animation.play(kSwimAnimation, Repeat::Loop);
        } else if (turn != 0) {
            hog.motion.state = MotionState::Turning;
            hog.animation.play(kTurnAnimation, Repeat::Loop);
        } else {
            hog.motion.state = MotionState::Standing;
        }
    }

    // Standing still: the held animation (aiming, firing) if there is one,
    // otherwise the idle behaviour chooses.
    if (hog.motion.state == MotionState::Standing) {
        if (hog.held && hog.held->repeat == Repeat::Hold) {
            hog.animation.hold(hog.held->animation, hog.held->frame);
        } else if (hog.held) {
            hog.animation.play(hog.held->animation, hog.held->repeat);
        } else {
            hog.idle.update(hog.animation, animations);
        }
    }

    const bool inWaterNow = isInWater(hog, map);
    events.enteredWater = inWaterNow && !wasInWater;
    events.surface = map.terrain().tileAt(hog.position.x, hog.position.z).surface;
    return events;
}

} // namespace openhogs::game
