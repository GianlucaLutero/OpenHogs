// World.cpp - see World.h for an overview.

#include "openhogs/game/World.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/game/Combat.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::game {

namespace {

// Once everything is still after a shot, the next turn starts this much
// later, so the player sees the result.
constexpr float kSettleSeconds = 1.5f;

// Thrown weapons fly at least at this share of their full power.
constexpr float kMinimumPower = 0.1f;

// Shots start at chest height, a little in front of the hog.
constexpr float kMuzzleHeight = 190.0f;
constexpr float kMuzzleForward = 70.0f;

// Melee reaches hogs in front of the attacker: within this height and
// within 60 degrees of where it faces (cos 60 = 0.5).
constexpr float kMeleeHeightReach = 200.0f;
constexpr float kMeleeConeCosine = 0.5f;
constexpr float kMeleeLift = 300.0f;

// Explosions start a little above the ground, so they reach hogs' bodies.
constexpr float kExplosionLift = 30.0f;

// Explosions set off explosives lying within their radius after this delay.
constexpr float kChainReactionSeconds = 0.15f;

// Cluster grenades break into bomblets thrown around and up.
constexpr float kBombletOutwardSpeed = 500.0f;
constexpr float kBombletUpwardSpeed = 700.0f;
constexpr float kBombletFuseSeconds = 1.0f;

bool isAirborne(const Hog& hog) {
    return hog.motion.state == MotionState::Airborne || hog.motion.state == MotionState::Landing;
}

// A mine waiting for a victim does not hold the turn up.
bool isWaitingMine(const Projectile& projectile) {
    const auto& spec = weaponSpec(projectile.weapon);
    return spec.kind == WeaponKind::Placed && spec.triggerRadius > 0.0f && projectile.fuseSecondsLeft <= 0.0f;
}

void addMovementEvents(const MovementEvents& movement, const Hog& hog, std::size_t index,
                       std::vector<GameEvent>& events) {
    GameEvent event;
    event.position = hog.position;
    event.surface = movement.surface;
    event.hog = index;
    for (int i = 0; i < movement.footsteps; ++i) {
        event.type = GameEventType::Footstep;
        events.push_back(event);
    }
    if (movement.landed) {
        event.type = GameEventType::Landed;
        events.push_back(event);
    }
    if (movement.enteredWater) {
        event.type = GameEventType::Splash;
        events.push_back(event);
    }
}

} // namespace

glm::vec3 muzzlePosition(const Hog& hog) {
    return hog.position + glm::vec3(0.0f, kMuzzleHeight, 0.0f) + hogFacing(hog) * kMuzzleForward;
}

glm::vec3 aimDirection(const Hog& hog) {
    const float pitch = glm::radians(hog.aimDegrees);
    return hogFacing(hog) * std::cos(pitch) + glm::vec3(0.0f, std::sin(pitch), 0.0f);
}

World::World(MapCollision map, std::vector<Hog> hogs, TurnRules rules) : map_(std::move(map)), hogs_(std::move(hogs)) {
    if (!hogs_.empty()) {
        turns_.emplace(hogs_, rules);
        hadSeveralTeams_ = turns_->teams().size() > 1;
    }
}

std::optional<std::size_t> World::activeHog() const {
    if (!turns_) {
        return std::nullopt;
    }
    return turns_->activeHog();
}

void World::handleAiming(std::size_t hogIndex, const PlayerInput& input, double seconds,
                         std::vector<GameEvent>& events) {
    Hog& hog = hogs_[hogIndex];
    const bool fireJustPressed = input.fire && !fireWasHeld_;
    fireWasHeld_ = input.fire;

    // Weapons are used standing on dry ground.
    const bool canUseWeapon = hog.alive && !isAirborne(hog) && hog.motion.state != MotionState::Swimming;
    if (!canUseWeapon) {
        hog.charging = false;
        return;
    }

    if (input.weaponStep != 0 && !hog.charging && !hog.weapons.empty()) {
        const auto count = static_cast<long long>(hog.weapons.size());
        const long long slot = (static_cast<long long>(hog.weaponSlot) + input.weaponStep) % count;
        hog.weaponSlot = static_cast<std::size_t>(slot < 0 ? slot + count : slot);
    }

    const int aim = (input.aimUp ? 1 : 0) - (input.aimDown ? 1 : 0);
    hog.aimDegrees = std::clamp(hog.aimDegrees + static_cast<float>(aim) * kAimDegreesPerSecond *
                                                     static_cast<float>(seconds),
                                kLowestAimDegrees, kHighestAimDegrees);

    const WeaponSpec& spec = weaponSpec(hog.selectedWeapon());
    if (!spec.usesPower) {
        if (fireJustPressed) {
            fire(hogIndex, 1.0f, events);
        }
        return;
    }
    // Powered weapons: charge while the key is held, fire when released.
    if (input.fire) {
        if (!hog.charging) {
            hog.charging = true;
            hog.power = 0.0f;
        }
        hog.power = std::min(1.0f, hog.power + static_cast<float>(seconds) / kSecondsToFullPower);
    } else if (hog.charging) {
        hog.charging = false;
        fire(hogIndex, std::max(hog.power, kMinimumPower), events);
    }
}

