// HogMotion.h
//
// The movement state of a hog: on the ground, in the air, swimming...
// Kept apart from HogMovement.h so that Hog can hold it without the
// movement code (see HogMovement.h for how it changes).

#pragma once

#include <glm/glm.hpp>

namespace openhogs::game {

enum class MotionState {
    Standing,     // on the ground, not moving
    Running,      // forwards
    WalkingBack,
    Turning,      // on the spot
    Swimming,     // in water, moving or not
    Airborne,     // jumping or falling
    Landing,      // the short recovery after a jump or a fall
};

struct HogMotion {
    MotionState state = MotionState::Standing;
    glm::vec3 velocity{0.0f};  // used while airborne
    float highestPoint = 0.0f; // feet height at the top of a jump or fall
    bool jumped = false;       // airborne because of a jump (not a fall)
};

} // namespace openhogs::game
