// World.cpp - see World.h for an overview.

#include "openhogs/game/World.h"

#include "openhogs/core/PathUtils.h"
#include "openhogs/formats/MapObjects.h"
#include "openhogs/formats/MapTerrain.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/game/Combat.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <limits>
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
constexpr float kSuddenDeathHealth = 1.0f;

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
// Two mines closer than this count as put down on the same spot.
constexpr float kStackedMineDistance = 60.0f;
// The gases' lasting effects (the wiki): poison takes 10 points at each of
// the hog's turns, madness makes it run for about ten seconds.
constexpr float kPoisonDamage = 10.0f;
// Flying: special ops and airstrike bombs start this high above the ground;
// the jetpack flies along the hog's aim at this speed and a dropped one
// falls away behind at a share of it; a guided missile turns at most this
// fast.
constexpr float kDropHeight = 2500.0f;
constexpr float kAirstrikeFallSpeed = 300.0f;
constexpr float kJetpackSpeed = 700.0f;
constexpr float kDroppedJetpackSpeedShare = 0.5f;
constexpr float kGuidedDegreesPerSecond = 90.0f;
// Getting into a building or vehicle: the hog must stand this close to it.
// Driving: speed and turning of tanks and troop carriers.
constexpr float kMountReach = 120.0f;
constexpr float kDriveSpeed = 400.0f;
constexpr float kDriveTurnDegreesPerSecond = 60.0f;
// Shots from inside leave this far beyond the object's box.
constexpr float kMountMuzzleClearance = 60.0f;
// The eye of a hog looking through binoculars, and how narrow their view is.
constexpr float kEyeHeight = 230.0f;

// How far apart the bombs of an airstrike fall, as a share of their blast.
constexpr float kAirstrikeSpacing = 0.8f;
constexpr float kMadnessSeconds = 10.0f;

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
    if (hog.sheltered()) {
        // From the barrel of the building or vehicle it is in.
        return hog.position + glm::vec3(0.0f, hog.mountMuzzleHeight, 0.0f) + hogFacing(hog) * hog.mountMuzzleReach;
    }
    return hog.position + glm::vec3(0.0f, kMuzzleHeight, 0.0f) + hogFacing(hog) * kMuzzleForward;
}

glm::vec3 aimDirection(const Hog& hog) {
    const float pitch = glm::radians(hog.aimDegrees);
    return hogFacing(hog) * std::cos(pitch) + glm::vec3(0.0f, std::sin(pitch), 0.0f);
}

World::World(MapCollision map, std::vector<Hog> hogs, TurnRules rules, std::vector<Pickup> pickups)
    : map_(std::move(map)), hogs_(std::move(hogs)), pickups_(std::move(pickups)) {
    if (!hogs_.empty()) {
        turns_.emplace(hogs_, rules);
        hadSeveralTeams_ = turns_->teams().size() > 1;
    }
    for (std::size_t i = 0; i < hogs_.size(); ++i) applyFooter(i);  // SKIP TURN for everyone
    if (isDeathmatch()) startingHogs_ = hogs_;
}

int World::deathmatchPoints(int team) const {
    int kills = 0;
    for (const auto& hog : hogs_) {
        if (hog.team == team) kills += hog.record.enemiesKilled;
    }
    const auto lost = deathmatchLosses_.find(team);
    return 2 * kills - (lost == deathmatchLosses_.end() ? 0 : lost->second);
}

void World::respawnDeadHogs(std::vector<GameEvent>& events) {
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        if (hogs_[i].alive) continue;
        ++deathmatchLosses_[hogs_[i].team];
        // Back as it began, unhurt, keeping what it did so far (its kills count).
        Hog fresh = startingHogs_[i];
        fresh.record = hogs_[i].record;
        fresh.health = fresh.maxHealth;
        hogs_[i] = std::move(fresh);
        GameEvent back;
        back.type = GameEventType::Respawned;
        back.hog = i;
        back.team = hogs_[i].team;
        events.push_back(back);
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

    if (hog.jetpackSecondsLeft > 0.0f) {
        // Flying a jetpack: fire again lets go of it (the wiki).
        if (fireJustPressed) dropJetpack(hogIndex, false, events);
        return;
    }
    if (chargeRetreat_) {
        hog.charging = false;  // TNT is burning: time to run, not to shoot
        return;
    }

    // Weapons are used standing on dry ground.
    const bool canUseWeapon = hog.alive && !isAirborne(hog) && hog.motion.state != MotionState::Swimming;
    if (!canUseWeapon) {
        hog.charging = false;
        return;
    }

    if (input.chooseWeapon && !hog.charging && *input.chooseWeapon < hog.weapons.size()) {
        hog.weaponSlot = *input.chooseWeapon;
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
    hog.aimDegrees = std::max(hog.aimDegrees, spec.lowestAimDegrees);  // a mortar lobs
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

void World::updateWeaponStance(Hog& hog) const {
    // As the original: the weapon is put away while the hog moves...
    const auto state = hog.motion.state;
    if (state != MotionState::Standing && state != MotionState::Turning) {
        hog.drawnWeapon.reset();
        hog.held.reset();
        return;
    }
    // ...and taken out again (with its "getting out" animation) when it
    // stands still, or when another weapon is chosen.
    const WeaponSpec& spec = weaponSpec(hog.selectedWeapon());
    if (hog.drawnWeapon != spec.id) {
        hog.drawnWeapon = spec.id;
        hog.held = HeldAnimation{spec.drawAnimation, AnimationPlayer::Repeat::Once};
        hog.animation.play(spec.drawAnimation, AnimationPlayer::Repeat::Once, true);
        return;
    }
    const bool drawing = hog.held && hog.held->animation == spec.drawAnimation &&
                         hog.held->repeat == AnimationPlayer::Repeat::Once &&
                         !(hog.animation.animation() == spec.drawAnimation && hog.animation.finished());
    if (drawing) return;
    // Then the aiming pose follows the aim (weapons without one keep the
    // last frame of taking the weapon out).
    if (spec.aimAnimation != kNoAnimation) {
        hog.held = HeldAnimation{spec.aimAnimation, AnimationPlayer::Repeat::Hold, aimPoseFrame(hog.aimDegrees)};
    }
}

void World::hurtSwimmers(double seconds, std::vector<GameEvent>& events) {
    const auto active = activeHog();
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        Hog& hog = hogs_[i];
        if (!hog.alive || hog.motion.state != MotionState::Swimming || swimsSafely(hog)) {
            hog.waterDamageOwed = 0.0f;
            hog.secondsSwumInItsTurn = 0.0f;
            continue;
        }
        // Steered into the water in its own turn: the damage ramps up.
        const bool steered = active == i && phase_ == MatchPhase::Playing;
        hog.secondsSwumInItsTurn = steered ? hog.secondsSwumInItsTurn + static_cast<float>(seconds) : 0.0f;
        hog.waterDamageOwed += swimDamagePerSecond(hog.secondsSwumInItsTurn) * static_cast<float>(seconds);
        // Charged in whole points, so the damage shows as whole numbers.
        const float whole = std::floor(hog.waterDamageOwed);
        if (whole >= 1.0f) {
            hog.waterDamageOwed -= whole;
            damageHog(hogs_, i, whole, events);
        }
    }
}

void World::armMines() {
    for (auto& projectile : projectiles_) projectile.armed = true;
    // The wiki: a mine put down on another one takes it for a hog and
    // goes off (both then go up, the second set off by the first).
    for (std::size_t i = 0; i < projectiles_.size(); ++i) {
        const WeaponSpec& spec = weaponSpec(projectiles_[i].weapon);
        if (spec.kind != WeaponKind::Placed || spec.triggerRadius <= 0.0f || projectiles_[i].bomblet) continue;
        for (std::size_t j = i + 1; j < projectiles_.size(); ++j) {
            const WeaponSpec& other = weaponSpec(projectiles_[j].weapon);
            if (other.kind == WeaponKind::Placed && other.triggerRadius > 0.0f &&
                glm::length(projectiles_[i].position - projectiles_[j].position) < kStackedMineDistance) {
                projectiles_[i].fuseSecondsLeft = kChainReactionSeconds;
            }
        }
    }
}

bool World::chargeBurning() const {
    return std::any_of(projectiles_.begin(), projectiles_.end(), [](const Projectile& projectile) {
        const WeaponSpec& spec = weaponSpec(projectile.weapon);
        return spec.kind == WeaponKind::Placed && spec.triggerRadius <= 0.0f && projectile.fuseSecondsLeft > 0.0f;
    });
}

void World::explodeDeadHogs(std::vector<GameEvent>& events) {
    // FUN_004680e0: once its dying animation is over, a dead hog blows up.
    // On land it leaves its boots behind (object 29, BOOTS), except in a
    // deathmatch, where it comes back; in water it sinks first.
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        Hog& hog = hogs_[i];
        const std::size_t animation = hog.animation.animation();
        const bool drowned = animation == kDrowningAnimation;
        const bool over = !hog.alive && hog.motion.state == MotionState::Dead && hog.animation.finished() &&
                          (drowned || isDyingAnimation(animation));
        if (!over || hog.blownUp) continue;
        hog.blownUp = true;
        const glm::vec3 centre = hog.position + glm::vec3(0.0f, kExplosionLift, 0.0f);
        GameEvent blown;
        blown.type = GameEventType::HogBlownUp;
        blown.hog = i;
        blown.position = hog.position;
        blown.amount = !drowned && !(turns_ && turns_->rules().deathmatchLimit > 0) ? 1.0f : 0.0f;
        events.push_back(blown);
        GameEvent explosion;
        explosion.type = GameEventType::Explosion;
        explosion.position = centre;
        events.push_back(explosion);
        addExplosion(centre, kDrownedBlastRadius);
        explode(hogs_, centre, kDrownedBlastRadius, kDrownedBlastDamage, 0.0f, events);
        damageObjects(centre, kDrownedBlastRadius, kDrownedBlastDamage, events);
    }
}

