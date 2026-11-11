// ChaseCamera.h
//
// The camera of the game: it orbits around a target (the selected hog),
// follows it smoothly and never goes under the ground.
//
// The player turns the camera around the target and zooms; the camera
// itself glides towards a new target instead of jumping, so switching hog
// reads as a movement across the map.
//
// The result is a plain position and two angles, with the convention of
// render::Camera (yaw 0 looks towards -Z, positive pitch looks up), so this
// file needs no drawing code and can be unit tested.

#pragma once

#include "openhogs/game/TerrainHeightField.h"

#include <glm/glm.hpp>

namespace openhogs::game {

struct CameraPose {
    glm::vec3 position{0.0f};
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
};

class ChaseCamera {
public:
    // Turns the camera around the target. Pitch is limited so the camera
    // stays between a low angle and almost straight above.
    void orbit(float yawDeltaDegrees, float pitchDeltaDegrees);

    // Multiplies the distance to the target (below 1 = closer), within limits.
    void zoom(float factor);

    // Points the camera along a direction on the ground (e.g. the way a hog
    // faces), keeping the current pitch.
    void lookAlong(const glm::vec3& direction);

    // Jumps to the target at once (used when a map starts).
    void snapTo(const glm::vec3& target);

    // Glides towards the target; call once per simulation step.
    void update(double seconds, const glm::vec3& target);

    // Where the camera is and where it looks, kept above the terrain.
    CameraPose pose(const TerrainHeightField& terrain) const;

    float distance() const { return distance_; }
    const glm::vec3& focus() const { return focus_; }

private:
    glm::vec3 focus_{0.0f};       // the point looked at; trails the target
    float yawDegrees_ = 0.0f;     // direction the camera looks in
    float pitchDegrees_ = -20.0f; // negative: looking down at the target
    float distance_ = 900.0f;     // world units
};

} // namespace openhogs::game
