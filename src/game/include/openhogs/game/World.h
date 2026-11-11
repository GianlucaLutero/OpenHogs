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
#include "openhogs/game/GameEvents.h"
#include "openhogs/game/Hog.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/Projectiles.h"
#include "openhogs/game/TurnSystem.h"

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
    World(MapCollision map, std::vector<Hog> hogs, TurnRules rules = {});

    const MapCollision& map() const { return map_; }
    const TerrainHeightField& terrain() const { return map_.terrain(); }
    const std::vector<Hog>& hogs() const { return hogs_; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    const std::vector<ExplosionEffect>& explosions() const { return explosions_; }
    const std::vector<TracerEffect>& tracers() const { return tracers_; }

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

    // Picks another hog of the active team (only before the active hog has
    // moved). Returns true if the active hog changed.
    bool switchHog(int step);

    // Ends the active team's turn now (only while Playing).
    void endTurn();

private:
    void handleAiming(std::size_t hogIndex, const PlayerInput& input, double seconds, std::vector<GameEvent>& events);
    void fire(std::size_t hogIndex, float power, std::vector<GameEvent>& events);
    void fireMelee(std::size_t hogIndex, const WeaponSpec& spec, std::vector<GameEvent>& events);
    void fireShot(std::vector<GameEvent>& events);
    void stepProjectiles(double seconds, std::vector<GameEvent>& events);
    void detonate(const Projectile& projectile, std::vector<GameEvent>& events);
    void updateEffects(double seconds);
    bool shotSettled() const;
    void startNextTurn(std::vector<GameEvent>& events);
    bool matchIsDecided() const;
    void finishMatch(std::vector<GameEvent>& events);

    MapCollision map_;
    std::vector<Hog> hogs_;
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
    bool fireWasHeld_ = false;
    // Bullet spread: seeded with a fixed value, so a match replays the same
    // way (and tests are reproducible).
    std::mt19937 random_{20261111};
};

// Loads the terrain (Maps/<NAME>.PMG), the objects' collision boxes and
// the hogs (Maps/<NAME>.POG, with the models in Maps/<NAME>.MAD) of a map.
// Throws std::runtime_error if the terrain is missing; a map without a .POG
// file simply has no hogs and no obstacles.
World loadWorld(const core::VirtualFileSystem& vfs, std::string_view mapName, TurnRules rules = {});

// The teams of a map, the one that plays first first (the map marks the
// player's hog). Empty for a map without hogs. Used by the menus to offer
// each team to a human or the computer.
std::vector<int> mapTeams(const core::VirtualFileSystem& vfs, std::string_view mapName);

// Where a hog's shots start, and the direction it aims in.
glm::vec3 muzzlePosition(const Hog& hog);
glm::vec3 aimDirection(const Hog& hog);

} // namespace openhogs::game