std::optional<glm::vec3> World::nearestShore(const glm::vec3& from) const {
    // Rings of tiles around the hog, nearest first.
    const auto& terrain = map_.terrain();
    constexpr int kSearchTiles = 32;
    constexpr float kTile = static_cast<float>(formats::kTileSize);
    for (int ring = 1; ring <= kSearchTiles; ++ring) {
        std::optional<glm::vec3> best;
        float bestDistance = 0.0f;
        for (int dz = -ring; dz <= ring; ++dz) {
            for (int dx = -ring; dx <= ring; ++dx) {
                if (std::max(std::abs(dx), std::abs(dz)) != ring) continue;
                const float x = from.x + static_cast<float>(dx) * kTile;
                const float z = from.z + static_cast<float>(dz) * kTile;
                if (!terrain.contains(x, z) || terrain.tileAt(x, z).isWater) continue;
                const float distance = glm::length(glm::vec2(x - from.x, z - from.z));
                if (!best || distance < bestDistance) {
                    best = glm::vec3(x, from.y, z);
                    bestDistance = distance;
                }
            }
        }
        if (best) return best;
    }
    return std::nullopt;
}

int World::stock(int team, WeaponId weapon) const {
    const auto found = stock_.find(team);
    if (found == stock_.end()) return 0;
    const auto count = found->second.find(weapon);
    return count == found->second.end() ? 0 : count->second;
}

void World::refreshStockWeapons(int team) {
    // Every hog of the team carries its own weapons, then the team's stock.
    for (std::size_t index = 0; index < hogs_.size(); ++index) {
        Hog& hog = hogs_[index];
        if (hog.team != team) continue;
        if (hog.sheltered()) {
            applyFooter(index);  // inside, only the object's weapons
            continue;
        }
        // A weapon in hand stays in hand; an action (the fist of a hog with
        // nothing else, SKIP TURN) gives way to the first weapon.
        const bool heldAction = !hog.weapons.empty() &&
                                hog.weaponSlot % hog.weapons.size() >=
                                    hog.weapons.size() - std::min(hog.footerActions, hog.weapons.size());
        const std::optional<WeaponId> selected =
            heldAction ? std::nullopt : std::optional<WeaponId>(hog.selectedWeapon());
        hog.footerActions = 0;  // dropped with the stock below, added again after
        if (hog.ownWeapons > hog.weapons.size()) hog.ownWeapons = hog.weapons.size();
        hog.weapons.resize(hog.ownWeapons);
        for (const auto& [weapon, count] : stock_[team]) {
            if (count > 0 && std::find(hog.weapons.begin(), hog.weapons.end(), weapon) == hog.weapons.end()) {
                hog.weapons.push_back(weapon);
            }
        }
        // Keep the same weapon in hand if it is still there.
        const auto kept = selected ? std::find(hog.weapons.begin(), hog.weapons.end(), *selected) : hog.weapons.end();
        hog.weaponSlot = kept != hog.weapons.end() ? static_cast<std::size_t>(kept - hog.weapons.begin()) : 0;
        applyFooter(index);
    }
}

void World::applyFooter(std::size_t hogIndex) {
    // The actions every hog has at the end of its list: its fists when it
    // has nothing else, getting into the building or vehicle beside it (or
    // out of the one it is in), and SKIP TURN, always in the menu's last
    // place as in the original (seen in a PS1 playthrough).
    Hog& hog = hogs_[hogIndex];
    std::vector<WeaponId> footer;
    const std::size_t kept = hog.weapons.size() - std::min(hog.footerActions, hog.weapons.size());
    if (kept == 0 && !hog.sheltered()) footer.push_back(WeaponId::Trotter);  // nothing else: its fists
    if (hog.sheltered()) {
        const bool vehicle = mountSpec(mounts_[*hog.mount].kind).vehicle;
        footer.push_back(vehicle ? WeaponId::VehicleInOut : WeaponId::BuildingInOut);
    } else if (const auto near = freeMountNear(hog)) {
        footer.push_back(mountSpec(mounts_[*near].kind).vehicle ? WeaponId::VehicleInOut : WeaponId::BuildingInOut);
    }
    if (std::find(hog.weapons.begin(), hog.weapons.begin() + static_cast<std::ptrdiff_t>(kept), WeaponId::SkipTurn) ==
        hog.weapons.begin() + static_cast<std::ptrdiff_t>(kept)) {
        footer.push_back(WeaponId::SkipTurn);
    }
    if (std::equal(hog.weapons.begin() + static_cast<std::ptrdiff_t>(kept), hog.weapons.end(), footer.begin(),
                   footer.end())) {
        return;  // already so
    }
    const WeaponId selected = hog.selectedWeapon();
    hog.weapons.resize(kept);
    hog.weapons.insert(hog.weapons.end(), footer.begin(), footer.end());
    hog.footerActions = footer.size();
    const auto still = std::find(hog.weapons.begin(), hog.weapons.end(), selected);
    hog.weaponSlot = still != hog.weapons.end() ? static_cast<std::size_t>(still - hog.weapons.begin()) : 0;
}

void World::useStock(std::size_t hogIndex) {
    Hog& hog = hogs_[hogIndex];
    if (hog.weapons.empty()) return;
    const std::size_t slot = hog.weaponSlot % hog.weapons.size();
    if (slot >= hog.weapons.size() - std::min(hog.footerActions, hog.weapons.size())) {
        return;  // an action the world adds (SKIP TURN, getting in or out)
    }
    if (hog.ownWeapons > hog.weapons.size() || slot < hog.ownWeapons) {
        useOwnWeapon(hog, slot);  // one of its own weapons
        return;
    }
    if (training_) return;  // the Boot Camp's crate weapons never run out
    int& count = stock_[hog.team][hog.selectedWeapon()];
    if (count > 0) --count;
    if (count == 0) refreshStockWeapons(hog.team);
}

void World::lowerFallingCrates(double seconds) {
    for (auto& pickup : pickups_) {
        if (!pickup.falling() || pickup.hidden) continue;
        pickup.dropHeight = std::max(0.0f, pickup.dropHeight - kCrateFallSpeed * static_cast<float>(seconds));
    }
}

void World::collectPickups(std::vector<GameEvent>& events) {
    for (std::size_t p = 0; p < pickups_.size(); ++p) {
        Pickup& pickup = pickups_[p];
        if (pickup.taken || pickup.hidden || pickup.falling()) continue;
        for (std::size_t h = 0; h < hogs_.size(); ++h) {
            Hog& hog = hogs_[h];
            if (!hog.alive || !touches(pickup, hog.position)) continue;
            pickup.taken = true;
            map_.removeObject(pickup.object);
            GameEvent event;
            event.type = GameEventType::PickedUp;
            event.hog = h;
            event.team = hog.team;
            event.pickup = p;
            event.position = pickup.centre;
            switch (pickup.kind) {
            case PickupKind::Health: {
                const float before = hog.health;
                const int amount = training_ ? kTrainingHealthCrate : pickup.amount;
                hog.health = std::min(hog.maxHealth, hog.health + static_cast<float>(amount));
                event.amount = hog.health - before;
                break;
            }
            case PickupKind::Weapon:
                event.amount = static_cast<float>(pickup.amount);
                if (const auto weapon = pickup.weapon()) {
                    event.weapon = *weapon;
                    // In training, crate weapons never run out (useStock).
                    stock_[hog.team][*weapon] += training_ ? 1 : pickup.amount;
                    refreshStockWeapons(hog.team);
                }
                break;
            case PickupKind::PromotionPoints:
                event.amount = static_cast<float>(pickup.amount);  // the campaign counts them
                break;
            }
            events.push_back(event);
            break;
        }
    }
}

void World::setObjectives(std::vector<Objective> objectives, std::size_t objectCount) {
    objectives_ = std::move(objectives);
    nextDropObject_ = objectCount;
}

