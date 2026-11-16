// BattleResultsPage.h
//
// The words of the BATTLE RESULTS page after a skirmish or a match between
// people (game/BattleAwards.h works the numbers out). The executable draws
// a table on the debriefing picture (FUN_00483cc0): a coloured box per team
// at the top of its column, the rows ENEMY GRILLED, FRIENDS FRIED, VICTORY
// BONUS, SPECIAL AWARDS and TOTAL, then the team that WINS!! and the
// SPECIAL AWARD with the hog's name.
//
// The page is written as lines for the results screen. A line with tabs is
// a table row: its first cell is the row's label, the others the teams'
// columns; a cell "#n" is the colour box of nation n.

#pragma once

#include "openhogs/game/BattleAwards.h"
#include "openhogs/ui/UiStrings.h"

#include <functional>
#include <string>
#include <vector>

namespace openhogs::ui {

// Marks a table cell drawn as the colour box of a nation.
inline constexpr char kColourBoxCell = '#';

// `nationOf` gives a team's nation; `hogName` the name shown for a hog
// (its own, or its team's when it has none).
std::vector<std::string> battleResultLines(const game::BattleResults& results, const UiStrings& strings,
                                           const std::function<int(int team)>& nationOf,
                                           const std::function<std::string(std::size_t hog)>& hogName);

} // namespace openhogs::ui
