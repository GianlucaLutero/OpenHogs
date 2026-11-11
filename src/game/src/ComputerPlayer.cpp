// ComputerPlayer.cpp - see ComputerPlayer.h for an overview.

#include "openhogs/game/ComputerPlayer.h"

#include "openhogs/game/Combat.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/Projectiles.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace openhogs::game {

namespace {

// --- Scoring --------------------------------------------------------------

// Killing an enemy is worth more than the damage alone: one hog fewer to
// shoot back.
constexpr float kKillBonus = 30.0f;
// Hurting its own team counts this many times the damage, so the computer
// would rather not shoot than risk its own hogs.
constexpr float kFriendlyFireWeight = 1.5f;
// Expected share of bullets that hit (they spread), and of a bouncing
// grenade's damage (it may roll away before going off).
constexpr float kBulletAccuracy = 0.7f;
constexpr float kBouncingShare = 0.7f;
// Melee: the reach the computer trusts (the rules also need the target in
// front and at about the same height).
constexpr float kMeleeReachShare = 0.9f;
constexpr float kMeleeHeightReach = 100.0f;

// --- Throw search ---------------------------------------------------------

// Throws are found in two passes. A coarse grid of angles and powers uses
// the plain flight arc (cheap: up to the first touch of the ground) to find
// where the shot comes down closest to the target. A finer grid around it
// then plays each throw out exactly, bounces and fuse included, since
// grenades roll on after landing.
constexpr float kCoarseAngleFrom = 5.0f;
constexpr float kCoarseAngleTo = 75.0f;
constexpr float kCoarseAngleStep = 5.0f;
constexpr float kCoarsePowerStep = 0.1f;
constexpr float kFineAngleRange = 4.0f;  // degrees either side of the coarse best
constexpr float kFineAngleStep = 2.0f;
constexpr float kFinePowerRange = 0.15f;  // either side of the coarse best
constexpr float kFinePowerStep = 0.03f;
constexpr float kLowestPower = 0.15f;
// Throws are played out at the match's step for at most the fuse plus
// this long.
constexpr double kSimulationStep = 1.0 / 50.0;
constexpr float kMaxFlightSeconds = 5.0f;

// --- Playing the turn -----------------------------------------------------

// Half the turn and aim made in one step: stopping within it means the
// next step could not get any closer.
constexpr float kYawTolerance = glm::radians(kTurnDegreesPerSecond * 0.02f / 2.0f);
constexpr float kAimTolerance = kAimDegreesPerSecond * 0.02f / 2.0f;
constexpr float kPowerTolerance = 0.02f / kSecondsToFullPower / 2.0f;
// Runs forward only when facing within this angle of where it goes.
constexpr float kRunFacingTolerance = glm::radians(45.0f);
// Blocked: it moved less than this distance over this time.
constexpr float kProgressDistance = 40.0f;
constexpr double kProgressCheckSeconds = 0.75;
constexpr double kMaxApproachSeconds = 10.0;
// After running, a short pause before planning again (to land and stop).
constexpr double kSettleSeconds = 0.4;
// Safety net: a computer turn never takes longer than this.
constexpr double kGiveUpSeconds = 30.0;

float horizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
}

// Value of doing `damage` to `target`: the health it actually loses, plus
// the kill bonus if it dies.
float damageValue(const Hog& target, float damage) {
    const float lost = std::min(damage, target.health);
    return lost + (damage >= target.health ? kKillBonus : 0.0f);
}

// Value of an explosion at `centre`: damage to enemies minus (weighted)
// damage to the shooter's team, the shooter included.
float explosionValue(const World& world, std::size_t shooter, const glm::vec3& centre, const WeaponSpec& spec) {
    const auto& hogs = world.hogs();
    float value = 0.0f;
    for (const auto& hog : hogs) {
        if (!hog.alive) continue;
        const float distance = glm::length(hogCentre(hog) - centre);
        if (distance >= spec.blastRadius) continue;
        const float damage = spec.damage * (1.0f - distance / spec.blastRadius);
        if (hog.team == hogs[shooter].team) {
            value -= kFriendlyFireWeight * damageValue(hog, damage);
        } else {
            value += damageValue(hog, damage);
        }
    }
    return value;
}

