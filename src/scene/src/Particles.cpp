// Particles.cpp - see Particles.h for an overview.

#include "openhogs/scene/Particles.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

namespace openhogs::scene {

namespace {

// Explosion recipe (decision 160). Counts grow with the blast's reach so a
// grenade and a 1000 lb bomb do not look alike.
constexpr float kFireballLife = 0.6f;
constexpr float kSmokeLife = 2.2f;
constexpr float kSmokeRise = 140.0f;
constexpr float kClodLife = 1.4f;
constexpr float kClodSize = 36.0f;
constexpr float kClodFramesPerSecond = 14.0f;
constexpr float kScorchLife = 30.0f;
constexpr float kGravity = 2400.0f;
constexpr float kSmokeGrey = 0.55f;

int countFor(float radius, float perHundredUnits, int least, int most) {
    return std::clamp(static_cast<int>(radius / 100.0f * perHundredUnits), least, most);
}

} // namespace

float Particle::size() const {
    return startSize + (endSize - startSize) * std::clamp(progress(), 0.0f, 1.0f);
}

float Particle::opacity() const {
    if (sprite == ParticleSprite::Scorch) {
        // The mark stays dark and fades over its last fifth.
        return startOpacity * std::clamp((1.0f - progress()) * 5.0f, 0.0f, 1.0f);
    }
    return startOpacity * std::clamp(1.0f - progress(), 0.0f, 1.0f);
}

std::size_t Particle::frame(std::size_t frameCount) const {
    if (frameCount == 0) return 0;
    return static_cast<std::size_t>(std::max(0.0f, ageSeconds * spin)) % frameCount;
}

ParticleSystem::ParticleSystem(unsigned seed) : random_(seed) {}

float ParticleSystem::between(float low, float high) {
    return std::uniform_real_distribution<float>(low, high)(random_);
}

glm::vec3 ParticleSystem::outward(float horizontalSpeed, float upLow, float upHigh) {
    const float angle = between(0.0f, glm::two_pi<float>());
    const float speed = between(0.4f, 1.0f) * horizontalSpeed;
    return {std::cos(angle) * speed, between(upLow, upHigh), std::sin(angle) * speed};
}

void ParticleSystem::explosion(const glm::vec3& centre, float radius, float ground, bool inWater) {
    radius = std::max(radius, 60.0f);

    // The fireball: several balls of fire swelling out from the centre.
    for (int i = 0, n = countFor(radius, 1.5f, 4, 12); i < n; ++i) {
        Particle fire;
        fire.sprite = ParticleSprite::Fireball;
        fire.position = centre + outward(radius * 0.25f, -radius * 0.1f, radius * 0.25f) * 0.5f;
        fire.velocity = outward(radius * 0.8f, 0.0f, radius * 0.6f);
        fire.startSize = radius * between(0.35f, 0.55f);
        fire.endSize = radius * between(0.9f, 1.3f);
        fire.lifeSeconds = kFireballLife * between(0.7f, 1.1f);
        fire.delaySeconds = between(0.0f, 0.12f);
        particles_.push_back(fire);
    }
    // Smoke rising once the fire is out.
    for (int i = 0, n = countFor(radius, 1.0f, 3, 8); i < n; ++i) {
        Particle smoke;
        smoke.sprite = ParticleSprite::Smoke;
        smoke.position = centre + outward(radius * 0.3f, 0.0f, radius * 0.3f) * 0.5f;
        smoke.velocity = outward(radius * 0.15f, kSmokeRise * 0.6f, kSmokeRise);
        smoke.startSize = radius * 0.5f;
        smoke.endSize = radius * between(1.2f, 1.6f);
        smoke.startOpacity = 0.75f;
        smoke.shade = kSmokeGrey;
        smoke.lifeSeconds = kSmokeLife * between(0.8f, 1.2f);
        smoke.delaySeconds = between(0.2f, 0.45f);
        particles_.push_back(smoke);
    }
    // Earth (or water) thrown up.
    for (int i = 0, n = countFor(radius, 2.0f, 4, 16); i < n; ++i) {
        Particle bit;
        bit.sprite = inWater ? ParticleSprite::Foam : ParticleSprite::Clod;
        bit.position = centre;
        bit.velocity = outward(radius * 1.6f, 500.0f, 1100.0f);
        bit.gravity = kGravity;
        bit.startSize = inWater ? 60.0f : kClodSize;
        bit.endSize = inWater ? 140.0f : kClodSize;
        bit.startOpacity = inWater ? 0.9f : 1.0f;
        bit.lifeSeconds = kClodLife * between(0.7f, 1.0f);
        bit.spin = kClodFramesPerSecond * between(0.7f, 1.3f);
        particles_.push_back(bit);
    }
    // The scorch mark on the ground under it, if the ground is close.
    if (!inWater && centre.y - ground < radius) {
        Particle mark;
        mark.sprite = ParticleSprite::Scorch;
        mark.position = {centre.x, ground, centre.z};
        mark.startSize = radius * 1.2f;
        mark.endSize = mark.startSize;
        mark.startOpacity = 0.85f;
        mark.lifeSeconds = kScorchLife;
        particles_.push_back(mark);
    }
}

void ParticleSystem::splash(const glm::vec3& at) {
    for (int i = 0; i < 8; ++i) {
        Particle foam;
        foam.sprite = ParticleSprite::Foam;
        foam.position = at;
        foam.velocity = outward(180.0f, 400.0f, 800.0f);
        foam.gravity = kGravity;
        foam.startSize = 50.0f;
        foam.endSize = 110.0f;
        foam.startOpacity = 0.9f;
        foam.lifeSeconds = between(0.6f, 0.9f);
        particles_.push_back(foam);
    }
}

void ParticleSystem::bulletHit(const glm::vec3& at) {
    for (int i = 0; i < 3; ++i) {
        Particle bit;
        bit.sprite = ParticleSprite::Clod;
        bit.position = at;
        bit.velocity = outward(200.0f, 250.0f, 500.0f);
        bit.gravity = kGravity;
        bit.startSize = bit.endSize = kClodSize * 0.6f;
        bit.lifeSeconds = between(0.4f, 0.6f);
        bit.spin = kClodFramesPerSecond;
        particles_.push_back(bit);
    }
    Particle dust;
    dust.sprite = ParticleSprite::Smoke;
    dust.position = at;
    dust.velocity = {0.0f, 60.0f, 0.0f};
    dust.startSize = 30.0f;
    dust.endSize = 90.0f;
    dust.startOpacity = 0.6f;
    dust.shade = 0.8f;
    dust.lifeSeconds = 0.8f;
    particles_.push_back(dust);
}

void ParticleSystem::puff(const glm::vec3& at) {
    Particle smoke;
    smoke.sprite = ParticleSprite::Smoke;
    smoke.position = at;
    smoke.velocity = {between(-15.0f, 15.0f), 90.0f, between(-15.0f, 15.0f)};
    smoke.startSize = 40.0f;
    smoke.endSize = 140.0f;
    smoke.startOpacity = 0.7f;
    smoke.shade = 0.35f;
    smoke.lifeSeconds = 2.5f;
    particles_.push_back(smoke);
}

void ParticleSystem::update(double seconds) {
    const float dt = static_cast<float>(seconds);
    for (auto& particle : particles_) {
        if (particle.delaySeconds > 0.0f) {
            particle.delaySeconds -= dt;
            continue;
        }
        particle.ageSeconds += dt;
        particle.velocity.y -= particle.gravity * dt;
        particle.position += particle.velocity * dt;
    }
    std::erase_if(particles_, [](const Particle& particle) { return particle.ageSeconds >= particle.lifeSeconds; });
}

} // namespace openhogs::scene