void World::fire(std::size_t hogIndex, float power, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[hogIndex];
    const WeaponSpec& spec = weaponSpec(hog.selectedWeapon());

    hog.held = HeldAnimation{spec.useAnimation, AnimationPlayer::Repeat::Once};
    hog.animation.play(spec.useAnimation, AnimationPlayer::Repeat::Once, true);
    phase_ = MatchPhase::Resolving;
    settleSecondsLeft_ = kSettleSeconds;

    GameEvent fired;
    fired.type = GameEventType::Fired;
    fired.position = muzzlePosition(hog);
    fired.weapon = spec.id;
    fired.hog = hogIndex;

    switch (spec.kind) {
    case WeaponKind::Melee:
        events.push_back(fired);
        fireMelee(hogIndex, spec, events);
        break;
    case WeaponKind::Bullet:
        // Each shot of the burst reports its own Fired event.
        burst_ = Burst{hogIndex, spec.id, spec.shots, 0.0f};
        break;
    case WeaponKind::Thrown:
    case WeaponKind::Shell:
        events.push_back(fired);
        projectiles_.push_back(launchProjectile(spec.id, hogIndex, muzzlePosition(hog), aimDirection(hog), power));
        break;
    case WeaponKind::Placed: {
        events.push_back(fired);
        glm::vec3 spot = hog.position + hogFacing(hog) * kMuzzleForward;
        spot.y = map_.groundHeight(spot.x, spot.z, hog.position.y);
        projectiles_.push_back(launchProjectile(spec.id, hogIndex, spot, glm::vec3(0.0f), 0.0f));
        break;
    }
    }
}

void World::fireMelee(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events) {
    const Hog& attacker = hogs_[hogIndex];
    const glm::vec3 facing = hogFacing(attacker);
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        Hog& target = hogs_[i];
        if (i == hogIndex || !target.alive) {
            continue;
        }
        glm::vec3 toTarget = target.position - attacker.position;
        const float height = std::abs(toTarget.y);
        toTarget.y = 0.0f;
        const float distance = glm::length(toTarget);
        if (distance > spec.range || height > kMeleeHeightReach) {
            continue;
        }
        const glm::vec3 direction = distance > 1.0f ? toTarget / distance : facing;
        if (glm::dot(direction, facing) < kMeleeConeCosine) {
            continue;
        }
        knockHog(target, direction * spec.knockback + glm::vec3(0.0f, kMeleeLift, 0.0f));
        damageHog(hogs_, i, spec.damage, events);
    }
}

