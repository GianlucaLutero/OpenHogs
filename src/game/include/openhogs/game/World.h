// World.h
//
// The state of the match being played: the map, the hogs, the shots in the
// air and whose turn it is.
//
// A turn goes through three phases:
//   - Playing:   the active hog moves, picks a weapon and aims;
//   - Resolving: it has fired; nobody is controlled while bullets fly,
//                grenades bounce, explosions throw hogs about. When all is
//                still for a moment, the next team plays;
//   - Over:      only one team is left (or none): the match is over.
//
// The world only changes through update(), which the game calls at a
// fixed rate (see FixedTimestep). It knows nothing about drawing or sound:
// the application reads the world, shows it, and turns the events of each
// step into sounds.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Destructibles.h"
#include "openhogs/game/GameEvents.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/Mounts.h"
#include "openhogs/game/Objectives.h"
#include "openhogs/game/Pickups.h"
#include "openhogs/game/Projectiles.h"
#include "openhogs/game/Training.h"
#include "openhogs/game/TurnSystem.h"

#include <functional>
#include <map>
#include <set>
#include <optional>
#include <random>
#include <string_view>
#include <vector>

namespace openhogs::game {

// Aiming speed and how long it takes to charge a throw to full power.
inline constexpr float kAimDegreesPerSecond = 45.0f;
inline constexpr float kSecondsToFullPower = 1.5f;

// What the player does during one step.
struct PlayerInput {
    MovementInput movement;
    int weaponStep = 0;   // +1 / -1: next / previous weapon (a key press)
    bool aimUp = false;   // held
    bool aimDown = false; // held
    bool fire = false;    // held: powered weapons charge while held and fire when released
    bool sight = false;   // held: look through the rifle sight (guns only, standing still)
    // Looking around through the sight with the mouse, this step: degrees
    // to turn the hog (left positive) and to raise its aim. Ignored when
    // the hog is not looking through the sight.
    float lookYawDegrees = 0.0f;
    float lookPitchDegrees = 0.0f;
};

// What the player sees through the rifle sight: from the gun's muzzle,
// along the line of fire.
struct SightView {
    glm::vec3 eye{0.0f};
    glm::vec3 direction{1.0f, 0.0f, 0.0f};  // unit vector
};

enum class MatchPhase { Playing, Resolving, Over };

// Short-lived things to draw.
struct ExplosionEffect {
    glm::vec3 position{0.0f};
    float radius = 0.0f;
    float ageSeconds = 0.0f;
};
struct TracerEffect {
    glm::vec3 from{0.0f};
    glm::vec3 to{0.0f};
    float ageSeconds = 0.0f;
};
inline constexpr float kExplosionEffectSeconds = 0.6f;
inline constexpr float kTracerEffectSeconds = 0.15f;

class World {
public:
    World(MapCollision map, std::vector<Hog> hogs, TurnRules rules = {}, std::vector<Pickup> pickups = {});

