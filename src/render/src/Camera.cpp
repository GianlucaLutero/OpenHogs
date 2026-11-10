// Camera.cpp - see Camera.h for an overview.

#include "openhogs/render/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace openhogs::render {

namespace {

constexpr float kMaxPitchDegrees = 89.0f;

// The maps are 32768 units wide: the far plane must cover the whole map,
// the near plane is kept large enough to avoid depth precision problems.
constexpr float kNearPlane = 16.0f;
constexpr float kFarPlane = 100000.0f;

} // namespace

glm::vec3 Camera::forward() const {
    const float yaw = glm::radians(yawDegrees);
    const float pitch = glm::radians(pitchDegrees);
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

glm::vec3 Camera::right() const {
    const float yaw = glm::radians(yawDegrees);
    return {std::cos(yaw), 0.0f, std::sin(yaw)};
}

void Camera::move(const glm::vec3& amount) {
    position += right() * amount.x;
    position += glm::vec3(0.0f, 1.0f, 0.0f) * amount.y;
    position += forward() * amount.z;
}

void Camera::turn(float yawDeltaDegrees, float pitchDeltaDegrees) {
    yawDegrees = std::fmod(yawDegrees + yawDeltaDegrees, 360.0f);
    pitchDegrees = std::clamp(pitchDegrees + pitchDeltaDegrees, -kMaxPitchDegrees, kMaxPitchDegrees);
}

void Camera::frame(const glm::vec3& boxMin, const glm::vec3& boxMax) {
    const glm::vec3 centre = (boxMin + boxMax) * 0.5f;
    const glm::vec3 size = boxMax - boxMin;

    // Stand outside the box's +Z edge, high up, and look at the centre.
    position = {centre.x, boxMax.y + size.x * 0.35f, boxMax.z + size.z * 0.25f};
    const glm::vec3 toCentre = glm::normalize(centre - position);
    yawDegrees = glm::degrees(std::atan2(toCentre.x, -toCentre.z));
    pitchDegrees = glm::degrees(std::asin(toCentre.y));
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, position + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fieldOfViewDegrees), aspectRatio, kNearPlane, kFarPlane);
}

} // namespace openhogs::render