// A copy of the shooter turned and aimed as planned, to ask the world's
// own functions where its shot would start and go.
Hog aimedCopy(const Hog& shooter, float yaw, float aimDegrees) {
    Hog copy = shooter;
    copy.yawRadians = yaw;
    copy.aimDegrees = aimDegrees;
    return copy;
}

struct ThrowResult {
    glm::vec3 landing{0.0f};     // where it explodes
    bool landsOnGround = false;  // it explodes (did not fall in water or leave the map)
};

// Where a throw first comes down, from its plain flight arc (fast).
ThrowResult throwArc(const World& world, std::size_t shooter, WeaponId weapon, float yaw, float angle, float power) {
    const Hog copy = aimedCopy(world.hogs()[shooter], yaw, angle);
    const auto projectile = launchProjectile(weapon, shooter, muzzlePosition(copy), aimDirection(copy), power);
    const auto path =
        predictTrajectory(projectile.position, projectile.velocity, weaponSpec(weapon).gravityFactor, world.map());
    ThrowResult result;
    if (path.empty()) return result;
    result.landing = path.back();
    const auto& terrain = world.terrain();
    result.landsOnGround = terrain.contains(result.landing.x, result.landing.z) &&
                           !terrain.tileAt(result.landing.x, result.landing.z).isWater;
    return result;
}

// Plays the throw out with the match's own projectile rules, bounces and
// fuse included, and returns where it would explode (slower).
ThrowResult simulateThrow(const World& world, std::size_t shooter, WeaponId weapon, float yaw, float angle,
                          float power) {
    const Hog copy = aimedCopy(world.hogs()[shooter], yaw, angle);
    auto projectile = launchProjectile(weapon, shooter, muzzlePosition(copy), aimDirection(copy), power);
    const int maxSteps = static_cast<int>((weaponSpec(weapon).fuseSeconds + kMaxFlightSeconds) / kSimulationStep);
    std::vector<GameEvent> ignored;
    ThrowResult result;
    for (int step = 0; step < maxSteps; ++step) {
        const auto outcome = stepProjectile(projectile, kSimulationStep, world.map(), world.hogs(), ignored);
        if (outcome == ProjectileOutcome::Explodes) {
            result.landing = projectile.position;
            result.landsOnGround = true;
            break;
        }
        if (outcome == ProjectileOutcome::Gone) break;
    }
    return result;
}

// The angle and power that land a throw closest to `target`.
std::optional<ShotPlan> planThrow(const World& world, std::size_t shooter, const WeaponSpec& spec,
                                  std::size_t target) {
    const Hog& me = world.hogs()[shooter];
    const Hog& enemy = world.hogs()[target];
    const float yaw = yawTowards(enemy.position - me.position);

    float bestMiss = std::numeric_limits<float>::max();
    ShotPlan best;
    best.weapon = spec.id;
    best.target = target;
    best.yawRadians = yaw;
    std::optional<glm::vec3> bestLanding;

    const auto tryShot = [&](float angle, float power, bool exact) {
        angle = std::clamp(angle, kLowestAimDegrees, kHighestAimDegrees);
        power = std::clamp(power, kLowestPower, 1.0f);
        const auto result = exact ? simulateThrow(world, shooter, spec.id, yaw, angle, power)
                                  : throwArc(world, shooter, spec.id, yaw, angle, power);
        if (!result.landsOnGround) return;
        const float miss = glm::length(result.landing - enemy.position);
        if (miss < bestMiss) {
            bestMiss = miss;
            best.aimDegrees = angle;
            best.power = power;
            bestLanding = result.landing;
        }
    };

    const float coarsePowerStep = spec.usesPower ? kCoarsePowerStep : 1.0f;
    for (float angle = kCoarseAngleFrom; angle <= kCoarseAngleTo; angle += kCoarseAngleStep) {
        for (float power = 1.0f; power >= kLowestPower - 1e-4f; power -= coarsePowerStep) {
            tryShot(angle, power, false);
        }
    }
    if (!bestLanding) {
        return std::nullopt;
    }
    // Refine around the best coarse shot, playing each throw out exactly.
    const float centreAngle = best.aimDegrees;
    const float centrePower = best.power;
    bestMiss = std::numeric_limits<float>::max();
    bestLanding.reset();
    for (float angle = centreAngle - kFineAngleRange; angle <= centreAngle + kFineAngleRange + 1e-3f;
         angle += kFineAngleStep) {
        if (!spec.usesPower) {
            tryShot(angle, 1.0f, true);
            continue;
        }
        for (float power = centrePower - kFinePowerRange; power <= centrePower + kFinePowerRange + 1e-4f;
             power += kFinePowerStep) {
            tryShot(angle, power, true);
        }
    }
    if (!bestLanding) {
        return std::nullopt;
    }

    best.score = explosionValue(world, shooter, *bestLanding, spec);
    if (spec.bounciness > 0.0f) {
        best.score *= kBouncingShare;
    }
    return best;
}

