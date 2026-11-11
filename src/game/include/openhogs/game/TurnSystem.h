// TurnSystem.h
//
// Who plays now: teams take turns, and in each turn one hog of the active
// team moves.
//
// - Teams play in order of their number, then the first team again.
// - Each team uses its hogs in rotation: the hog after the one it used
//   last time. Before the hog has moved, the player may pick another hog
//   of the same team.
// - A turn has a time limit and a movement allowance (the distance the
//   hog may walk, jumps included). It ends when the time runs out, when
//   the player ends it, or once the shot of the turn is over (the world
//   decides that, see World).
// - Dead hogs are skipped, and so are teams with no hog left.
//
// The values of TurnRules are OpenHogs' own until the original's are known
// (see docs/open-decisions.md).

#pragma once

#include "openhogs/game/Hog.h"

#include <cstddef>
#include <vector>

namespace openhogs::game {

struct TurnRules {
    double turnSeconds = 60.0;
    float movementAllowance = 3500.0f;  // world units: about 7 seconds of running
};

class TurnSystem {
public:
    // Throws std::invalid_argument if there are no hogs.
    TurnSystem(const std::vector<Hog>& hogs, TurnRules rules = {});

    const TurnRules& rules() const { return rules_; }

    // The teams in playing order (team numbers as in Hog::team).
    const std::vector<int>& teams() const { return teams_; }

    int activeTeam() const { return teams_[activeTeamIndex_]; }
    std::size_t activeHog() const { return activeHog_; }
    int turnNumber() const { return turnNumber_; }  // 1 for the first turn

    double secondsLeft() const { return secondsLeft_; }
    float movementLeft() const { return movementLeft_; }
    bool canMove() const { return movementLeft_ > 0.0f; }

    // The player may pick another hog until the active one has moved.
    bool canSwitchHog() const { return !hasMoved_; }

    // Picks the next (+1) or previous (-1) hog of the active team, if
    // canSwitchHog(). Returns true if the active hog changed.
    bool switchHog(int step, const std::vector<Hog>& hogs);

    // Counts down the time and the movement allowance. Returns true if the
    // turn ran out of time during this step (the turn has then already
    // passed to the next team).
    bool update(double seconds, float distanceMoved, const std::vector<Hog>& hogs);

    // Ends the turn now: the next team with a living hog plays.
    void endTurn(const std::vector<Hog>& hogs);

    // The teams that still have a living hog.
    std::vector<int> livingTeams(const std::vector<Hog>& hogs) const;

private:
    std::vector<std::size_t> hogsOfTeam(int team, const std::vector<Hog>& hogs) const;
    void startTurn(const std::vector<Hog>& hogs);

    TurnRules rules_;
    std::vector<int> teams_;
    std::vector<std::size_t> lastHogOfTeam_;  // per team index; hogs.size() = none yet
    std::size_t activeTeamIndex_ = 0;
    std::size_t activeHog_ = 0;
    int turnNumber_ = 0;
    double secondsLeft_ = 0.0;
    float movementLeft_ = 0.0f;
    bool hasMoved_ = false;
};

} // namespace openhogs::game
