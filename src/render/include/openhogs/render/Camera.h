// Camera.h
//
// A free-flying camera, as used by map editors: it has a position and two
// angles (yaw = turning left/right, pitch = looking up/down), and moves
// relative to the direction it is looking at.

#pragma once

#include <glm/glm.hpp>

namespace openhogs::render {

class Camera {
public:
    glm::vec3 position{0.0f};
    float yawDegrees = 0.0f;     // 0 = looking towards -Z
    float pitchDegrees = 0.0f;   // positive = looking up, limited to +/-89
    float fieldOfViewDegrees = 60.0f;

    // Direction the camera looks at (unit vector).
    glm::vec3 forward() const;
    // Unit vector pointing to the camera's right, parallel to the ground.
    glm::vec3 right() const;

    // Moves relative to the view: `amount.x` right, `amount.y` up (world
    // up), `amount.z` forward.
    void move(const glm::vec3& amount);

    // Turns the camera by the given angles. Pitch is clamped so the camera
    // can never flip over.
    void turn(float yawDeltaDegrees, float pitchDeltaDegrees);

    // Places the camera above one corner of the given box, looking at its
    // centre, so the whole map is in view.
    void frame(const glm::vec3& boxMin, const glm::vec3& boxMax);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;
};

} // namespace openhogs::render