std::optional<ShotPlan> planBullet(const World& world, std::size_t shooter, const WeaponSpec& spec,
                                   std::size_t target) {
    const Hog& me = world.hogs()[shooter];
    const Hog& enemy = world.hogs()[target];
    const float yaw = yawTowards(enemy.position - me.position);
    const glm::vec3 muzzle = muzzlePosition(aimedCopy(me, yaw, 0.0f));
    const glm::vec3 toTarget = hogCentre(enemy) - muzzle;
    if (glm::length(toTarget) > spec.range) {
        return std::nullopt;
    }
    const float angle =
        glm::degrees(std::atan2(toTarget.y, glm::length(glm::vec2(toTarget.x, toTarget.z))));
    if (angle < kLowestAimDegrees || angle > kHighestAimDegrees) {
        return std::nullopt;
    }
    // Would the bullet get there, or hit the scenery or another hog first?
    const auto hit = traceBullet(muzzle, aimDirection(aimedCopy(me, yaw, angle)), spec.range, world.map(),
                                 world.hogs(), shooter);
    if (hit.hog != target) {
        return std::nullopt;
    }
    ShotPlan plan;
    plan.weapon = spec.id;
    plan.target = target;
    plan.yawRadians = yaw;
    plan.aimDegrees = angle;
    plan.score = damageValue(enemy, spec.damage * static_cast<float>(spec.shots * spec.pellets) * kBulletAccuracy);
    return plan;
}

std::optional<ShotPlan> planMelee(const World& world, std::size_t shooter, const WeaponSpec& spec,
                                  std::size_t target) {
    const Hog& me = world.hogs()[shooter];
    const Hog& enemy = world.hogs()[target];
    if (horizontalDistance(me.position, enemy.position) > spec.range * kMeleeReachShare ||
        std::abs(me.position.y - enemy.position.y) > kMeleeHeightReach) {
        return std::nullopt;
    }
    ShotPlan plan;
    plan.weapon = spec.id;
    plan.target = target;
    plan.yawRadians = yawTowards(enemy.position - me.position);
    plan.aimDegrees = me.aimDegrees;  // melee does not care
    plan.score = damageValue(enemy, spec.damage);
    return plan;
}

// The signed smallest angle from `from` to `to`, in -pi..pi.
float angleDifference(float from, float to) {
    return std::remainder(to - from, glm::two_pi<float>());
}

// Turn input that brings the hog's yaw to `wanted`; true once there.
bool steer(const Hog& hog, float wanted, MovementInput& movement) {
    const float difference = angleDifference(hog.yawRadians, wanted);
    if (std::abs(difference) <= kYawTolerance) {
        return true;
    }
    // A positive turn (turnLeft) increases the yaw.
    movement.turnLeft = difference > 0.0f;
    movement.turnRight = difference < 0.0f;
    return false;
}