void World::updateObjectives(std::vector<GameEvent>& events) {
    // Hogs that died and objects destroyed in this step: their objectives
    // are done (or, for a hog that had somewhere to go, lost). The list grows as rewards are
    // reported, hence the index.
    for (std::size_t e = 0; e < events.size(); ++e) {
        std::optional<std::size_t> gone;
        if (events[e].type == GameEventType::Died && events[e].hog < hogs_.size()) gone = hogs_[events[e].hog].object;
        if (events[e].type == GameEventType::ObjectDestroyed) gone = events[e].object;
        // A crate picked up counts as gone too (FUN_00464630).
        if (events[e].type == GameEventType::PickedUp && events[e].pickup < pickups_.size()) {
            gone = pickups_[events[e].pickup].object;
        }
        if (!gone) continue;
        if (const auto result = destroyObjective(objectives_, *gone)) {
            objectiveDone(*result, events);
        }
    }

    // The active hog walking into its target (only in its own turn, as in
    // FUN_004a7730, which checks for the player's current hog).
    const auto active = activeHog();
    if (!active || phase_ != MatchPhase::Playing) return;
    const Hog& hog = hogs_[*active];
    if (!hog.alive) return;
    for (auto& objective : objectives_) {
        if (objective.object != hog.object) continue;
        if (const auto result = reachObjective(objective, hog.position)) {
            objectiveDone(*result, events);
        }
    }
}

void World::setTraining(bool training) {
    training_ = training;
    if (!training) return;
    // The Boot Camp's hog starts with nothing: crates bring the weapons.
    for (auto& hog : hogs_) {
        hog.weapons.clear();
        hog.ownWeapons = 0;
        hog.weaponSlot = 0;
    }
}

void World::hideObjects(const std::vector<std::size_t>& objects) {
    for (const std::size_t object : objects) {
        hiddenBoxes_[object] = map_.takeObject(object);
        for (std::size_t p = 0; p < pickups_.size(); ++p) {
            if (pickups_[p].object != object) continue;
            pickups_[p].hidden = true;
            trainingCrates_.push_back(p);  // the crates of the chain
        }
    }
}

void World::reveal(std::size_t object, std::vector<GameEvent>& events) {
    const auto hidden = hiddenBoxes_.find(object);
    if (hidden == hiddenBoxes_.end()) return;  // already in sight
    map_.addBoxes(hidden->second);
    hiddenBoxes_.erase(hidden);

    GameEvent revealed;
    revealed.type = GameEventType::ObjectRevealed;
    revealed.object = object;
    events.push_back(revealed);
    for (auto& pickup : pickups_) {
        if (pickup.object != object) continue;
        pickup.hidden = false;
        pickup.dropHeight = kCrateDropHeight;  // crates come down by parachute
        if (training_) {
            if (const auto tip = tipWhenCrateAppears(pickup)) trainingTip(*tip, events);
        }
    }
}

void World::trainingTip(const TrainingTip& tip, std::vector<GameEvent>& events) {
    GameEvent event;
    event.type = GameEventType::TrainingTip;
    event.text = tip.text;
    event.speechLine = tip.line;
    events.push_back(event);
}

void World::trainingTips(std::optional<std::size_t> active, std::vector<GameEvent>& events) {
    if (!trainingStarted_) {
        trainingStarted_ = true;
        trainingTip(trainingStartTip(), events);
    }
    // Crates picked up in this step (the list grows as tips are added).
    for (std::size_t e = 0; e < events.size(); ++e) {
        if (events[e].type != GameEventType::PickedUp || events[e].pickup >= pickups_.size()) continue;
        if (const auto tip = tipWhenCratePicked(pickups_[events[e].pickup])) trainingTip(*tip, events);
    }
    // How to use a weapon, the first time it is in hand.
    if (active && hogs_[*active].alive && !hogs_[*active].weapons.empty()) {
        const WeaponId weapon = hogs_[*active].selectedWeapon();
        if (!weaponsTipped_.contains(weapon)) {
            weaponsTipped_.insert(weapon);
            if (const auto tip = tipWhenWeaponTaken(weapon)) trainingTip(*tip, events);
        }
    }
}

bool World::trainingComplete() const {
    if (!training_ || trainingCrates_.empty()) return false;
    return std::all_of(trainingCrates_.begin(), trainingCrates_.end(),
                       [&](std::size_t p) { return pickups_[p].taken; });
}

void World::recordDeeds(std::optional<std::size_t> active, const std::vector<GameEvent>& events) {
    for (const auto& event : events) {
        if (!active || event.hog >= hogs_.size()) continue;
        if (event.type != GameEventType::Hurt && event.type != GameEventType::Died) continue;
        HogRecord& record = hogs_[*active].record;
        const bool friendly = hogs_[event.hog].team == hogs_[*active].team;
        if (event.type == GameEventType::Hurt) {
            (friendly ? record.friendDamage : record.enemyDamage) += event.amount;
        } else if (friendly) {
            ++record.friendsKilled;
        } else {
            ++record.enemiesKilled;
            ++record.killsThisTurn;
        }
    }
}

void World::objectiveDone(const ObjectiveResult& result, std::vector<GameEvent>& events) {
    if (result.revealGroup) {
        std::vector<std::size_t> crates;
        for (const auto& pickup : pickups_) crates.push_back(pickup.object);
        for (const std::size_t object : revealGroup(objectives_, *result.revealGroup, crates)) {
            reveal(object, events);
        }
    }
    if (!result.drop && result.pointsLost == 0 && result.sergeantSection < 0) return;  // only revealed things
    GameEvent event;
    event.type = GameEventType::ObjectiveDone;
    event.amount = static_cast<float>(result.pointsLost);
    event.sergeantSection = result.sergeantSection;
    if (result.drop) {
        Pickup drop = *result.drop;
        drop.object = nextDropObject_++;
        drop.dropHeight = kCrateDropHeight;  // it comes down by parachute
        event.position = drop.centre;
        event.pickup = pickups_.size();
        event.dropped = true;
        pickups_.push_back(drop);
    }
    events.push_back(event);
}

void World::damageObjects(const glm::vec3& centre, float radius, float damage, std::vector<GameEvent>& events) {
    if (radius <= 0.0f) return;
    // As for hogs (explode): all of the damage at the centre, none at the
    // edge, measured to the nearest point of the object's box. The box
    // list shrinks as objects go, so the hits are gathered first.
    std::vector<std::pair<std::size_t, float>> hits;
    for (const auto& box : map_.boxes()) {
        const float distance = box.distanceTo(centre);
        if (distance < radius) hits.emplace_back(box.object, damage * (1.0f - distance / radius));
    }
    for (const auto& [object, amount] : hits) {
        damageMapObject(object, amount, events);
    }
}

namespace {
// How close to the top of an object a hog's feet must be for it to count
// as standing on it.
constexpr float kStandingTolerance = 16.0f;
} // namespace

void World::damageMapObject(std::size_t object, float amount, std::vector<GameEvent>& events) {
    const auto target = std::find_if(destructibles_.begin(), destructibles_.end(),
                                     [&](const Destructible& d) { return d.object == object; });
    if (target == destructibles_.end()) return;
    const float healthBefore = target->health;
    if (!damageObject(*target, amount)) return;

    const auto box = std::find_if(map_.boxes().begin(), map_.boxes().end(),
                                  [&](const CollisionBox& b) { return b.object == object; });
    const glm::vec3 centre = box != map_.boxes().end() ? box->centre : glm::vec3(0.0f);

    if (target->objectClass == ObjectClass::Drum) {
        // It catches fire and blows up later (burnDrums).
        burningDrums_.push_back({object, centre, kDrumFuseSeconds, target->poison});
        return;
    }
    if (target->objectClass == ObjectClass::Crate) {
        // What it held is lost with it.
        for (auto& pickup : pickups_) {
            if (pickup.object == object) pickup.taken = true;
        }
    }
    if (target->objectClass == ObjectClass::Scenery && box != map_.boxes().end()) {
        // FUN_0048d990: the damage the object could not take, in whole
        // points, goes to a hog standing on it, which then falls.
        const float leftover = std::floor(amount - healthBefore);
        if (leftover > 0.0f) {
            for (std::size_t h = 0; h < hogs_.size(); ++h) {
                const Hog& hog = hogs_[h];
                const bool onTop = hog.alive && box->footprintDistance(hog.position.x, hog.position.z) <= 0.0f &&
                                   std::abs(hog.position.y - box->top()) < kStandingTolerance;
                if (onTop) damageHog(hogs_, h, leftover, events);
            }
        }
    }
    removeDestroyedObject(*target, events);
}

void World::removeDestroyedObject(const Destructible& target, std::vector<GameEvent>& events) {
    const std::size_t object = target.object;
    GameEvent destroyed;
    destroyed.type = GameEventType::ObjectDestroyed;
    destroyed.object = object;
    const bool blowsUp = target.blowsUp() || target.objectClass == ObjectClass::Drum;
    destroyed.amount = blowsUp ? 1.0f : 0.0f;
    for (const auto& box : map_.boxes()) {
        if (box.object == object) {
            destroyed.position = box.centre;
            if (target.blowsUp()) {
                addExplosion(box.centre, std::max({box.halfSize.x, box.halfSize.y, box.halfSize.z}));
            }
            break;
        }
    }
    map_.removeObject(object);  // whatever stood on it falls
    events.push_back(destroyed);

    // A building or vehicle with a hog inside throws it out, hurt.
    for (auto& mount : mounts_) {
        if (mount.object != object) continue;
        mount.destroyed = true;
        if (mount.occupant) {
            const std::size_t inside = *mount.occupant;
            leaveMount(inside, events);
            damageHog(hogs_, inside, mountSpec(mount.kind).damageWhenDestroyed, events);
        }
    }
}