    const MapCollision& map() const { return map_; }
    const TerrainHeightField& terrain() const { return map_.terrain(); }
    const std::vector<Hog>& hogs() const { return hogs_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    const std::vector<ExplosionEffect>& explosions() const { return explosions_; }
    const std::vector<TracerEffect>& tracers() const { return tracers_; }
    // The map's crates; taken ones stay in the list, marked taken.
    const std::vector<Pickup>& pickups() const { return pickups_; }
    // The map's objectives (Objectives.h); done ones stay, marked done.
    const std::vector<Objective>& objectives() const { return objectives_; }
    // Sets the objectives; `objectCount` is how many objects the map has,
    // so reward crates get object numbers after them.
    void setObjectives(std::vector<Objective> objectives, std::size_t objectCount);
    // The objects weapons can destroy; destroyed ones stay, marked.
    const std::vector<Destructible>& destructibles() const { return destructibles_; }
    void setDestructibles(std::vector<Destructible> destructibles) { destructibles_ = std::move(destructibles); }
    // Training mode (the Boot Camp, Training.h): set before the first update.
    void setTraining(bool training);
    bool training() const { return training_; }
    // Hides map objects until an objective reveals them (Objectives.h).
    void hideObjects(const std::vector<std::size_t>& objects);
    bool isHidden(std::size_t object) const { return hiddenBoxes_.contains(object); }
    // How many of a weapon a team has from crates (beyond its hogs' own).
    int stock(int team, WeaponId weapon) const;

    // Empty when the map has no hogs (a map without hogs is only scenery).
    const std::optional<TurnSystem>& turns() const { return turns_; }

    // The hog whose turn it is; empty when the map has no hogs.
    std::optional<std::size_t> activeHog() const;

    MatchPhase phase() const { return phase_; }

    // When the match is over: the winning team, or empty for a draw.
    std::optional<int> winner() const { return winner_; }

    // Seconds of simulated time since the match started.
    double timeSeconds() const { return timeSeconds_; }

    // Advances the match by one simulation step. `input` controls the
    // active hog while the turn is in its Playing phase.
    std::vector<GameEvent> update(double seconds, const PlayerInput& input,
                                  const std::vector<formats::Animation>& animations);

    // Where the active hog's shot would go: the arc of a throw (at the
    // power being charged, or full power when not charging) or the line
    // of a bullet. Empty for melee weapons and explosives put down.
    std::vector<glm::vec3> aimPreview() const;

    // While the active hog's player looks through the rifle sight (held
    // with PlayerInput::sight, with a gun, standing on dry ground): where
    // the view is. The hog can then turn and aim but not walk.
    std::optional<SightView> sightView() const;

    // The buildings and vehicles hogs can get into (Mounts.h), and where
    // each is now. Set by loadWorld from the map's objects; a hog standing
    // inside one when the match starts is put in it.
    void setMounts(const std::vector<std::pair<std::size_t, MountKind>>& mounts);
    const std::vector<Mount>& mounts() const { return mounts_; }

    // While the active hog's player looks through binoculars: the view.
    std::optional<SightView> binocularView() const;
    // While the active hog's player looks at the whole map (MAP VIEW).
    bool mapView() const;

    // Picks another hog of the active team (only before the active hog has
    // moved). Returns true if the active hog changed.
    bool switchHog(int step);

    // Ends the active team's turn now (only while Playing).
    void endTurn();

    // Which nation (index into scene::characterTeams) a team fights for:
    // its uniform and its name. By default team n is nation n (wrapping
    // round); the campaign sets the player's chosen nation and the enemy
    // of each mission.
    int nationOf(int team) const;
    void setTeamNation(int team, int nation);

private:
    void handleAiming(std::size_t hogIndex, const PlayerInput& input, double seconds, std::vector<GameEvent>& events);
    void fire(std::size_t hogIndex, float power, std::vector<GameEvent>& events);
    void updateWeaponStance(Hog& hog) const;
    void hurtSwimmers(double seconds, std::vector<GameEvent>& events);
    void collectPickups(std::vector<GameEvent>& events);
    void updateObjectives(std::vector<GameEvent>& events);
    void recordDeeds(std::optional<std::size_t> active, const std::vector<GameEvent>& events);
    void objectiveDone(const ObjectiveResult& result, std::vector<GameEvent>& events);
    void reveal(std::size_t object, std::vector<GameEvent>& events);
    void trainingTip(const TrainingTip& tip, std::vector<GameEvent>& events);
    void trainingTips(std::optional<std::size_t> active, std::vector<GameEvent>& events);
    bool trainingComplete() const;
    void damageObjects(const glm::vec3& centre, float radius, float damage, std::vector<GameEvent>& events);
    void damageMapObject(std::size_t object, float amount, std::vector<GameEvent>& events);
    void refreshStockWeapons(int team);
    void useStock(std::size_t hogIndex);
    std::optional<glm::vec3> nearestShore(const glm::vec3& from) const;
    void heal(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events);
    void healHog(std::size_t index, float amount, std::vector<GameEvent>& events);
    void afflict(std::size_t index, WeaponEffect effect);
    void shockwave(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events);
    void pickPocket(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events);
    void useOwnWeapon(Hog& hog, std::size_t slot);
    glm::vec3 targetSpot(const Hog& hog, const WeaponSpec& spec, float power) const;
    void flyJetpack(std::size_t index, const MovementInput& input, double seconds,
                    const std::vector<formats::Animation>& animations);
    void steerGuided(const PlayerInput& input, double seconds);
    std::optional<std::size_t> freeMountNear(const Hog& hog) const;
    const CollisionBox* mountBox(const Mount& mount) const;
    void enterMount(std::size_t hogIndex, std::size_t mountIndex, std::vector<GameEvent>& events);
    void leaveMount(std::size_t hogIndex, std::vector<GameEvent>& events);
    void driveMount(std::size_t hogIndex, const MovementInput& input, double seconds);
    void applyFooter(std::size_t hogIndex);
    void fireMelee(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events);
    void fireShot(std::vector<GameEvent>& events);
    void stepProjectiles(double seconds, std::vector<GameEvent>& events);
    void detonate(const Projectile& projectile, std::vector<GameEvent>& events);
    void updateEffects(double seconds);
    bool shotSettled() const;
    void startNextTurn(std::vector<GameEvent>& events);
    void turnBegan(std::vector<GameEvent>& events);
    bool matchIsDecided() const;
    void finishMatch(std::vector<GameEvent>& events);

    MapCollision map_;
    std::vector<Hog> hogs_;
    std::map<int, int> nations_;  // team -> nation, where not the default
    std::vector<Pickup> pickups_;
    std::vector<Objective> objectives_;
    std::vector<Destructible> destructibles_;
    bool training_ = false;
    std::map<std::size_t, std::vector<CollisionBox>> hiddenBoxes_;  // hidden object -> its boxes
    std::vector<std::size_t> trainingCrates_;  // pickups of the Boot Camp's chain
    bool trainingStarted_ = false;
    std::set<WeaponId> weaponsTipped_;
    std::size_t nextDropObject_ = 0;  // the object number of the next reward crate
    std::map<int, std::map<WeaponId, int>> stock_;  // team -> weapon -> how many, from crates
    std::optional<TurnSystem> turns_;
    MatchPhase phase_ = MatchPhase::Playing;
    std::optional<int> winner_;
    bool hadSeveralTeams_ = false;
    double timeSeconds_ = 0.0;

    std::vector<Projectile> projectiles_;
    std::vector<ExplosionEffect> explosions_;
    std::vector<TracerEffect> tracers_;

    // A burst of bullets being fired (machine guns fire several shots).
    struct Burst {
        std::size_t shooter = 0;
        WeaponId weapon = WeaponId::Pistol;
        int shotsLeft = 0;
        float secondsToNextShot = 0.0f;
    };
    std::optional<Burst> burst_;

    float settleSecondsLeft_ = 0.0f;
    bool suddenDeathDone_ = false;
    std::vector<GameEvent> pendingEvents_;  // from endTurn(), reported by the next update()
    bool fireWasHeld_ = false;
    bool flightEndsTurn_ = false;
    bool sighting_ = false;  // the active hog's player looks through the rifle sight
    std::vector<Mount> mounts_;  // a jetpack flight is on: the turn ends when the hog is down
    // Bullet spread: seeded with a fixed value, so a match replays the same
    // way (and tests are reproducible).
    std::mt19937 random_{20261111};
};

// Loads the terrain (Maps/<NAME>.PMG), the objects' collision boxes,
// crates and objectives, and the hogs (Maps/<NAME>.POG, with the models in Maps/<NAME>.MAD) of a map.
// Throws std::runtime_error if the terrain is missing; a map without a .POG
// file simply has no hogs and no obstacles. `prepareHogs`, if given, can
// change the map's hogs before the match is set up (the campaign puts the
// player's squad in place of the map's hogs).
World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName, TurnRules rules = {},
                const std::function<void(std::vector<Hog>&)>& prepareHogs = {});

// The teams of a map, the one that plays first first (the map marks the
// player's hog). Empty for a map without hogs. Used by the menus to offer
// each team to a human or the computer.
std::vector<int> mapTeams(const core::VirtualFileSystem& vfs, std::string_view mapName);

// Where a hog's shots start, and the direction it aims in.
glm::vec3 muzzlePosition(const Hog& hog);
glm::vec3 aimDirection(const Hog& hog);

} // namespace openhogs::game