std::optional<std::size_t> nearestEnemy(const World& world, std::size_t shooter) {
    const auto& hogs = world.hogs();
    std::optional<std::size_t> nearest;
    float nearestDistance = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        if (!hogs[i].alive || hogs[i].team == hogs[shooter].team) continue;
        const float distance = horizontalDistance(hogs[i].position, hogs[shooter].position);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearest = i;
        }
    }
    return nearest;
}

} // namespace

float yawTowards(const glm::vec3& direction) {
    // hogFacing() is (cos yaw, 0, -sin yaw).
    return std::atan2(-direction.z, direction.x);
}

ShotPlanner::ShotPlanner(const World& world, std::size_t shooter) : shooter_(shooter) {
    const auto& hogs = world.hogs();
    const Hog& me = hogs[shooter];
    for (const WeaponId weapon : me.weapons) {
        for (std::size_t target = 0; target < hogs.size(); ++target) {
            if (hogs[target].alive && hogs[target].team != me.team) {
                pending_.push_back({weapon, target});
            }
        }
    }
}

void ShotPlanner::step(const World& world) {
    if (done()) return;
    const auto [weapon, target] = pending_[next_++];
    const WeaponSpec& spec = weaponSpec(weapon);
    std::optional<ShotPlan> plan;
    switch (spec.kind) {
    case WeaponKind::Melee: plan = planMelee(world, shooter_, spec, target); break;
    case WeaponKind::Bullet: plan = planBullet(world, shooter_, spec, target); break;
    case WeaponKind::Thrown:
    case WeaponKind::Shell: plan = planThrow(world, shooter_, spec, target); break;
    // Explosives put down would need the hog to walk away after using
    // them, which the rules do not allow yet.
    case WeaponKind::Placed: break;
    }
    if (plan && plan->score > 0.0f && (!best_ || plan->score > best_->score)) {
        best_ = plan;
    }
}

std::optional<ShotPlan> planShot(const World& world, std::size_t shooter) {
    ShotPlanner planner(world, shooter);
    while (!planner.done()) {
        planner.step(world);
    }
    return planner.best();
}

ComputerPlayer::ComputerPlayer(AiSettings settings) : settings_(settings), random_(settings.seed) {}

void ComputerPlayer::enter(Stage stage, double seconds) {
    stage_ = stage;
    stageSeconds_ = seconds;
}

void ComputerPlayer::startTurn(const World& world) {
    turnNumber_ = world.turns()->turnNumber();
    hog_ = world.turns()->activeHog();
    turnSeconds_ = 0.0;
    plan_.reset();
    planner_.reset();
    hasApproached_ = false;
    hasJumped_ = false;
    approachTarget_.reset();
    enter(Stage::Thinking, settings_.thinkSeconds);
}

void ComputerPlayer::spoil(ShotPlan& plan) {
    // A uniform error in -1..1 of each setting.
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    plan.yawRadians += glm::radians(settings_.aimErrorDegrees) * unit(random_);
    plan.aimDegrees = std::clamp(plan.aimDegrees + settings_.aimErrorDegrees * unit(random_), kLowestAimDegrees,
                                 kHighestAimDegrees);
    if (weaponSpec(plan.weapon).usesPower) {
        plan.power = std::clamp(plan.power + settings_.powerError * unit(random_), 0.1f, 1.0f);
    }
}

void ComputerPlayer::decide(const World& world, ComputerDecision& decision) {
    plan_ = planner_->best();
    planner_.reset();
    if (plan_ && plan_->score >= settings_.minimumScore) {
        spoil(*plan_);
        enter(Stage::Turning);
        return;
    }
    if (!hasApproached_ && world.turns()->canMove()) {
        approachTarget_ = nearestEnemy(world, hog_);
        if (approachTarget_) {
            const Hog& hog = world.hogs()[hog_];
            progressCheckPosition_ = hog.position;
            progressCheckSeconds_ = 0.0;
            enter(Stage::Approaching);
            return;
        }
    }
    if (plan_) {
        // Not much, but better than nothing.
        spoil(*plan_);
        enter(Stage::Turning);
        return;
    }
    decision.endTurn = true;
    enter(Stage::Done);
}

