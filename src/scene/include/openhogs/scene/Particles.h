// Particles.h
//
// The small pictures that make up the game's effects: fireballs, smoke,
// clods of earth, water foam and the scorch mark an explosion leaves. The
// pictures are the original's, from Language/Tims/expltims.mad (decision
// 29):
//
//   ptp0      a fireball (black around it: drawn added to the scene)
//   ptp1      a puff of smoke
//   ptp2      water foam
//   pcfx003-011  a clod of earth turning, nine frames
//   pcfx013   the scorch mark, a dithered black stain
//
// The executable drives them with a particle system of some 95 emitter
// kinds and a table of explosion recipes (FUN_00487ca0, 0x4D61E8; see
// docs/formats/effects.md). How each kind moves was not decoded in full,
// so the motion, sizes and lifetimes here are OpenHogs' own (decision
// 160), with the original's pictures.
//
// This class only simulates; render::EffectsRenderer draws the particles
// as pictures facing the camera (the scorch mark lies on the ground).

#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <random>
#include <vector>

namespace openhogs::scene {

enum class ParticleSprite { Fireball, Smoke, Foam, Clod, Scorch };

struct Particle {
    ParticleSprite sprite = ParticleSprite::Smoke;
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};    // world units per second
    float gravity = 0.0f;        // downwards, units per second squared
    float startSize = 0.0f;      // width in world units when born...
    float endSize = 0.0f;        // ...and at the end of its life
    float startOpacity = 1.0f;   // fades linearly to 0
    float shade = 1.0f;          // colour multiplier (grey smoke, darker clods)
    float lifeSeconds = 1.0f;
    float ageSeconds = 0.0f;
    float delaySeconds = 0.0f;   // not shown (nor moving) until then
    float spin = 0.0f;           // clods: frames per second of their turning

    float progress() const { return ageSeconds / lifeSeconds; }
    float size() const;
    float opacity() const;
    bool visible() const { return delaySeconds <= 0.0f; }
    // Clods: which of the nine frames to show.
    std::size_t frame(std::size_t frameCount) const;
};

class ParticleSystem {
public:
    explicit ParticleSystem(unsigned seed = 20261119);

    // An explosion of `radius` (the blast's reach) at `centre`. `ground`
    // is the height of the ground under it, for the scorch mark; `inWater`
    // makes foam instead of earth and no mark.
    void explosion(const glm::vec3& centre, float radius, float ground, bool inWater);
    // Something fell into water.
    void splash(const glm::vec3& at);
    // A bullet hit the ground or an object.
    void bulletHit(const glm::vec3& at);
    // A puff of smoke rising (the boots of a hog that blew up).
    void puff(const glm::vec3& at);

    void update(double seconds);
    const std::vector<Particle>& particles() const { return particles_; }

private:
    float between(float low, float high);
    glm::vec3 outward(float horizontalSpeed, float upLow, float upHigh);

    std::vector<Particle> particles_;
    std::minstd_rand random_;
};

} // namespace openhogs::scene
