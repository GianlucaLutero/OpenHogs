// BattleAwards.h
//
// The battle results after a skirmish or a match between people: each
// team's score and the one special award of the match, as the executable
// works them out (FUN_00483cc0, the BATTLE RESULTS page):
//
//   score = enemies killed (ENEMY GRILLED) - own hogs killed (FRIENDS FRIED)
//         + 3 for the team that won (VICTORY BONUS)
//         + the special award's points, for the team of the hog that got it.
//
// The award goes to the most deserving hog of the match, the first that
// fits in this order:
//
//   award                  | for                                   | points
//   -----------------------+---------------------------------------+-------
//   THE ANNIHLATOR!!!      | 4 or more enemies killed in one turn  |   5
//   TRIPLE RIP REAPER!!!   | 3 enemies killed in one turn          |   3
//   MASTER SADIST!!!       | 351 or more damage dealt to enemies   |   3
//   DOUBLE DEATH DEALER!   | 2 enemies killed in one turn          |   2
//   SADISM AWARD!          | 176 to 350 damage dealt               |   2
//   PIG OF THE BATTLE      | any damage dealt (the most of anyone) |   1
//
// The kill awards go to the hog with the most kills in one turn, the
// damage awards to the hog that dealt the most damage (the first one
// found when two are level). The team with the best score WINS!!; when
// two share it, nobody does.

#pragma once

#include "openhogs/game/Hog.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace openhogs::game {

// In the order of the text entries 203 to 208.
enum class SpecialAwardKind { Annihilator, TripleRipReaper, MasterSadist, DoubleDeathDealer, SadismAward, PigOfTheBattle };

struct SpecialAward {
    SpecialAwardKind kind = SpecialAwardKind::PigOfTheBattle;
    std::size_t hog = 0;  // index in the match's hogs
    int points = 0;
};

struct TeamResult {
    int team = 0;
    int enemiesKilled = 0;
    int friendsKilled = 0;
    int victoryBonus = 0;
    int awardPoints = 0;
    int total() const { return enemiesKilled - friendsKilled + victoryBonus + awardPoints; }
};

struct BattleResults {
    std::vector<TeamResult> teams;      // in the order of `teams` given
    std::optional<SpecialAward> award;  // empty when nobody hurt an enemy
    std::optional<int> bestTeam;        // the team that WINS!!, unless two share the best score
};

// Points the winning team of the match gets.
inline constexpr int kVictoryBonus = 3;

// The results of a match fought by `teams` (team numbers, in playing
// order) that `winner` won (empty for a draw).
BattleResults battleResults(const std::vector<Hog>& hogs, const std::vector<int>& teams, std::optional<int> winner);

} // namespace openhogs::game
