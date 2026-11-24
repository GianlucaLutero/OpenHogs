// Fog.h
//
// Distance fog: the scene fades into `colour` between `start` and `end`
// world units from the camera, as the original's afSetFog (linear fog,
// set per sky by FUN_00485550). A fog whose end is not past its start is
// no fog at all, which is the default.

#pragma once

#include <glm/glm.hpp>

namespace openhogs::render {

struct Fog {
    glm::vec3 colour{0.0f};  // 0..1
    float start = 0.0f;
    float end = 0.0f;

    bool enabled() const { return end > start; }
};

} // namespace openhogs::render