void World::addExplosion(const glm::vec3& centre, float radius) {
    explosions_.push_back({centre, radius, 0.0f, ++explosionCount_});
}

void World::burnDrums(double seconds, std::vector<GameEvent>& events) {
    // A drum on fire blows up after its fuse (the wiki's Explosive barrel
    // page): up to 75 points around it, or 15 and poison close by for a
    // poison drum. Its blast can set other drums off.
    std::vector<BurningDrum> due;
    for (auto& drum : burningDrums_) {
        drum.secondsLeft -= static_cast<float>(seconds);
        if (drum.secondsLeft <= 0.0f) due.push_back(drum);
    }
    std::erase_if(burningDrums_, [](const BurningDrum& drum) { return drum.secondsLeft <= 0.0f; });
    for (const auto& drum : due) {
        const auto target = std::find_if(destructibles_.begin(), destructibles_.end(),
                                         [&](const Destructible& d) { return d.object == drum.object; });
        if (target != destructibles_.end()) removeDestroyedObject(*target, events);
        GameEvent explosion;
        explosion.type = GameEventType::Explosion;
        explosion.position = drum.centre;
        events.push_back(explosion);
        const float radius = drum.poison ? kPoisonDrumRadius : kDrumBlastRadius;
        const float damage = drum.poison ? kPoisonDrumDamage : kDrumBlastDamage;
        addExplosion(drum.centre, radius);
        explode(hogs_, drum.centre, radius, damage, drum.poison ? 0.0f : kDrumKnockback, events);
        if (drum.poison) {
            for (std::size_t h = 0; h < hogs_.size(); ++h) {
                if (hogs_[h].alive && glm::distance(hogCentre(hogs_[h]), drum.centre) < radius) {
                    afflict(h, WeaponEffect::Poison);
                }
            }
        } else {
            damageObjects(drum.centre, radius, damage, events);
        }
    }
}

void World::fire(std::size_t hogIndex, float power, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[hogIndex];
    const WeaponSpec& spec = weaponSpec(hog.selectedWeapon());
    useStock(hogIndex);
    // Any weapon used arms the mines put down before it (the wiki).
    armMines();
    // The wiki's explosives: the first mine of a turn does not use the
    // turn up, and TNT leaves the hog its fuse's time to get away.
    const bool placed = spec.kind == WeaponKind::Placed;
    const bool firstMine = placed && spec.triggerRadius > 0.0f && minesThisTurn_ == 0;
    const bool charge = placed && spec.fuseSeconds > 0.0f;

    // Guns fire from the aiming pose; other weapons have their own motion.
    if (spec.useAnimation != kNoAnimation) {
        hog.held = HeldAnimation{spec.useAnimation, AnimationPlayer::Repeat::Once};
        hog.animation.play(spec.useAnimation, AnimationPlayer::Repeat::Once, true);
    }
    if (spec.endsTurn && !firstMine && !charge) {
        phase_ = MatchPhase::Resolving;
        settleSecondsLeft_ = kSettleSeconds;
    } else {
        fireWasHeld_ = true;  // used once per press; the turn goes on
    }
    if (charge) chargeRetreat_ = true;

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
        Projectile explosive = launchProjectile(spec.id, hogIndex, spot, glm::vec3(0.0f), 0.0f);
        if (spec.triggerRadius > 0.0f) {
            explosive.armed = false;  // harmless until the turn ends or another weapon is used
            ++minesThisTurn_;
        }
        projectiles_.push_back(explosive);
        break;
    }
    case WeaponKind::Heal:
        events.push_back(fired);
        heal(hogIndex, spec, events);
        break;
    case WeaponKind::Suicide: {
        // The hog goes up like a charge at its feet, taking itself with it.
        events.push_back(fired);
        const glm::vec3 centre = hog.position + glm::vec3(0.0f, kExplosionLift, 0.0f);
        GameEvent explosion;
        explosion.type = GameEventType::Explosion;
        explosion.position = centre;
        explosion.weapon = spec.id;
        events.push_back(explosion);
        addExplosion(centre, spec.blastRadius);
        damageHog(hogs_, hogIndex, hogs_[hogIndex].health, events);
        explode(hogs_, centre, spec.blastRadius, spec.damage, spec.knockback, events);
        damageObjects(centre, spec.blastRadius, spec.damage, events);
        break;
    }
    case WeaponKind::SkipTurn:
        break;  // the turn ends as after any shot
    case WeaponKind::Surrender:
        // The whole team gives up.
        for (std::size_t i = 0; i < hogs_.size(); ++i) {
            if (hogs_[i].team != hog.team || !hogs_[i].alive) continue;
            hogs_[i].hideShield = 0.0f;
            damageHog(hogs_, i, hogs_[i].health, events);
        }
        break;
    case WeaponKind::Shockwave:
        events.push_back(fired);
        shockwave(hogIndex, spec, events);
        break;
    case WeaponKind::Hide: {
        // In a disguise until its next turn (the wiki: it takes 20 points).
        events.push_back(fired);
        hog.hideShield = spec.damage;
        GameEvent hid;
        hid.type = GameEventType::HogHidden;
        hid.hog = hogIndex;
        hid.position = hog.position;
        events.push_back(hid);
        break;
    }
    case WeaponKind::PickPocket:
        events.push_back(fired);
        pickPocket(hogIndex, spec, events);
        break;
    case WeaponKind::Jetpack:
        // It takes off along its aim (the wiki: "in the direction set by
        // the angle"); the turn keys steer it until fire is pressed again
        // or the fuel runs out (dropJetpack). The turn goes on afterwards.
        events.push_back(fired);
        hog.jetpackSecondsLeft = spec.fuseSeconds;
        hog.motion.state = MotionState::Airborne;
        hog.motion.velocity = glm::vec3(0.0f);
        hog.motion.jumped = false;
        hog.motion.highestPoint = hog.position.y;
        hog.drawnWeapon.reset();
        hog.held.reset();
        break;
    case WeaponKind::SpecialOps: {
        // Dropped by parachute on the chosen spot.
        events.push_back(fired);
        const glm::vec3 spot = targetSpot(hog, spec, power);
        hog.position = spot + glm::vec3(0.0f, kDropHeight, 0.0f);
        hog.motion.state = MotionState::Airborne;
        hog.motion.velocity = glm::vec3(0.0f);
        hog.motion.jumped = false;
        hog.motion.highestPoint = hog.position.y;
        hog.parachute = true;
        hog.drawnWeapon.reset();
        hog.held.reset();
        break;
    }
    case WeaponKind::InOut:
        // In or out of the building or vehicle; the turn goes on.
        events.push_back(fired);
        if (hog.sheltered()) {
            leaveMount(hogIndex, events);
        } else if (const auto near = freeMountNear(hog)) {
            enterMount(hogIndex, *near, events);
        }
        break;
    case WeaponKind::MapView:
    case WeaponKind::Binoculars: {
        // Switched on, or off again.
        const auto wanted = spec.kind == WeaponKind::MapView ? Hog::Looking::AtTheMap : Hog::Looking::ThroughBinoculars;
        hog.looking = hog.looking == wanted ? Hog::Looking::Normally : wanted;
        break;
    }
    case WeaponKind::Airstrike: {
        // Bombs fall in a line across the chosen spot, along the hog's aim.
        events.push_back(fired);
        const glm::vec3 spot = targetSpot(hog, spec, power);
        const glm::vec3 along = hogFacing(hog);
        const float spacing = spec.blastRadius * kAirstrikeSpacing;
        for (int i = 0; i < spec.shots; ++i) {
            const float offset = (static_cast<float>(i) - static_cast<float>(spec.shots - 1) / 2.0f) * spacing;
            glm::vec3 drop = spot + along * offset;
            drop.y = map_.groundHeight(drop.x, drop.z, drop.y + kDropHeight) + kDropHeight +
                     static_cast<float>(i) * 60.0f;  // one after the other
            Projectile bomb = launchProjectile(spec.id, hogIndex, drop, glm::vec3(0.0f), 0.0f);
            bomb.velocity = glm::vec3(0.0f, -kAirstrikeFallSpeed, 0.0f);
            projectiles_.push_back(bomb);
        }
        break;
    }
    }
}

std::optional<std::size_t> World::freeMountNear(const Hog& hog) const {
    if (!hog.alive || hog.sheltered()) return std::nullopt;
    const glm::vec3 body = hog.position + glm::vec3(0.0f, 50.0f, 0.0f);
    for (std::size_t i = 0; i < mounts_.size(); ++i) {
        const Mount& mount = mounts_[i];
        if (mount.destroyed || mount.occupant) continue;
        if (const CollisionBox* box = mountBox(mount); box && box->distanceTo(body) <= kMountReach) return i;
    }
    return std::nullopt;
}

const CollisionBox* World::mountBox(const Mount& mount) const {
    for (const auto& box : map_.boxes()) {
        if (box.object == mount.object) return &box;
    }
    return nullptr;
}