void World::fireShot(std::vector<GameEvent>& events) {
    Burst& burst = *burst_;
    const Hog& shooter = hogs_[burst.shooter];
    const WeaponSpec& spec = weaponSpec(burst.weapon);

    GameEvent fired;
    fired.type = GameEventType::Fired;
    fired.position = muzzlePosition(shooter);
    fired.weapon = spec.id;
    fired.hog = burst.shooter;
    events.push_back(fired);

    // A random offset within the spread, from the reproducible generator.
    const auto randomSpread = [&] {
        const float unit = static_cast<float>(random_() % 2001) / 1000.0f - 1.0f;  // -1 to 1
        return glm::radians(spec.spreadDegrees) * unit;
    };
    for (int pellet = 0; pellet < spec.pellets; ++pellet) {
        const float yaw = shooter.yawRadians + randomSpread();
        const float pitch = glm::radians(shooter.aimDegrees) + randomSpread();
        const glm::vec3 facing{std::cos(yaw), 0.0f, -std::sin(yaw)};
        const glm::vec3 direction = facing * std::cos(pitch) + glm::vec3(0.0f, std::sin(pitch), 0.0f);

        const auto hit = traceBullet(fired.position, direction, spec.range, map_, hogs_, burst.shooter);
        tracers_.push_back({fired.position, hit.point, 0.0f});
        GameEvent bulletHit;
        bulletHit.type = GameEventType::BulletHit;
        bulletHit.position = hit.point;
        bulletHit.weapon = spec.id;
        events.push_back(bulletHit);
        if (hit.hog) {
            Hog& target = hogs_[*hit.hog];
            if (spec.knockback > 0.0f && !isAirborne(target)) {
                const glm::vec3 push = glm::vec3(direction.x, 0.0f, direction.z) * spec.knockback;
                knockHog(target, push + glm::vec3(0.0f, spec.knockback, 0.0f));
            }
            damageHog(hogs_, *hit.hog, spec.damage, events);
        }
    }

    --burst.shotsLeft;
    burst.secondsToNextShot = spec.secondsBetweenShots;
    if (burst.shotsLeft <= 0) {
        burst_.reset();
    }
}

void World::detonate(const Projectile& projectile, std::vector<GameEvent>& events) {
    const WeaponSpec& spec = weaponSpec(projectile.weapon);
    const glm::vec3 centre = projectile.position + glm::vec3(0.0f, kExplosionLift, 0.0f);

    GameEvent explosion;
    explosion.type = GameEventType::Explosion;
    explosion.position = centre;
    explosion.weapon = spec.id;
    events.push_back(explosion);
    explosions_.push_back({centre, spec.blastRadius, 0.0f});

    explode(hogs_, centre, spec.blastRadius, spec.damage, spec.knockback, events);

    // Explosives lying within the blast go off too, a moment later.
    for (auto& other : projectiles_) {
        const auto& otherSpec = weaponSpec(other.weapon);
        if (otherSpec.kind == WeaponKind::Placed && glm::length(other.position - centre) < spec.blastRadius) {
            other.fuseSecondsLeft = kChainReactionSeconds;
        }
    }

    if (spec.bomblets > 0 && !projectile.bomblet) {
        for (int i = 0; i < spec.bomblets; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(spec.bomblets);
            Projectile bomblet = projectile;
            bomblet.bomblet = true;
            bomblet.resting = false;
            bomblet.ageSeconds = 0.0f;
            bomblet.position = centre;
            bomblet.velocity = {std::cos(angle) * kBombletOutwardSpeed, kBombletUpwardSpeed,
                                std::sin(angle) * kBombletOutwardSpeed};
            bomblet.fuseSecondsLeft = kBombletFuseSeconds + 0.1f * static_cast<float>(i);
            projectiles_.push_back(bomblet);
        }
    }
}

void World::stepProjectiles(double seconds, std::vector<GameEvent>& events) {
    // Explosions may add projectiles (bomblets) or set off others, so the
    // list is walked by index and explosions happen after the moves.
    std::vector<Projectile> exploding;
    std::vector<Projectile> remaining;
    for (std::size_t i = 0; i < projectiles_.size(); ++i) {
        Projectile projectile = projectiles_[i];
        const auto outcome = stepProjectile(projectile, seconds, map_, hogs_, events);
        if (outcome == ProjectileOutcome::Explodes) {
            exploding.push_back(projectile);
        } else if (outcome == ProjectileOutcome::Flying) {
            remaining.push_back(projectile);
        }
    }
    projectiles_ = std::move(remaining);
    for (const auto& projectile : exploding) {
        detonate(projectile, events);
    }
}

void World::updateEffects(double seconds) {
    const auto dt = static_cast<float>(seconds);
    for (auto& effect : explosions_) effect.ageSeconds += dt;
    for (auto& effect : tracers_) effect.ageSeconds += dt;
    std::erase_if(explosions_, [](const ExplosionEffect& e) { return e.ageSeconds >= kExplosionEffectSeconds; });
    std::erase_if(tracers_, [](const TracerEffect& e) { return e.ageSeconds >= kTracerEffectSeconds; });
}

