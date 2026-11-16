// RifleSight.cpp - see RifleSight.h for an overview.

#include "openhogs/ui/RifleSight.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace openhogs::ui {

glm::vec2 sightCentre(float canvasWidth) {
    return {canvasWidth / 2.0f, kSightCentreY};
}

std::vector<glm::vec2> sightMaskTriangles(float canvasWidth, int segments) {
    // A ring from the edge of the hole out to a circle beyond every corner
    // of the canvas, cut into quads of two triangles.
    const glm::vec2 centre = sightCentre(canvasWidth);
    const float outer = canvasWidth + 480.0f;
    std::vector<glm::vec2> triangles;
    triangles.reserve(static_cast<std::size_t>(segments) * 6);
    for (int i = 0; i < segments; ++i) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
        const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);
        const glm::vec2 d0{std::cos(a0), std::sin(a0)};
        const glm::vec2 d1{std::cos(a1), std::sin(a1)};
        const glm::vec2 in0 = centre + d0 * kSightRadius;
        const glm::vec2 in1 = centre + d1 * kSightRadius;
        const glm::vec2 out0 = centre + d0 * outer;
        const glm::vec2 out1 = centre + d1 * outer;
        triangles.insert(triangles.end(), {in0, out0, out1, in0, out1, in1});
    }
    return triangles;
}

glm::vec2 crosshairCorner(float canvasWidth) {
    return {canvasWidth / 2.0f - kCrosshairSize / 2.0f, 240.0f - kCrosshairSize / 2.0f};
}

} // namespace openhogs::ui
