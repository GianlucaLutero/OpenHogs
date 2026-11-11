// EffectsRenderer.h
//
// Draws the simple shapes of the game's effects on top of the scene:
//   - dots: the aiming line or arc, bullet tracers, flying shells;
//   - explosions: a ball that grows and fades from yellow to dark red;
//   - health bars over the hogs.
//
// They are plain coloured shapes for now; the original's effect sprites
// are still to be found (docs/open-decisions.md).

#pragma once

#include "openhogs/render/Camera.h"
#include "openhogs/render/GlObjects.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace openhogs::render {

class EffectsRenderer {
public:
    // Needs a current OpenGL context.
    EffectsRenderer();

    // Must be called before the draw functions of a frame.
    void begin(const Camera& camera, int width, int height) const;

    // A small cube of `size` world units centred on each point.
    // `alwaysVisible` draws them even behind other things (the aiming
    // line must not disappear behind the hog's own head).
    void drawDots(const std::vector<glm::vec3>& points, float size, const glm::vec4& colour,
                  bool alwaysVisible = false) const;

    // An explosion of `radius` at `progress` (0 = just started, 1 = over).
    void drawExplosion(const glm::vec3& centre, float radius, float progress) const;

    // A health bar centred on `centre`, facing the camera: `fraction`
    // (0 to 1) of it in `colour`, the rest dark.
    void drawHealthBar(const glm::vec3& centre, float fraction, const glm::vec4& colour) const;

private:
    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;
    std::unique_ptr<GlMesh> cube_;    // 1 unit wide, centred on the origin
    std::unique_ptr<GlMesh> sphere_;  // radius 1
    mutable glm::vec3 cameraRight_{1.0f, 0.0f, 0.0f};
};

} // namespace openhogs::render