void World::enterMount(std::size_t hogIndex, std::size_t mountIndex, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[hogIndex];
    Mount& mount = mounts_[mountIndex];
    const CollisionBox* box = mountBox(mount);
    if (!box) return;
    const MountSpec& spec = mountSpec(mount.kind);

    // Its own weapons are put by; the object's take their place.
    const std::size_t own = hog.weapons.size() - std::min(hog.footerActions, hog.weapons.size());
    mount.savedWeapons.assign(hog.weapons.begin(), hog.weapons.begin() + static_cast<std::ptrdiff_t>(own));
    mount.savedOwnWeapons = hog.ownWeapons;
    mount.savedAmmo = hog.ownAmmo;
    mount.savedSlot = hog.weaponSlot;
    hog.weapons.clear();
    hog.ownAmmo.clear();
    for (const auto& [weapon, uses] : spec.arsenal) {
        if (uses == 0) {
            hog.weapons.push_back(weapon);
        } else if (const auto left = mount.ammo.find(weapon); left != mount.ammo.end() && left->second > 0) {
            hog.weapons.push_back(weapon);
            hog.ownAmmo[weapon] = left->second;
        }
    }
    hog.ownWeapons = hog.weapons.size();
    hog.footerActions = 0;
    hog.weaponSlot = 0;

    mount.occupant = hogIndex;
    hog.mount = mountIndex;
    hog.position = glm::vec3(box->centre.x, box->bottom(), box->centre.z);
    hog.motion.state = MotionState::Standing;
    hog.motion.velocity = glm::vec3(0.0f);
    if (spec.drivable) hog.yawRadians = box->yawRadians;  // facing the way it drives
    hog.aimDegrees = std::clamp(hog.aimDegrees, spec.lowestAimDegrees, spec.highestAimDegrees);
    hog.mountMuzzleHeight = box->halfSize.y * 1.4f;
    hog.mountMuzzleReach = std::max(box->halfSize.x, box->halfSize.z) + kMountMuzzleClearance;
    hog.drawnWeapon.reset();
    hog.held.reset();
    hog.hideShield = 0.0f;
    applyFooter(hogIndex);

    GameEvent mounted;
    mounted.type = GameEventType::HogMounted;
    mounted.hog = hogIndex;
    mounted.object = mount.object;
    mounted.position = hog.position;
    events.push_back(mounted);
}

void World::leaveMount(std::size_t hogIndex, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[hogIndex];
    if (!hog.mount) return;
    Mount& mount = mounts_[*hog.mount];
    const MountSpec& spec = mountSpec(mount.kind);

    // The shots it did not fire stay with the object.
    for (const auto& [weapon, uses] : spec.arsenal) {
        if (uses == 0) continue;
        const auto left = hog.ownAmmo.find(weapon);
        mount.ammo[weapon] = left != hog.ownAmmo.end() ? left->second : 0;
    }
    hog.weapons = mount.savedWeapons;
    hog.ownWeapons = mount.savedOwnWeapons;
    hog.ownAmmo = mount.savedAmmo;
    hog.weaponSlot = mount.savedSlot;
    hog.footerActions = 0;

    // Out beside it, on the side it faces if there is room.
    glm::vec3 spot = hog.position;
    if (const CollisionBox* box = mountBox(mount)) {
        const float reach = std::max(box->halfSize.x, box->halfSize.z) + kHogRadius + 20.0f;
        const glm::vec3 facing = hogFacing(hog);
        const glm::vec3 side(-facing.z, 0.0f, facing.x);
        for (const glm::vec3 way : {facing, side, -side, -facing}) {
            const glm::vec3 candidate = glm::vec3(box->centre.x, 0.0f, box->centre.z) + way * reach;
            if (!map_.terrain().contains(candidate.x, candidate.z)) continue;
            const float ground = map_.groundHeight(candidate.x, candidate.z, box->top());
            if (!map_.hitsObstacle(glm::vec3(candidate.x, ground + 50.0f, candidate.z))) {
                spot = glm::vec3(candidate.x, ground, candidate.z);
                break;
            }
        }
    }
    hog.mount.reset();
    mount.occupant.reset();
    hog.position = spot;
    hog.motion.state = MotionState::Standing;
    hog.drawnWeapon.reset();
    hog.held.reset();
    refreshStockWeapons(hog.team);

    GameEvent dismounted;
    dismounted.type = GameEventType::HogDismounted;
    dismounted.hog = hogIndex;
    dismounted.object = mount.object;
    dismounted.position = hog.position;
    events.push_back(dismounted);
}

void World::driveMount(std::size_t hogIndex, const MovementInput& input, double seconds) {
    // Tanks and carriers: the movement keys drive them; its box moves with it.
    Hog& hog = hogs_[hogIndex];
    Mount& mount = mounts_[*hog.mount];
    const auto dt = static_cast<float>(seconds);
    const int turn = (input.turnLeft ? 1 : 0) - (input.turnRight ? 1 : 0);
    const float dyaw = static_cast<float>(turn) * glm::radians(kDriveTurnDegreesPerSecond) * dt;
    const int direction = (input.forward ? 1 : 0) - (input.backward ? 1 : 0);
    if (turn == 0 && direction == 0) return;

    auto boxes = map_.takeObject(mount.object);
    if (boxes.empty()) return;
    const float yaw = hog.yawRadians + dyaw;
    const glm::vec3 facing{std::cos(yaw), 0.0f, -std::sin(yaw)};
    const glm::vec3 step = facing * (kDriveSpeed * dt * static_cast<float>(direction));
    const glm::vec3 centre = boxes.front().centre + step;
    const float reach = std::max(boxes.front().halfSize.x, boxes.front().halfSize.z);
    const glm::vec3 front = centre + facing * (reach * static_cast<float>(direction == 0 ? 0 : direction));
    const auto& terrain = map_.terrain();
    const float centreX = (terrain.minX() + terrain.maxX()) / 2.0f;
    const float centreZ = (terrain.minZ() + terrain.maxZ()) / 2.0f;
    const bool blocked =
        !terrain.contains(front.x, front.z) || std::abs(front.x - centreX) > kPlayAreaHalfSize ||
        std::abs(front.z - centreZ) > kPlayAreaHalfSize || terrain.tileAt(front.x, front.z).isWater ||
        map_.hitsObstacle(glm::vec3(front.x, terrain.heightAt(front.x, front.z) + 50.0f, front.z));
    const glm::vec3 move = blocked ? glm::vec3(0.0f) : step;
    for (auto& box : boxes) {
        box.centre += move;
        box.centre.y = terrain.heightAt(box.centre.x, box.centre.z) + box.halfSize.y;
        box.yawRadians += dyaw;
    }
    map_.addBoxes(boxes);
    mount.moved += move;
    mount.moved.y = boxes.front().centre.y - mount.origin.y;
    mount.turned += dyaw;
    hog.yawRadians = yaw;
    hog.position = glm::vec3(boxes.front().centre.x, boxes.front().bottom(), boxes.front().centre.z);
}

void World::setMounts(const std::vector<std::pair<std::size_t, MountKind>>& mounts) {
    mounts_.clear();
    for (const auto& [object, kind] : mounts) {
        Mount mount;
        mount.object = object;
        mount.kind = kind;
        for (const auto& [weapon, uses] : mountSpec(kind).arsenal) {
            if (uses > 0) mount.ammo[weapon] = uses;
        }
        if (const CollisionBox* box = mountBox(mount)) mount.origin = box->centre;
        mounts_.push_back(std::move(mount));
    }
    // Hogs the map puts inside one start in it (the pillboxes' gunners).
    std::vector<GameEvent> ignored;
    for (std::size_t h = 0; h < hogs_.size(); ++h) {
        if (!hogs_[h].alive || hogs_[h].sheltered()) continue;
        for (std::size_t m = 0; m < mounts_.size(); ++m) {
            const CollisionBox* box = mountBox(mounts_[m]);
            if (!mounts_[m].occupant && box && box->footprintDistance(hogs_[h].position.x, hogs_[h].position.z) == 0.0f) {
                enterMount(h, m, ignored);
                break;
            }
        }
    }
    for (std::size_t h = 0; h < hogs_.size(); ++h) applyFooter(h);
}

std::optional<SightView> World::binocularView() const {
    const auto active = activeHog();
    if (!active || phase_ != MatchPhase::Playing) return std::nullopt;
    const Hog& hog = hogs_[*active];
    if (hog.looking != Hog::Looking::ThroughBinoculars) return std::nullopt;
    return SightView{hog.position + glm::vec3(0.0f, kEyeHeight, 0.0f), aimDirection(hog)};
}

bool World::mapView() const {
    const auto active = activeHog();
    return active && phase_ == MatchPhase::Playing && hogs_[*active].looking == Hog::Looking::AtTheMap;
}

glm::vec3 World::targetSpot(const Hog& hog, const WeaponSpec& spec, float power) const {
    // Straight ahead, as far as the charged power, kept inside the play
    // area and on the ground.
    const auto& terrain = map_.terrain();
    const float centreX = (terrain.minX() + terrain.maxX()) / 2.0f;
    const float centreZ = (terrain.minZ() + terrain.maxZ()) / 2.0f;
    glm::vec3 spot = hog.position + hogFacing(hog) * (spec.range * std::clamp(power, kMinimumPower, 1.0f));
    spot.x = std::clamp(spot.x, centreX - kPlayAreaHalfSize, centreX + kPlayAreaHalfSize);
    spot.z = std::clamp(spot.z, centreZ - kPlayAreaHalfSize, centreZ + kPlayAreaHalfSize);
    spot.y = map_.groundHeight(spot.x, spot.z, spot.y + kDropHeight);
    return spot;
}

