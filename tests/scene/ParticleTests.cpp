// Tests for the effects' particles (scene/Particles.h). They protect what
// the player reads from an explosion: something to see at once (fire),
// what it was made of (earth on land, foam in water), a mark left where it
// happened, and that the effects clear away instead of piling up over a
// long match.

#include "openhogs/scene/Particles.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace openhogs::scene;
using Catch::Approx;

namespace {

std::size_t countOf(const ParticleSystem& system, ParticleSprite sprite) {
    return static_cast<std::size_t>(std::count_if(system.particles().begin(), system.particles().end(),
                                                  [&](const Particle& p) { return p.sprite == sprite; }));
}

} // namespace

TEST_CASE("An explosion on land throws fire, smoke and earth, and scorches the ground", "[scene][particles]") {
    // The scorch mark lies on the ground under the blast, not in the air.
    ParticleSystem system;
    system.explosion({100.0f, 260.0f, -50.0f}, 300.0f, 200.0f, false);
    CHECK(countOf(system, ParticleSprite::Fireball) > 0);
    CHECK(countOf(system, ParticleSprite::Smoke) > 0);
    CHECK(countOf(system, ParticleSprite::Clod) > 0);
    CHECK(countOf(system, ParticleSprite::Foam) == 0);
    REQUIRE(countOf(system, ParticleSprite::Scorch) == 1);
    const auto mark = std::find_if(system.particles().begin(), system.particles().end(),
                                   [](const Particle& p) { return p.sprite == ParticleSprite::Scorch; });
    CHECK(mark->position.y == Approx(200.0f));
}

TEST_CASE("An explosion in water throws foam and leaves no mark", "[scene][particles]") {
    ParticleSystem system;
    system.explosion({0.0f, 10.0f, 0.0f}, 300.0f, 0.0f, true);
    CHECK(countOf(system, ParticleSprite::Foam) > 0);
    CHECK(countOf(system, ParticleSprite::Clod) == 0);
    CHECK(countOf(system, ParticleSprite::Scorch) == 0);
}

TEST_CASE("A bigger blast makes a bigger show", "[scene][particles]") {
    // A grenade and a 1000 lb bomb must not look alike.
    ParticleSystem small, big;
    small.explosion({}, 150.0f, 0.0f, false);
    big.explosion({}, 800.0f, 0.0f, false);
    CHECK(big.particles().size() > small.particles().size());
}

TEST_CASE("Fire is gone quickly, the scorch mark stays, and everything clears in the end", "[scene][particles]") {
    // Effects must not build up during a long match.
    ParticleSystem system;
    system.explosion({}, 300.0f, 0.0f, false);
    system.splash({500.0f, 0.0f, 0.0f});
    system.puff({0.0f, 50.0f, 0.0f});
    for (int i = 0; i < 50 * 5; ++i) system.update(0.02);  // five seconds
    CHECK(countOf(system, ParticleSprite::Fireball) == 0);
    CHECK(countOf(system, ParticleSprite::Scorch) == 1);
    for (int i = 0; i < 50 * 60; ++i) system.update(0.02);  // another minute
    CHECK(system.particles().empty());
}

TEST_CASE("Particles fade out and clods fall", "[scene][particles]") {
    // A particle that suddenly vanished at full strength would flicker.
    ParticleSystem system;
    system.bulletHit({0.0f, 0.0f, 0.0f});
    const auto clod = std::find_if(system.particles().begin(), system.particles().end(),
                                   [](const Particle& p) { return p.sprite == ParticleSprite::Clod; });
    REQUIRE(clod != system.particles().end());
    Particle p = *clod;
    p.ageSeconds = p.lifeSeconds * 0.99f;
    CHECK(p.opacity() < 0.05f);
    const float firstUp = clod->velocity.y;
    system.update(0.1);
    CHECK(system.particles().front().velocity.y < firstUp);
}
