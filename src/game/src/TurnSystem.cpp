// TurnSystem.cpp - see TurnSystem.h for an overview.

#include "openhogs/game/TurnSystem.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace openhogs::game {

TurnSystem::TurnSystem(const std::vector<Hog>& hogs, TurnRules rules) : rules_(rules) {
    if (hogs.empty()) {
        throw std::invalid_argument("A match needs at least one hog");
    }
    std::set<int> teams;
    for (const auto& hog : hogs) {
        teams.insert(hog.team);
    }
    teams_.assign(teams.begin(), teams.end());
    lastHogOfTeam_.assign(teams_.size(), hogs.size());

    // The first turn goes to the team of the hog the map marks for the
    // player, so a mission starts with the player's team.
    const auto player = std::find_if(hogs.begin(), hogs.end(), [](const Hog& hog) { return hog.playerControlled; });
    if (player != hogs.end()) {
        activeTeamIndex_ = static_cast<std::size_t>(
            std::find(teams_.begin(), teams_.end(), player->team) - teams_.begin());
    }
    startTurn(hogs);
}

int TurnSystem::round() const {
    const auto teams = static_cast<int>(teams_.size());
    return teams > 0 ? (turnNumber_ - 1) / teams + 1 : 1;
}

std::vector<std::size_t> TurnSystem::hogsOfTeam(int team, const std::vector<Hog>& hogs) const {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < hogs.size(); ++i) {
        if (hogs[i].team == team && hogs[i].alive) {
            result.push_back(i);
        }
    }
    return result;
}

std::vector<int> TurnSystem::livingTeams(const std::vector<Hog>& hogs) const {
    std::vector<int> living;
    for (const int team : teams_) {
        if (!hogsOfTeam(team, hogs).empty()) {
            living.push_back(team);
        }
    }
    return living;
}

void TurnSystem::startTurn(const std::vector<Hog>& hogs) {
    ++turnNumber_;
    secondsLeft_ = rules_.turnSeconds;
    movementLeft_ = rules_.movementAllowance;
    hasMoved_ = false;

    // The hog after the one this team used last time (the first hog the
    // first time).
    const auto members = hogsOfTeam(activeTeam(), hogs);
    if (members.empty()) {
        return;  // nobody left to play (the match is over)
    }
    const std::size_t last = lastHogOfTeam_[activeTeamIndex_];
    const auto next = std::upper_bound(members.begin(), members.end(), last);
    activeHog_ = (last == hogs.size() || next == members.end()) ? members.front() : *next;
    lastHogOfTeam_[activeTeamIndex_] = activeHog_;
}

bool TurnSystem::switchHog(int step, const std::vector<Hog>& hogs) {
    if (!canSwitchHog()) {
        return false;
    }
    const auto members = hogsOfTeam(activeTeam(), hogs);
    if (members.size() < 2) {
        return false;
    }
    const auto position = static_cast<long long>(std::find(members.begin(), members.end(), activeHog_) - members.begin());
    const auto count = static_cast<long long>(members.size());
    const long long next = ((position + step) % count + count) % count;
    activeHog_ = members[static_cast<std::size_t>(next)];
    lastHogOfTeam_[activeTeamIndex_] = activeHog_;
    return true;
}

bool TurnSystem::update(double seconds, float distanceMoved, const std::vector<Hog>& hogs) {
    if (distanceMoved > 0.0f) {
        hasMoved_ = true;
        movementLeft_ = std::max(0.0f, movementLeft_ - distanceMoved);
    }
    secondsLeft_ -= seconds;
    if (secondsLeft_ > 0.0) {
        return false;
    }
    endTurn(hogs);
    return true;
}

void TurnSystem::endTurn(const std::vector<Hog>& hogs) {
    for (std::size_t tries = 0; tries < teams_.size(); ++tries) {
        activeTeamIndex_ = (activeTeamIndex_ + 1) % teams_.size();
        if (!hogsOfTeam(activeTeam(), hogs).empty()) {
            break;
        }
    }
    startTurn(hogs);
}

} // namespace openhogs::game