void World::flyJetpack(std::size_t index, const MovementInput& input, double seconds,
                       const std::vector<formats::Animation>& animations, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[index];
    const auto dt = static_cast<float>(seconds);
    hog.animation.update(seconds, animations);
    if (hog.animation.animation() != kFallAnimation) {
        hog.animation.play(kFallAnimation, AnimationPlayer::Repeat::Loop);
    }
    const int turn = (input.turnLeft ? 1 : 0) - (input.turnRight ? 1 : 0);
    hog.yawRadians += static_cast<float>(turn) * glm::radians(kTurnDegreesPerSecond) * dt;

    // Along the aim it took off with: level, climbing or diving.
    const float pitch = glm::radians(hog.aimDegrees);
    const glm::vec3 along = hogFacing(hog) * std::cos(pitch) + glm::vec3(0.0f, std::sin(pitch), 0.0f);
    glm::vec3 next = hog.position + along * (kJetpackSpeed * dt);
    const auto& terrain = map_.terrain();
    const float centreX = (terrain.minX() + terrain.maxX()) / 2.0f;
    const float centreZ = (terrain.minZ() + terrain.maxZ()) / 2.0f;
    const bool outside = std::abs(next.x - centreX) > kPlayAreaHalfSize || std::abs(next.z - centreZ) > kPlayAreaHalfSize;
    if (outside || map_.hitsObstacle(next)) {
        next.x = hog.position.x;  // held back at the edge or by a wall
        next.z = hog.position.z;
    }
    const float ground = map_.groundHeight(next.x, next.z, next.y);
    hog.position = glm::vec3(next.x, std::max(next.y, ground), next.z);
    hog.motion.highestPoint = hog.position.y;
    if (next.y <= ground) {
        dropJetpack(index, true, events);  // flown into the ground: a crash landing
        return;
    }
    hog.jetpackSecondsLeft -= dt;
    if (hog.jetpackSecondsLeft <= 0.0f) dropJetpack(index, false, events);  // out of fuel
}

void World::dropJetpack(std::size_t index, bool crashed, std::vector<GameEvent>& events) {
    Hog& hog = hogs_[index];
    const float pitch = glm::radians(hog.aimDegrees);
    const glm::vec3 along = hogFacing(hog) * std::cos(pitch) + glm::vec3(0.0f, std::sin(pitch), 0.0f);
    hog.jetpackSecondsLeft = 0.0f;
    hog.motion.velocity = glm::vec3(0.0f);
    Projectile jetpack = launchProjectile(WeaponId::Jetpack, index, hog.position, glm::vec3(0.0f), 0.0f);
    if (crashed) {
        // The wiki: landing without the parachute hurts (the jetpack goes
        // off under the hog).
        hog.motion.state = MotionState::Airborne;  // lands at once, unhurt by the fall itself
        detonate(jetpack, events);
        return;
    }
    // The wiki: the released jetpack flies backwards and explodes where it
    // comes down; the hog floats down under its parachute.
    hog.parachute = true;
    jetpack.position += glm::vec3(0.0f, kHogHeight / 2.0f, 0.0f);
    jetpack.velocity = -along * (kJetpackSpeed * kDroppedJetpackSpeedShare);
    projectiles_.push_back(jetpack);
}

void World::steerGuided(const PlayerInput& input, double seconds) {
    // The player who fired a guided missile steers it while it flies.
    const auto active = activeHog();
    if (!active) return;
    const float step = glm::radians(kGuidedDegreesPerSecond) * static_cast<float>(seconds);
    const int yaw = (input.movement.turnLeft ? 1 : 0) - (input.movement.turnRight ? 1 : 0);
    const int pitch = (input.aimUp ? 1 : 0) - (input.aimDown ? 1 : 0);
    for (auto& projectile : projectiles_) {
        if (projectile.owner != *active || weaponSpec(projectile.weapon).guidance != Guidance::Guided) continue;
        steerProjectile(projectile, static_cast<float>(yaw) * step, static_cast<float>(pitch) * step);
    }
}

void World::shockwave(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events) {
    // Every other hog within reach is thrown up, and a little away.
    const glm::vec3 origin = hogs_[hogIndex].position;
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        if (i == hogIndex || !hogs_[i].alive) continue;
        glm::vec3 away = hogs_[i].position - origin;
        away.y = 0.0f;
        const float distance = glm::length(away);
        if (distance > spec.range) continue;
        const glm::vec3 direction = distance > 1.0f ? away / distance : glm::vec3(1.0f, 0.0f, 0.0f);
        knockHog(hogs_[i], direction * (spec.knockback * 0.3f) + glm::vec3(0.0f, spec.knockback, 0.0f));
        damageHog(hogs_, i, spec.damage, events);
    }
}

void World::pickPocket(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events) {
    // The nearest enemy within reach loses one use of a weapon of its own,
    // chosen at random, which the thief may use once (the wiki).
    Hog& thief = hogs_[hogIndex];
    std::optional<std::size_t> victim;
    float nearest = spec.range;
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        if (!hogs_[i].alive || hogs_[i].team == thief.team) continue;
        const float distance = glm::length(hogs_[i].position - thief.position);
        if (distance <= nearest) {
            nearest = distance;
            victim = i;
        }
    }
    if (!victim) return;
    Hog& target = hogs_[*victim];
    const std::size_t own = target.ownCount();
    std::vector<std::size_t> candidates;
    for (std::size_t w = 0; w < own; ++w) {
        if (target.weapons[w] != WeaponId::Trotter) candidates.push_back(w);
    }
    if (candidates.empty()) return;
    const std::size_t slot = candidates[random_() % candidates.size()];
    const WeaponId weapon = target.weapons[slot];
    useOwnWeapon(target, slot);

    // The thief carries it among its own weapons, with one use.
    const std::size_t thiefOwn = thief.ownCount();
    const auto had = std::find(thief.weapons.begin(), thief.weapons.begin() + static_cast<std::ptrdiff_t>(thiefOwn), weapon);
    if (had == thief.weapons.begin() + static_cast<std::ptrdiff_t>(thiefOwn)) {
        thief.weapons.insert(thief.weapons.begin() + static_cast<std::ptrdiff_t>(thiefOwn), weapon);
        if (thief.ownWeapons != static_cast<std::size_t>(-1)) ++thief.ownWeapons;
        thief.ownAmmo[weapon] = 1;
    } else if (thief.ownAmmo.contains(weapon)) {
        ++thief.ownAmmo[weapon];
    }

    GameEvent stolen;
    stolen.type = GameEventType::PickedPocket;
    stolen.hog = *victim;
    stolen.weapon = weapon;
    stolen.position = target.position;
    events.push_back(stolen);
}

void World::useOwnWeapon(Hog& hog, std::size_t slot) {
    // One use of a weapon of its own: those with a number of uses run out.
    const WeaponId weapon = hog.weapons[slot];
    const auto ammo = hog.ownAmmo.find(weapon);
    if (ammo == hog.ownAmmo.end()) return;  // never runs out
    if (--ammo->second > 0) return;
    hog.ownAmmo.erase(ammo);
    const WeaponId selected = hog.selectedWeapon();
    hog.weapons.erase(hog.weapons.begin() + static_cast<std::ptrdiff_t>(slot));
    if (hog.ownWeapons != static_cast<std::size_t>(-1) && hog.ownWeapons > 0) --hog.ownWeapons;
    const auto kept = std::find(hog.weapons.begin(), hog.weapons.end(), selected);
    hog.weaponSlot = kept != hog.weapons.end() ? static_cast<std::size_t>(kept - hog.weapons.begin()) : 0;
}

void World::heal(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events) {
    // Self heal helps the hog itself; healing hands the nearest living
    // friend within reach (or the hog itself when there is none).
    std::size_t patient = hogIndex;
    if (spec.range > 0.0f) {
        float nearest = spec.range;
        for (std::size_t i = 0; i < hogs_.size(); ++i) {
            if (i == hogIndex || !hogs_[i].alive || hogs_[i].team != hogs_[hogIndex].team) continue;
            const float distance = glm::length(hogs_[i].position - hogs_[hogIndex].position);
            if (distance <= nearest) {
                nearest = distance;
                patient = i;
            }
        }
    }
    healHog(patient, spec.damage, events);
}

void World::healHog(std::size_t index, float amount, std::vector<GameEvent>& events) {
    // Health back, and any poison or sleep cured (the wiki).
    Hog& hog = hogs_[index];
    if (!hog.alive) return;
    const float before = hog.health;
    hog.health = std::min(hog.maxHealth, hog.health + amount);
    hog.poisoned = false;
    hog.asleep = false;
    hog.confused = false;
    GameEvent healed;
    healed.type = GameEventType::Healed;
    healed.hog = index;
    healed.amount = hog.health - before;
    healed.position = hog.position;
    events.push_back(healed);
}

