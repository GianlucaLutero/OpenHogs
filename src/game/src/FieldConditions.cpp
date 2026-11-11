// FieldConditions.cpp - see FieldConditions.h for an overview.

#include "openhogs/game/FieldConditions.h"

#include <algorithm>
#include <map>

namespace openhogs::game {

void applyFieldConditions(std::vector<Hog>& hogs, const FieldConditions& conditions) {
    std::vector<Hog> kept;
    std::map<int, int> hogsOfTeam;
    for (auto& hog : hogs) {
        const bool removed = std::find(conditions.removedTeams.begin(), conditions.removedTeams.end(), hog.team) !=
                             conditions.removedTeams.end();
        if (removed) continue;
        int& count = hogsOfTeam[hog.team];
        if (conditions.hogsPerTeam > 0 && count >= conditions.hogsPerTeam) continue;
        ++count;
        hog.maxHealth *= conditions.healthFactor;
        hog.health = hog.maxHealth;
        kept.push_back(std::move(hog));
    }
    hogs = std::move(kept);
}

} // namespace openhogs::game
