// ComputerPlayer.h
//
// The computer opponent: plays the turns of the teams the computer
// controls, through the same PlayerInput a human produces with the
// keyboard. It cannot do anything a player could not: it turns, runs,
// changes weapon, aims and charges a throw at the normal speeds.
//
// A turn goes like this:
//
//   1. Think for a moment (so the players can see whose turn it is).
//   2. Plan a shot from where the hog stands (ShotPlanner, one weapon and
//      enemy per step while thinking): for every weapon and every enemy,
//      work out the direction, angle and power that would hit, and how much
//      damage it would do to the enemies minus the damage to its own team.
//   3. No worthwhile shot? Run towards the nearest enemy (jumping once if
//      blocked), then plan again from there.
//   4. Turn to the planned direction, select the weapon, aim, and fire
//      (holding the fire key until the planned power for throws).
//   5. Nothing at all to do: end the turn.
//
// The plan is made with the exact rules of the match (projectile flight,
// bullets stopped by the scenery...), then spoiled by a small random error
// (AiSettings), so the computer is good but not perfect. How the original
// game's computer plays is still to be compared (docs/open-decisions.md).

#pragma once

#include "openhogs/game/Weapons.h"
#include "openhogs/game/World.h"

#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace openhogs::game {

struct AiSettings {
    double thinkSeconds = 1.0;        // pause before acting
    float aimErrorDegrees = 2.0f;     // up to this much off in direction and angle
    float powerError = 0.04f;         // up to this much off in throw power
    float minimumScore = 8.0f;        // a shot must be worth at least this much damage
    float approachDistance = 600.0f;  // runs until this close to the enemy
    std::uint32_t seed = 1;           // the random errors are reproducible
};

// A planned shot: which weapon, in which direction, at which angle and
// power, and how good it is (expected damage to enemies, minus own losses).
struct ShotPlan {
    WeaponId weapon = WeaponId::Trotter;
    std::size_t target = 0;
    float yawRadians = 0.0f;
    float aimDegrees = 0.0f;
    float power = 1.0f;
    float score = 0.0f;
};

// Weighs every weapon of a hog against every enemy, one pair per step()
// call, and keeps the best shot. Spreading the work lets the game plan
// while the hog "thinks", without freezing a frame.
class ShotPlanner {
public:
    ShotPlanner(const World& world, std::size_t shooter);

    // Weighs the next weapon and target. The world must not have changed
    // since the planner was made.
    void step(const World& world);

    bool done() const { return next_ >= pending_.size(); }

    // The best shot found so far; empty if none does any good.
    const std::optional<ShotPlan>& best() const { return best_; }

private:
    struct Option {
        WeaponId weapon;
        std::size_t target;
    };
    std::size_t shooter_;
    std::vector<Option> pending_;
    std::size_t next_ = 0;
    std::optional<ShotPlan> best_;
};

// The best shot `shooter` can make from where it stands, all at once;
// empty if no weapon can do any good.
std::optional<ShotPlan> planShot(const World& world, std::size_t shooter);

// The yaw (Hog::yawRadians) that makes a hog face along `direction`.
float yawTowards(const glm::vec3& direction);

// What the computer does during one step.
struct ComputerDecision {
    PlayerInput input;
    bool endTurn = false;  // call World::endTurn()
};

class ComputerPlayer {
public:
    explicit ComputerPlayer(AiSettings settings = {});

    // Called every step while a computer team plays; reads the world and
    // decides the active hog's input.
    ComputerDecision update(const World& world, double seconds);

private:
    enum class Stage { Thinking, Approaching, Turning, Selecting, Aiming, Firing, Done };

    void startTurn(const World& world);
    void decide(const World& world, ComputerDecision& decision);
    void approach(const World& world, const Hog& hog, double seconds, ComputerDecision& decision);
    void spoil(ShotPlan& plan);
    void enter(Stage stage, double seconds = 0.0);

    AiSettings settings_;
    std::mt19937 random_;

    // The turn being played (turn number and hog), to notice a new one.
    int turnNumber_ = -1;
    std::size_t hog_ = 0;

    Stage stage_ = Stage::Done;
    double stageSeconds_ = 0.0;  // Thinking: time left; others: time spent
    double turnSeconds_ = 0.0;   // time spent on this turn
    std::optional<ShotPlanner> planner_;  // while thinking
    std::optional<ShotPlan> plan_;
    bool hasApproached_ = false;
    std::optional<std::size_t> approachTarget_;
    glm::vec3 progressCheckPosition_{0.0f};
    double progressCheckSeconds_ = 0.0;
    bool hasJumped_ = false;
};

} // namespace openhogs::game