bool World::shotSettled() const {
    if (burst_ || !explosions_.empty()) {
        return false;
    }
    for (const auto& projectile : projectiles_) {
        if (!isWaitingMine(projectile)) {
            return false;
        }
    }
    for (const auto& hog : hogs_) {
        if (isAirborne(hog)) {
            return false;
        }
        if (hog.motion.state == MotionState::Dead && !hog.animation.finished()) {
            return false;  // still playing its dying animation
        }
        if (hog.held && hog.held->repeat == AnimationPlayer::Repeat::Once && !hog.animation.finished()) {
            return false;  // still throwing or striking
        }
    }
    return true;
}

bool World::matchIsDecided() const {
    if (!turns_) {
        return false;
    }
    const auto living = turns_->livingTeams(hogs_);
    // A map with a single team (training) only ends if that team is wiped out.
    return living.empty() || (hadSeveralTeams_ && living.size() <= 1);
}

void World::finishMatch(std::vector<GameEvent>& events) {
    phase_ = MatchPhase::Over;
    const auto living = turns_->livingTeams(hogs_);
    winner_ = living.size() == 1 ? std::optional<int>(living.front()) : std::nullopt;
    GameEvent over;
    over.type = GameEventType::MatchOver;
    over.team = winner_.value_or(-1);
    events.push_back(over);
}

void World::startNextTurn(std::vector<GameEvent>& events) {
    for (auto& hog : hogs_) {
        hog.held.reset();
        hog.charging = false;
    }
    if (matchIsDecided()) {
        finishMatch(events);
        return;
    }
    turns_->endTurn(hogs_);
    phase_ = MatchPhase::Playing;
    fireWasHeld_ = true;  // a key still held from the last turn must not fire at once
    GameEvent started;
    started.type = GameEventType::TurnStarted;
    started.team = turns_->activeTeam();
    started.hog = turns_->activeHog();
    events.push_back(started);
}

std::vector<GameEvent> World::update(double seconds, const PlayerInput& input,
                                     const std::vector<formats::Animation>& animations) {
    timeSeconds_ += seconds;
    std::vector<GameEvent> events;
    const auto active = activeHog();
    const bool playing = phase_ == MatchPhase::Playing && active.has_value();

    if (playing) {
        handleAiming(*active, input, seconds, events);
        Hog& hog = hogs_[*active];
        // While still aiming, the hog holds its weapon's aiming pose.
        if (phase_ == MatchPhase::Playing && hog.alive) {
            const auto& spec = weaponSpec(hog.selectedWeapon());
            if (spec.aimAnimation != kStandingAnimation) {
                hog.held = HeldAnimation{spec.aimAnimation, AnimationPlayer::Repeat::Loop};
            } else {
                hog.held.reset();
            }
        }
    }

    float distanceMoved = 0.0f;
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        MovementInput movement;
        bool canMove = false;
        if (active && i == *active && phase_ == MatchPhase::Playing) {
            movement = input.movement;
            canMove = turns_->canMove();
            if (hogs_[i].charging) {
                // Charging a throw: the hog may turn to aim, not walk away.
                movement.forward = movement.backward = movement.jump = false;
            }
        }
        const auto moved = moveHog(hogs_, i, movement, canMove, seconds, map_, animations);
        addMovementEvents(moved, hogs_[i], i, events);
        if (moved.landed) {
            damageHog(hogs_, i, fallDamage(moved.fallHeight), events);
        }
        if (active && i == *active) {
            distanceMoved = moved.distanceMoved;
        }
    }

    if (burst_) {
        burst_->secondsToNextShot -= static_cast<float>(seconds);
        if (burst_->secondsToNextShot <= 0.0f) {
            fireShot(events);
        }
    }
    stepProjectiles(seconds, events);
    updateEffects(seconds);

    if (!turns_ || phase_ == MatchPhase::Over) {
        return events;
    }
    if (phase_ == MatchPhase::Playing) {
        const auto before = turns_->turnNumber();
        if (!hogs_[turns_->activeHog()].alive || matchIsDecided()) {
            // The active hog died (a mine, a fall) or the last enemy did:
            // let things settle, then move on.
            phase_ = MatchPhase::Resolving;
            settleSecondsLeft_ = kSettleSeconds;
        } else if (turns_->update(seconds, distanceMoved, hogs_)) {
            // Out of time: TurnSystem already passed the turn on.
            hogs_[*active].held.reset();
            hogs_[*active].charging = false;
            fireWasHeld_ = true;
            if (turns_->turnNumber() != before) {
                GameEvent started;
                started.type = GameEventType::TurnStarted;
                started.team = turns_->activeTeam();
                started.hog = turns_->activeHog();
                events.push_back(started);
            }
        }
    }
    if (phase_ == MatchPhase::Resolving) {
        if (!shotSettled()) {
            settleSecondsLeft_ = kSettleSeconds;
        } else {
            settleSecondsLeft_ -= static_cast<float>(seconds);
            if (settleSecondsLeft_ <= 0.0f) {
                startNextTurn(events);
            }
        }
    }
    return events;
}

