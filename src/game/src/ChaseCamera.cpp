// ChaseCamera.cpp - see ChaseCamera.h for an overview.

#include "openhogs/game/ChaseCamera.h"

#include <algorithm>
#include <cmath>

namespace openhogs::game {

namespace {

constexpr float kMinPitchDegrees = -85.0f;  // almost straight down
constexpr float kMaxPitchDegrees = 10.0f;   // slightly from below
constexpr float kMinDistance = 300.0f;
constexpr float kMaxDistance = 6000.0f;

// How quickly the focus closes in on the target: each second the remaining
// gap shrinks to e^-6 (0.25%), i.e. a new hog is reached in about a second.
constexpr double kFollowRate = 6.0;

// How quickly turnTowards() swings the camera: the remaining angle
// shrinks to e^-2.5 (8%) each second, slower than the hog turns, so the
// camera follows smoothly instead of copying every small turn.
constexpr double kSwingRate = 2.5;

// The camera stays at least this far above the ground under it.
constexpr float kGroundClearance = 60.0f;

// Unit vector for a yaw and pitch, same convention as render::Camera.
glm::vec3 direction(float yawDegrees, float pitchDegrees) {
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

} // namespace

void ChaseCamera::orbit(float yawDeltaDegrees, float pitchDeltaDegrees) {
    yawDegrees_ = std::fmod(yawDegrees_ + yawDeltaDegrees, 360.0f);
    pitchDegrees_ = std::clamp(pitchDegrees_ + pitchDeltaDegrees, kMinPitchDegrees, kMaxPitchDegrees);
}

void ChaseCamera::zoom(float factor) {
    distance_ = std::clamp(distance_ * factor, kMinDistance, kMaxDistance);
}

void ChaseCamera::lookAlong(const glm::vec3& along) {
    if (along.x == 0.0f && along.z == 0.0f) {
        return;  // straight up or down: no direction on the ground
    }
    yawDegrees_ = glm::degrees(std::atan2(along.x, -along.z));
}

void ChaseCamera::turnTowards(const glm::vec3& along, double seconds) {
    if (along.x == 0.0f && along.z == 0.0f) {
        return;
    }
    const float wanted = glm::degrees(std::atan2(along.x, -along.z));
    // Shortest signed difference, in -180..180 degrees.
    const float difference = std::remainder(wanted - yawDegrees_, 360.0f);
    const auto share = static_cast<float>(1.0 - std::exp(-kSwingRate * seconds));
    yawDegrees_ = std::fmod(yawDegrees_ + difference * share, 360.0f);
}

void ChaseCamera::snapTo(const glm::vec3& target) {
    focus_ = target;
}

void ChaseCamera::update(double seconds, const glm::vec3& target) {
    // Exponential approach: the same share of the gap is closed in the same
    // time whatever the step length, so the glide does not depend on it.
    const auto share = static_cast<float>(1.0 - std::exp(-kFollowRate * seconds));
    focus_ += (target - focus_) * share;
}

CameraPose ChaseCamera::pose(const TerrainHeightField& terrain) const {
    CameraPose result;
    result.position = focus_ - direction(yawDegrees_, pitchDegrees_) * distance_;
    result.yawDegrees = yawDegrees_;
    result.pitchDegrees = pitchDegrees_;

    const float lowest = terrain.heightAt(result.position.x, result.position.z) + kGroundClearance;
    if (result.position.y < lowest) {
        // A hill is in the way: lift the camera over it and tilt it down
        // so it still looks at the focus.
        result.position.y = lowest;
        const glm::vec3 toFocus = glm::normalize(focus_ - result.position);
        result.pitchDegrees = glm::degrees(std::asin(std::clamp(toFocus.y, -1.0f, 1.0f)));
    }
    return result;
}

} // namespace openhogs::game
