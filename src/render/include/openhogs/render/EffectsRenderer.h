// EffectsRenderer.h
//
// Draws the game's effects on top of the scene:
//   - particles (scene::ParticleSystem): fireballs, smoke, clods of earth,
//     water foam, scorch marks, with the original's pictures from
//     Language/Tims/expltims.mad, facing the camera;
//   - shadows: the dark blob (Shad_org) under hogs;
//   - dots: the aiming line or arc, bullet tracers, flying shells.
// Without the pictures, explosions fall back to a ball that grows and
// fades from yellow to dark red.

#pragma once

#include "openhogs/render/Camera.h"
#include "openhogs/render/GlObjects.h"
#include "openhogs/scene/Particles.h"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <vector>

namespace openhogs::render {

// The effect pictures of expltims.mad (see scene/Particles.h).
struct EffectPictures {
    formats::RgbaImage fireball;          // ptp0
    formats::RgbaImage smoke;             // ptp1
    formats::RgbaImage foam;              // ptp2
    formats::RgbaImage scorch;            // pcfx013
    formats::RgbaImage shadow;            // Shad_org
    std::vector<formats::RgbaImage> clod; // pcfx003-011
};

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

    // An explosion of `radius` at `progress` (0 = just started, 1 = over),
    // as a plain ball: used only when the pictures are missing.
    void drawExplosion(const glm::vec3& centre, float radius, float progress) const;

    void setPictures(const EffectPictures& pictures);
    bool hasPictures() const { return pictures_.has_value(); }

    // The particles, as pictures facing the camera (the scorch mark lies
    // flat on the ground): marks first, then smoke, foam and clods mixed
    // over the scene, fire added to it last.
    void drawParticles(const std::vector<scene::Particle>& particles) const;

    // A shadow `width` across, lying on the ground at `feet`.
    void drawShadow(const glm::vec3& feet, float width, float opacity) const;

private:
    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;
    std::unique_ptr<GlMesh> cube_;    // 1 unit wide, centred on the origin
    std::unique_ptr<GlMesh> sphere_;  // radius 1
    std::unique_ptr<GlMesh> quad_;    // 1 x 1 in X and Y, centred, facing +Z

    struct GpuPictures {
        GlTexture fireball, smoke, foam, scorch, shadow;
        std::vector<GlTexture> clod;
    };
    std::optional<GpuPictures> pictures_;
    // The camera's axes, set by begin(), to turn pictures towards it.
    mutable glm::vec3 cameraRight_{1.0f, 0.0f, 0.0f};
    mutable glm::vec3 cameraUp_{0.0f, 1.0f, 0.0f};

    void drawFacing(const GlTexture& texture, const glm::vec3& centre, float size, const glm::vec4& tint) const;
    void drawFlat(const GlTexture& texture, const glm::vec3& centre, float size, const glm::vec4& tint) const;
};

} // namespace openhogs::render