void World::afflict(std::size_t index, WeaponEffect effect) {
    Hog& hog = hogs_[index];
    if (!hog.alive) return;
    switch (effect) {
    case WeaponEffect::Poison: hog.poisoned = true; break;
    case WeaponEffect::Sleep: hog.asleep = true; break;
    case WeaponEffect::Madness: hog.madnessSecondsLeft = kMadnessSeconds; break;
    case WeaponEffect::Confusion: hog.confused = true; break;
    case WeaponEffect::Heal:
    case WeaponEffect::None: break;
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

    // Objects within reach in front are hit too (the Boot Camp's first
    // dummy is attacked with the bayonet). The box list shrinks as objects
    // go, so the hits are gathered first.
    const glm::vec3 reachPoint = attacker.position + glm::vec3(0.0f, kMuzzleHeight, 0.0f);
    std::vector<std::size_t> struck;
    for (const auto& box : map_.boxes()) {
        glm::vec3 toBox = box.centre - attacker.position;
        toBox.y = 0.0f;
        const float along = glm::length(toBox);
        const bool inFront = along < 1.0f || glm::dot(toBox / along, facing) >= kMeleeConeCosine;
        if (inFront && box.distanceTo(reachPoint) <= spec.range) struck.push_back(box.object);
    }
    for (const std::size_t object : struck) damageMapObject(object, spec.damage, events);
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
    // A sweeping weapon (the flame thrower) swings from a little to the
    // left to the right as the burst goes on.
    float sweep = 0.0f;
    if (spec.sweepDegrees > 0.0f && spec.shots > 1) {
        const float done = static_cast<float>(spec.shots - burst.shotsLeft) / static_cast<float>(spec.shots - 1);
        sweep = glm::radians(spec.sweepDegrees * (0.15f - done));
    }
    for (int pellet = 0; pellet < spec.pellets; ++pellet) {
        const float yaw = shooter.yawRadians + sweep + randomSpread();
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
            if (spec.effect == WeaponEffect::Heal) {
                healHog(*hit.hog, spec.damage, events);
            } else {
                damageHog(hogs_, *hit.hog, spec.damage, events);
                afflict(*hit.hog, spec.effect);
            }
        } else if (hit.object) {
            damageMapObject(*hit.object, spec.damage, events);
        }
    }

    --burst.shotsLeft;
    burst.secondsToNextShot = spec.secondsBetweenShots;
    if (burst.shotsLeft <= 0) {
        burst_.reset();
    }
}

