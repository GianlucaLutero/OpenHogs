// Cutscenes.cpp - see Cutscenes.h for an overview.

#include "openhogs/game/Cutscenes.h"

#include "openhogs/game/Campaign.h"

namespace openhogs::game {

namespace {

// The names as the executable writes them (file names are not
// case-sensitive in the game's folders).
std::string stream(const std::string& name) {
    return "Language/Streams/" + name + ".bik";
}

constexpr int kMissionsPerIsland = 5;

} // namespace

std::vector<std::string> introCutscenes() {
    return {stream("infologo"), stream("sheff"), stream("fmv 01"), stream("fmv 02")};
}

std::vector<std::string> campaignCutscenes(int nextMission) {
    if (nextMission > kMissionCount) {
        return {stream("fmv 08"), stream("fmv 09")};  // the campaign is won
    }
    if (nextMission == kMissionCount) {
        return {stream("fmv 07")};  // Arstria conquered, the Isle of Swill ahead
    }
    // An island conquered: missions 6, 11, 16, 21 begin the next one.
    if (nextMission > 1 && (nextMission - 1) % kMissionsPerIsland == 0) {
        const int island = (nextMission - 1) / kMissionsPerIsland;  // 1..4
        return {stream("fmv 0" + std::to_string(island + 2))};
    }
    return {};
}

} // namespace openhogs::game