std::vector<glm::vec3> World::aimPreview() const {
    const auto active = activeHog();
    if (!active || phase_ != MatchPhase::Playing) {
        return {};
    }
    const Hog& hog = hogs_[*active];
    if (!hog.alive || isAirborne(hog) || hog.motion.state == MotionState::Swimming) {
        return {};
    }
    const WeaponSpec& spec = weaponSpec(hog.selectedWeapon());
    const glm::vec3 start = muzzlePosition(hog);

    if (spec.kind == WeaponKind::Thrown || spec.kind == WeaponKind::Shell) {
        const float power = spec.usesPower ? (hog.charging ? std::max(hog.power, kMinimumPower) : 1.0f) : 1.0f;
        return predictTrajectory(start, aimDirection(hog) * (spec.launchSpeed * power), spec.gravityFactor, map_);
    }
    if (spec.kind == WeaponKind::Bullet) {
        const auto hit = traceBullet(start, aimDirection(hog), spec.range, map_, hogs_, *active);
        std::vector<glm::vec3> line;
        constexpr float kSpacing = 150.0f;
        const float length = glm::length(hit.point - start);
        for (float along = kSpacing; along < length; along += kSpacing) {
            line.push_back(start + aimDirection(hog) * along);
        }
        line.push_back(hit.point);
        return line;
    }
    return {};
}

bool World::switchHog(int step) {
    return turns_ && phase_ == MatchPhase::Playing && turns_->switchHog(step, hogs_);
}

void World::endTurn() {
    if (turns_ && phase_ == MatchPhase::Playing) {
        std::vector<GameEvent> ignored;
        startNextTurn(ignored);
    }
}

World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName, TurnRules rules,
                const std::function<void(std::vector<Hog>&)>& prepareHogs) {
    const std::string base = "maps/" + core::toLower(mapName);
    if (!vfs.exists(base + ".pmg")) {
        throw std::runtime_error("Map '" + std::string(mapName) + "' not found (no " + base + ".pmg)");
    }
    TerrainHeightField terrain(formats::MapTerrain::parse(vfs.readFile(base + ".pmg")));

    std::vector<Hog> hogs;
    std::vector<CollisionBox> boxes;
    if (vfs.exists(base + ".pog")) {
        const auto objects = formats::parseMapObjects(vfs.readFile(base + ".pog"));
        hogs = spawnHogs(objects, terrain);

        // Only objects with a model in the map's archive get a collision box.
        std::optional<formats::PackageArchive> models;
        if (vfs.exists(base + ".mad")) {
            models = formats::PackageArchive::parse(vfs.readFile(base + ".mad"));
        }
        boxes = collisionBoxes(objects, [&](const std::string& name) {
            return models && models->find(name + ".vtx") != nullptr;
        });
    }
    if (prepareHogs) {
        prepareHogs(hogs);
    }
    return World(MapCollision(std::move(terrain), std::move(boxes)), std::move(hogs), rules);
}

int World::nationOf(int team) const {
    const auto found = nations_.find(team);
    if (found != nations_.end()) {
        return found->second;
    }
    const auto count = static_cast<int>(scene::characterTeams().size());
    return ((team % count) + count) % count;
}

void World::setTeamNation(int team, int nation) {
    nations_[team] = nation;
}

std::vector<int> mapTeams(const core::VirtualFileSystem& vfs, std::string_view mapName) {
    const World world = loadWorld(vfs, mapName);
    if (!world.turns()) {
        return {};
    }
    std::vector<int> teams = world.turns()->teams();
    const auto first = std::find(teams.begin(), teams.end(), world.turns()->activeTeam());
    std::rotate(teams.begin(), first, teams.end());
    return teams;
}

} // namespace openhogs::game