void ComputerPlayer::approach(const World& world, const Hog& hog, double seconds, ComputerDecision& decision) {
    const auto& hogs = world.hogs();
    const bool targetGone = !approachTarget_ || !hogs[*approachTarget_].alive;
    const glm::vec3 goal = targetGone ? hog.position : hogs[*approachTarget_].position;

    progressCheckSeconds_ += seconds;
    bool blocked = false;
    if (progressCheckSeconds_ >= kProgressCheckSeconds) {
        blocked = horizontalDistance(hog.position, progressCheckPosition_) < kProgressDistance;
        progressCheckPosition_ = hog.position;
        progressCheckSeconds_ = 0.0;
    }
    if (blocked && !hasJumped_) {
        // Try once to jump over whatever is in the way.
        decision.input.movement.forward = true;
        decision.input.movement.jump = true;
        hasJumped_ = true;
        blocked = false;
    }

    const bool arrived = horizontalDistance(hog.position, goal) <= settings_.approachDistance;
    if (targetGone || arrived || blocked || !world.turns()->canMove() || stageSeconds_ > kMaxApproachSeconds) {
        hasApproached_ = true;
        decision.input.movement = {};
        enter(Stage::Thinking, kSettleSeconds);
        planner_.reset();  // planned again from where it stops
        return;
    }

    const float wanted = yawTowards(goal - hog.position);
    steer(hog, wanted, decision.input.movement);
    if (std::abs(angleDifference(hog.yawRadians, wanted)) < kRunFacingTolerance) {
        decision.input.movement.forward = true;
    }
}

ComputerDecision ComputerPlayer::update(const World& world, double seconds) {
    ComputerDecision decision;
    const auto& turns = world.turns();
    if (!turns || world.phase() != MatchPhase::Playing) {
        return decision;  // the shot is flying, or the match is over
    }
    if (turns->turnNumber() != turnNumber_ || turns->activeHog() != hog_) {
        startTurn(world);
    }
    const Hog& hog = world.hogs()[hog_];
    turnSeconds_ += seconds;
    if (stage_ == Stage::Thinking) {
        stageSeconds_ -= seconds;
    } else {
        stageSeconds_ += seconds;
    }

    if (turnSeconds_ > kGiveUpSeconds && stage_ != Stage::Done) {
        decision.endTurn = true;
        enter(Stage::Done);
        return decision;
    }

    switch (stage_) {
    case Stage::Thinking: {
        // Plan only once the hog is back on its feet: the plan starts from
        // where it is. One weapon and target is weighed per step, so the
        // work is spread over the thinking time instead of freezing a frame.
        const bool steady = hog.motion.state != MotionState::Airborne && hog.motion.state != MotionState::Landing;
        if (!steady) break;
        if (!planner_) planner_.emplace(world, hog_);
        planner_->step(world);
        if (stageSeconds_ <= 0.0 && planner_->done()) {
            decide(world, decision);
        }
        break;
    }

    case Stage::Approaching:
        approach(world, hog, seconds, decision);
        break;

    case Stage::Turning:
        if (steer(hog, plan_->yawRadians, decision.input.movement)) {
            enter(Stage::Selecting);
        }
        break;

    case Stage::Selecting:
        if (hog.selectedWeapon() != plan_->weapon) {
            decision.input.weaponStep = 1;
        } else {
            enter(Stage::Aiming);
        }
        break;

    case Stage::Aiming: {
        const float difference = plan_->aimDegrees - hog.aimDegrees;
        if (std::abs(difference) <= kAimTolerance) {
            enter(Stage::Firing);
        } else {
            decision.input.aimUp = difference > 0.0f;
            decision.input.aimDown = difference < 0.0f;
        }
        break;
    }

    case Stage::Firing:
        if (!weaponSpec(plan_->weapon).usesPower) {
            decision.input.fire = true;  // one press
            enter(Stage::Done);
        } else if (!hog.charging || hog.power < plan_->power - kPowerTolerance) {
            decision.input.fire = true;  // keep charging
        } else {
            enter(Stage::Done);  // releasing the key throws
        }
        break;

    case Stage::Done:
        break;
    }
    return decision;
}

} // namespace openhogs::game
