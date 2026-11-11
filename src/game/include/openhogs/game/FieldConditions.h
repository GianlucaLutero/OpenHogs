// FieldConditions.h
//
// The match settings of the multiplayer FIELD CONDITIONS page that change
// the map's hogs before a match: teams switched off, the number of hogs
// per team and their starting health. (Turn time and sudden death are
// TurnRules.)
//
// The original also has a deathmatch limit and "select pig"; they are not
// implemented yet (docs/open-decisions.md).

#pragma once

#include "openhogs/game/Hog.h"

#include <vector>

namespace openhogs::game {

struct FieldConditions {
    std::vector<int> removedTeams;  // teams set to OFF: their hogs are taken away
    int hogsPerTeam = 0;            // 0 = as many as the map has; otherwise at most this many
    float healthFactor = 1.0f;      // starting health: 0.5 half, 1 normal, 2 double
};

// Applies the conditions to the map's hogs (keeping their order). Teams
// keep their first hogs when they have more than hogsPerTeam.
void applyFieldConditions(std::vector<Hog>& hogs, const FieldConditions& conditions);

} // namespace openhogs::game
