// TurnSystem.h
//
// Who plays now: teams take turns, and in each turn one hog of the active
// team moves.
//
// - Teams play in order of their number, then the first team again.
// - Each team uses its hogs in rotation: the hog after the one it used
//   last time. Before the hog has moved, the player may pick another hog
//   of the same team.
// - A turn has a time limit; until it runs out the hog may walk as far as
//   it likes (the original has no movement allowance). It ends when the
//   time runs out, when the player ends it, or once the shot of the turn
//   is over (the world decides that, see World).
// - Dead hogs are skipped, and so are teams with no hog left.
// - With SELECT PIG (a FIELD CONDITIONS setting) each turn begins with up
//   to 10 seconds to choose the hog, as the original's manual says; the
//   turn clock waits meanwhile. Choosing ends when the hog moves.
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
    // Sudden death: when this round begins every hog is left with one
    // point of health (0 = never). A round is one turn of every team.
    int suddenDeathRound = 0;
    // DEATHMATCH LIMIT: killed hogs come back and the first team to this
    // many points wins (2 for a kill, -1 for a loss, see World). 0 = the
    // usual match: the last team standing wins.
    int deathmatchLimit = 0;
    // SELECT PIG: the turn starts with time to choose the hog.
    bool selectPig = false;
};

// How long SELECT PIG gives to choose (the manual: "only 10 seconds").
inline constexpr double kSelectPigSeconds = 10.0;

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
    // 1 while every team plays its first turn, 2 for their second turns...
    int round() const;

    double secondsLeft() const { return secondsLeft_; }

    // The player may pick another hog until the active one has moved
    // (with SELECT PIG: also only while the choosing time lasts).
    bool canSwitchHog() const { return !hasMoved_ && (!rules_.selectPig || choosingSecondsLeft_ > 0.0); }
    // SELECT PIG: still choosing the hog (the turn clock waits).
    bool choosingHog() const { return rules_.selectPig && canSwitchHog(); }

    // Picks the next (+1) or previous (-1) hog of the active team, if
    // canSwitchHog(). Returns true if the active hog changed.
    bool switchHog(int step, const std::vector<Hog>& hogs);

    // Counts down the time and notes whether the hog has moved. Returns true if the
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
    bool hasMoved_ = false;
    double choosingSecondsLeft_ = 0.0;  // SELECT PIG
};

} // namespace openhogs::game
