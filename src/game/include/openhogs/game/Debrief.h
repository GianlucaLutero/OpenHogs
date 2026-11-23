// Debrief.h - what the campaign's debriefing page shows after a mission.
//
// The original's MISSION RESULTS page (FUN_004845e0 in warhogs_.exe) is a
// picture page (Language/Tims/Debrief/): on the left, the first five hogs
// of the squad, each with a face in a frame (cheerful, bandaged, or a
// gravestone for a hog lost for good), the nation's uniform under it, its
// name and its rank badges; on the right, the medals of the mission
// (level complete, survival, and the special medals it offers). This file
// holds the rules of that page; drawing it is the app's job.

#pragma once

#include "openhogs/game/Campaign.h"
#include "openhogs/game/Ranks.h"

#include <cstddef>
#include <string>
#include <vector>

namespace openhogs::game {

// The page lists at most this many hogs, the first of the squad.
inline constexpr std::size_t kDebriefRows = 5;

// The face in a hog's frame.
enum class DebriefFace {
    Fine,  // facepc<n>: survived, or did not take part
    Hurt,  // facepcw<n>: fell in the mission but will be back
    Lost,  // r_i_p: lost for good, a new recruit takes the place
};

struct DebriefRow {
    std::string name;
    RankId rank = RankId::Grunt;
    DebriefFace face = DebriefFace::Fine;
    int faceNumber = 1;  // 1 to 9, which of the nine faces
};

// The rows for a mission: `squad` as it was during the mission (before
// Campaign::recordMission replaced the hogs lost for good), the report of
// the mission and how many hogs were deployed.
std::vector<DebriefRow> debriefRows(const std::vector<SquadMember>& squad, const MissionReport& report,
                                    std::size_t deployed, bool completed);

// The badges of a rank, as file names in Language/Tims/Debrief/ (none for
// a Grunt): the career's badge, then up to one pip picture for the rank
// within the career.
std::vector<std::string> rankBadges(RankId rank);

} // namespace openhogs::game
