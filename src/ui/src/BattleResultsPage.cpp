// BattleResultsPage.cpp - see BattleResultsPage.h for an overview.

#include "openhogs/ui/BattleResultsPage.h"

namespace openhogs::ui {

namespace {

// One table row: the label, then one value per team.
std::string row(const std::string& label, const std::vector<std::string>& cells) {
    std::string line = label;
    for (const auto& cell : cells) line += "\t" + cell;
    return line;
}

} // namespace

std::vector<std::string> battleResultLines(const game::BattleResults& results, const UiStrings& strings,
                                           const std::function<int(int team)>& nationOf,
                                           const std::function<std::string(std::size_t hog)>& hogName) {
    std::vector<std::string> boxes, grilled, fried, victory, awards, totals;
    for (const auto& team : results.teams) {
        boxes.push_back(std::string(1, kColourBoxCell) + std::to_string(nationOf(team.team)));
        grilled.push_back(std::to_string(team.enemiesKilled));
        fried.push_back(std::to_string(team.friendsKilled));
        victory.push_back(std::to_string(team.victoryBonus));
        awards.push_back(std::to_string(team.awardPoints));
        totals.push_back(std::to_string(team.total()));
    }
    std::vector<std::string> lines{row("", boxes),
                                   row(strings.enemyGrilled, grilled),
                                   row(strings.friendsFried, fried),
                                   row(strings.victoryBonus, victory),
                                   row(strings.specialAwards, awards),
                                   row(strings.total, totals)};
    if (results.bestTeam) {
        lines.push_back(strings.teamName(nationOf(*results.bestTeam)) + " " + strings.wins);
    }
    if (results.award) {
        const auto kind = static_cast<std::size_t>(results.award->kind);
        const std::string name = kind < strings.awardNames.size() ? strings.awardNames[kind] : "";
        lines.push_back(strings.specialAward + ": " + hogName(results.award->hog) + " - " + name);
    }
    return lines;
}

} // namespace openhogs::ui