void World::detonate(const Projectile& projectile, std::vector<GameEvent>& events) {
    WeaponSpec spec = weaponSpec(projectile.weapon);
    if (projectile.bomblet) {
        // Some weapons' pieces burst smaller than the weapon itself.
        if (spec.bombletDamage > 0.0f) spec.damage = spec.bombletDamage;
        if (spec.bombletRadius > 0.0f) spec.blastRadius = spec.bombletRadius;
    }
    const glm::vec3 centre = projectile.position + glm::vec3(0.0f, kExplosionLift, 0.0f);

    GameEvent explosion;
    explosion.type = GameEventType::Explosion;
    explosion.position = centre;
    explosion.weapon = spec.id;
    events.push_back(explosion);
    addExplosion(centre, spec.blastRadius);

    if (spec.effect == WeaponEffect::Heal) {
        // Medicine: health for everyone within reach, most at the centre.
        for (std::size_t i = 0; i < hogs_.size(); ++i) {
            const float distance = glm::length(hogCentre(hogs_[i]) - centre);
            if (hogs_[i].alive && distance < spec.blastRadius) {
                healHog(i, spec.damage * (1.0f - distance / spec.blastRadius), events);
            }
        }
        return;
    }
    explode(hogs_, centre, spec.blastRadius, spec.damage, spec.knockback, events);
    damageObjects(centre, spec.blastRadius, spec.damage, events);
    if (spec.effect != WeaponEffect::None) {
        // A gas reaches every hog in its cloud.
        for (std::size_t i = 0; i < hogs_.size(); ++i) {
            if (hogs_[i].alive && glm::length(hogCentre(hogs_[i]) - centre) < spec.blastRadius) afflict(i, spec.effect);
        }
    }

    // Explosives lying within the blast go off too, a moment later.
    for (auto& other : projectiles_) {
        const auto& otherSpec = weaponSpec(other.weapon);
        if (otherSpec.kind == WeaponKind::Placed && glm::length(other.position - centre) < spec.blastRadius) {
            other.fuseSecondsLeft = kChainReactionSeconds;
        }
    }

    if (spec.hasPayload) {
        // The mine shell leaves a mine where it burst (after the chain
        // reaction above, which would set the new mine off at once).
        glm::vec3 spot = projectile.position;
        spot.y = map_.groundHeight(spot.x, spot.z, spot.y + kExplosionLift);
        if (!map_.terrain().tileAt(spot.x, spot.z).isWater) {
            projectiles_.push_back(launchProjectile(spec.payload, projectile.owner, spot, glm::vec3(0.0f), 0.0f));
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
    if (isDeathmatch()) {
        const int limit = turns_->rules().deathmatchLimit;
        const auto& teams = turns_->teams();
        return std::any_of(teams.begin(), teams.end(), [&](int team) { return deathmatchPoints(team) >= limit; });
    }
    const auto living = turns_->livingTeams(hogs_);
    // A map with a single team (training) only ends if that team is wiped
    // out, or when the Boot Camp is done.
    return living.empty() || (hadSeveralTeams_ && living.size() <= 1) || trainingComplete();
}

void World::finishMatch(std::vector<GameEvent>& events) {
    phase_ = MatchPhase::Over;
    if (isDeathmatch()) {
        // The team with the most points; two at the top is a draw.
        winner_.reset();
        int best = std::numeric_limits<int>::min();
        for (const int team : turns_->teams()) {
            const int points = deathmatchPoints(team);
            if (points > best) {
                best = points;
                winner_ = team;
            } else if (points == best) {
                winner_.reset();
            }
        }
    } else {
        const auto living = turns_->livingTeams(hogs_);
        winner_ = living.size() == 1 ? std::optional<int>(living.front()) : std::nullopt;
    }
    GameEvent over;
    over.type = GameEventType::MatchOver;
    over.team = winner_.value_or(-1);
    events.push_back(over);
}

void World::startNextTurn(std::vector<GameEvent>& events) {
    for (auto& hog : hogs_) {
        hog.held.reset();
        hog.drawnWeapon.reset();
        hog.charging = false;
    }
    if (isDeathmatch()) respawnDeadHogs(events);  // the turn's dead count, then come back
    if (matchIsDecided()) {
        finishMatch(events);
        return;
    }
    turns_->endTurn(hogs_);
    phase_ = MatchPhase::Playing;
    fireWasHeld_ = true;  // a key still held from the last turn must not fire at once
    turnBegan(events);
}

void World::turnBegan(std::vector<GameEvent>& events) {
    // The turn before is over: its kills count as one go (the executable keeps
    // the best at hog + 0x1C6 when a turn ends).
    for (auto& hog : hogs_) {
        hog.record.mostKillsInATurn = std::max(hog.record.mostKillsInATurn, hog.record.killsThisTurn);
        hog.record.killsThisTurn = 0;
    }

    GameEvent started;
    started.type = GameEventType::TurnStarted;
    started.team = turns_->activeTeam();
    started.hog = turns_->activeHog();
    events.push_back(started);

    // The last turn's mines are armed now; this turn starts afresh.
    armMines();
    minesThisTurn_ = 0;
    chargeRetreat_ = false;

    // A flight still going on when the turn changed ends as if the player
    // had pressed fire: the jetpack falls away and the parachute opens.
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        hogs_[i].controlsReversed = false;
        hogs_[i].looking = Hog::Looking::Normally;
        if (hogs_[i].jetpackSecondsLeft > 0.0f) dropJetpack(i, false, events);
    }

    // The hog whose turn it is comes out of hiding, suffers its poison and,
    // if asleep, misses the turn; if confused, plays it with its controls
    // reversed (the wiki's gases, darts and hide).
    if (const auto active = activeHog()) {
        Hog& hog = hogs_[*active];
        if (hog.confused) {
            hog.confused = false;
            hog.controlsReversed = true;
        }
        if (hog.sheltered()) {
            // Tents heal the hog inside (the wiki: a medic tent 50 points,
            // a mash tent all of them).
            const MountKind kind = mounts_[*hog.mount].kind;
            if (kind == MountKind::MedicTent) healHog(*active, 50.0f, events);
            if (kind == MountKind::MashTent) healHog(*active, hog.maxHealth, events);
        }
        if (hog.hidden()) {
            hog.hideShield = 0.0f;
            GameEvent unhid;
            unhid.type = GameEventType::HogUnhidden;
            unhid.hog = *active;
            events.push_back(unhid);
        }
        if (hog.poisoned) damageHog(hogs_, *active, kPoisonDamage, events);
        if (hog.asleep && hog.alive) {
            hog.asleep = false;
            GameEvent slept;
            slept.type = GameEventType::TurnSlept;
            slept.hog = *active;
            events.push_back(slept);
            phase_ = MatchPhase::Resolving;  // its turn passes at once
            settleSecondsLeft_ = 0.0f;
        }
    }

    // Sudden death: when the chosen round begins, every hog is down to one
    // point of health, so the next hit decides.
    const int round = turns_->round();
    if (turns_->rules().suddenDeathRound > 0 && round == turns_->rules().suddenDeathRound && !suddenDeathDone_) {
        suddenDeathDone_ = true;
        for (auto& hog : hogs_) {
            if (hog.alive) hog.health = std::min(hog.health, kSuddenDeathHealth);
        }
        GameEvent suddenDeath;
        suddenDeath.type = GameEventType::SuddenDeath;
        events.push_back(suddenDeath);
    }
}

std::vector<GameEvent> World::update(double seconds, const PlayerInput& input,
                                     const std::vector<formats::Animation>& animations) {
    timeSeconds_ += seconds;
    // Events of a turn ended from outside (endTurn) come first.
    std::vector<GameEvent> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    const auto active = activeHog();
    const bool playing = phase_ == MatchPhase::Playing && active.has_value();

    sighting_ = false;
    if (playing) {
        handleAiming(*active, input, seconds, events);
        Hog& hog = hogs_[*active];
        if (phase_ == MatchPhase::Playing && hog.alive) updateWeaponStance(hog);
        // The rifle sight, held as the original's "rifle cam": guns only,
        // and only standing (not flying, falling or swimming).
        const bool steady = hog.alive && !isAirborne(hog) && hog.motion.state != MotionState::Swimming &&
                            hog.jetpackSecondsLeft <= 0.0f;
        sighting_ = input.sight && steady && phase_ == MatchPhase::Playing &&
                    weaponSpec(hog.selectedWeapon()).rifleView;
        if (sighting_) {
            // The mouse looks around through the sight (decision 131).
            hog.yawRadians += glm::radians(input.lookYawDegrees);
            hog.aimDegrees =
                std::clamp(hog.aimDegrees + input.lookPitchDegrees, kLowestAimDegrees, kHighestAimDegrees);
        }
        if (hog.sheltered()) {
            const MountSpec& mount = mountSpec(mounts_[*hog.mount].kind);
            hog.aimDegrees = std::clamp(hog.aimDegrees, mount.lowestAimDegrees, mount.highestAimDegrees);
        }
        if (input.weaponStep != 0 || input.chooseWeapon) hog.looking = Hog::Looking::Normally;
        applyFooter(*active);  // getting in appears and goes as it walks
    }

    float distanceMoved = 0.0f;
    for (std::size_t i = 0; i < hogs_.size(); ++i) {
        MovementInput movement;
        bool canMove = false;
        if (active && i == *active && phase_ == MatchPhase::Playing) {
            movement = input.movement;
            movement.keepInPlayArea = true;
            if (hogs_[i].controlsReversed) {
                std::swap(movement.forward, movement.backward);
                std::swap(movement.turnLeft, movement.turnRight);
            }
            if (hogs_[i].jetpackSecondsLeft > 0.0f && hogs_[i].alive) {
                flyJetpack(i, movement, seconds, animations, events);
                continue;
            }
            if (hogs_[i].sheltered()) {
                // Inside: it turns to aim; a vehicle drives. Its animations
                // still play (an action's motion holds the turn up).
                Hog& inside = hogs_[i];
                inside.animation.update(seconds, animations);
                if (mountSpec(mounts_[*inside.mount].kind).drivable) {
                    driveMount(i, movement, seconds);
                } else {
                    const int turn = (movement.turnLeft ? 1 : 0) - (movement.turnRight ? 1 : 0);
                    inside.yawRadians += static_cast<float>(turn) * glm::radians(kDriveTurnDegreesPerSecond) *
                                         static_cast<float>(seconds);
                }
                continue;
            }
            if (hogs_[i].looking != Hog::Looking::Normally) {
                movement.forward = movement.backward = movement.jump = false;  // looking, not walking
            }
            canMove = true;  // no movement allowance: only the clock limits the turn
            if (hogs_[i].charging || sighting_) {
                // Charging a throw or looking through the sight: the hog may
                // turn to aim, not walk away.
                movement.forward = movement.backward = movement.jump = false;
            }
        } else if (hogs_[i].sheltered()) {
            hogs_[i].animation.update(seconds, animations);
            continue;  // waits inside its building or vehicle
        } else if (hogs_[i].alive && hogs_[i].madnessSecondsLeft > 0.0f) {
            // Madness gas: it runs straight ahead until it wears off.
            hogs_[i].madnessSecondsLeft = std::max(0.0f, hogs_[i].madnessSecondsLeft - static_cast<float>(seconds));
            movement.forward = true;
            canMove = true;
        } else if (hogs_[i].alive && hogs_[i].motion.state == MotionState::Swimming) {
            // A hog nobody controls swims to the nearest shore by itself, as
            // in the original (FUN_00472dd0 looks for the nearest dry tile).
            if (const auto shore = nearestShore(hogs_[i].position)) {
                const glm::vec3 way = *shore - hogs_[i].position;
                hogs_[i].yawRadians = yawFacing(way);
                movement.forward = true;
                canMove = true;
            }
        }
        const auto moved = moveHog(hogs_, i, movement, canMove, seconds, map_, animations);
        addMovementEvents(moved, hogs_[i], i, events);
        if (moved.landed) {
            damageHog(hogs_, i, fallDamage(moved.fallHeight), events);
            hogs_[i].parachute = false;
        }
        if (active && i == *active) {
            distanceMoved = moved.distanceMoved;
        }
    }

    hurtSwimmers(seconds, events);
    explodeDeadHogs(events);
    burnDrums(seconds, events);
    lowerFallingCrates(seconds);
    collectPickups(events);

    if (burst_) {
        burst_->secondsToNextShot -= static_cast<float>(seconds);
        if (burst_->secondsToNextShot <= 0.0f) {
            fireShot(events);
        }
    }
    steerGuided(input, seconds);
    stepProjectiles(seconds, events);
    updateEffects(seconds);
    updateObjectives(events);
    if (training_) trainingTips(active, events);
    recordDeeds(active, events);

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
        } else if (chargeRetreat_ && !chargeBurning()) {
            // The TNT has gone off: the turn is over once things settle.
            chargeRetreat_ = false;
            phase_ = MatchPhase::Resolving;
            settleSecondsLeft_ = kSettleSeconds;
        } else if (turns_->update(seconds, distanceMoved, hogs_)) {
            // Out of time: TurnSystem already passed the turn on.
            hogs_[*active].held.reset();
            hogs_[*active].charging = false;
            fireWasHeld_ = true;
            if (turns_->turnNumber() != before) {
                turnBegan(events);
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
    if (spec.kind == WeaponKind::SpecialOps || spec.kind == WeaponKind::Airstrike) {
        // A ring on the chosen spot.
        const float power = hog.charging ? std::max(hog.power, kMinimumPower) : 1.0f;
        const glm::vec3 spot = targetSpot(hog, spec, power);
        std::vector<glm::vec3> ring;
        constexpr int kRingPoints = 16;
        constexpr float kRingRadius = 200.0f;
        for (int i = 0; i < kRingPoints; ++i) {
            const float angle = glm::two_pi<float>() * static_cast<float>(i) / kRingPoints;
            glm::vec3 point = spot + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * kRingRadius;
            point.y = map_.groundHeight(point.x, point.z, spot.y + kRingRadius) + 20.0f;
            ring.push_back(point);
        }
        return ring;
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

std::optional<SightView> World::sightView() const {
    const auto active = activeHog();
    if (!sighting_ || !active) return std::nullopt;
    const Hog& hog = hogs_[*active];
    return SightView{muzzlePosition(hog), aimDirection(hog)};
}

bool World::switchHog(int step) {
    return turns_ && phase_ == MatchPhase::Playing && turns_->switchHog(step, hogs_);
}

void World::endTurn() {
    if (turns_ && phase_ == MatchPhase::Playing) {
        // Reported by the next update(), like everything else that happens.
        startNextTurn(pendingEvents_);
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
    std::vector<Pickup> pickups;
    std::vector<Objective> objectives;
    std::vector<Destructible> destructibles;
    std::vector<std::size_t> hidden;
    std::vector<std::pair<std::size_t, MountKind>> mounts;
    std::size_t objectCount = 0;
    if (vfs.exists(base + ".pog")) {
        const auto objects = formats::parseMapObjects(vfs.readFile(base + ".pog"));
        hogs = spawnHogs(objects, terrain);
        pickups = findPickups(objects);
        objectives = findObjectives(objects, isTrainingMap(mapName));
        for (std::size_t i = 0; i < objects.size(); ++i) {
            if (startsHidden(objects[i], isTrainingMap(mapName))) hidden.push_back(i);
        }
        destructibles = findDestructibles(objects);
        mounts = findMounts(objects);
        objectCount = objects.size();

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
    World world(MapCollision(std::move(terrain), std::move(boxes)), std::move(hogs), rules, std::move(pickups));
    world.setObjectives(std::move(objectives), objectCount);
    world.setDestructibles(std::move(destructibles));
    world.setTraining(isTrainingMap(mapName));
    world.hideObjects(hidden);
    world.setMounts(mounts);
    return world;
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
