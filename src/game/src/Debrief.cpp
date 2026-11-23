// Debrief.cpp - see Debrief.h for an overview.

#include "openhogs/game/Debrief.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace openhogs::game {

namespace {

// The executable's badge pictures, in the order of its table.
constexpr std::array<std::string_view, 8> kBadgeFiles{"pcHeavy.bmp",  "pcSniper.bmp", "pcEngine.bmp", "pcMedic.bmp",
                                                      "pcCmmndo.bmp", "pcLegend.bmp", "pcPip1.bmp",   "pcPip2.bmp"};

// The badges of each of the executable's 17 ranks (0x4D35C8): two
// indexes into kBadgeFiles, -1 for none. The executable numbers the ranks
// as their names in gtext (63 GRUNT ... 79 LEGEND), so rank n is entry 63 + n.
constexpr int kNone = -1;
constexpr std::array<std::array<int, 2>, 17> kRankBadges{{
    {kNone, kNone},  // grunt
    {0, kNone},      // gunner
    {0, 6},          // bombardier
    {0, 7},          // pyrotechnic
    {4, kNone},      // commando
    {2, kNone},      // sapper
    {2, 6},          // engineer
    {2, 7},          // saboteur
    {1, kNone},      // scout
    {1, 6},          // sniper
    {1, 7},          // spy
    {3, kNone},      // orderly
    {3, 6},          // medic
    {3, 7},          // surgeon
    {5, kNone},      // hero
    {5, kNone},      // ace (not used by OpenHogs' ranks)
    {5, 5},          // legend: the executable names the same badge twice
}};
constexpr std::size_t kFirstRankEntry = 63;

// The hogs lost for good are the first to die, one per death beyond the
// forgiven ones (as Campaign::recordMission).
constexpr std::size_t kLossesForgiven = 2;

constexpr int kFaceCount = 9;

} // namespace

std::vector<DebriefRow> debriefRows(const std::vector<SquadMember>& squad, const MissionReport& report,
                                    std::size_t deployed, bool completed) {
    const std::size_t deaths = report.deathOrder.size();
    const std::size_t lostForGood = completed && deaths > kLossesForgiven ? deaths - kLossesForgiven : 0;

    std::vector<DebriefRow> rows;
    for (std::size_t slot = 0; slot < std::min(squad.size(), kDebriefRows); ++slot) {
        DebriefRow row;
        row.name = squad[slot].name;
        row.rank = squad[slot].rank;
        // Each hog keeps its own face; the squad slot picks one of nine.
        row.faceNumber = static_cast<int>(slot % kFaceCount) + 1;
        const auto died = std::find(report.deathOrder.begin(), report.deathOrder.end(), slot);
        if (slot < deployed && died != report.deathOrder.end()) {
            const auto order = static_cast<std::size_t>(died - report.deathOrder.begin());
            row.face = order < lostForGood ? DebriefFace::Lost : DebriefFace::Hurt;
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::vector<std::string> rankBadges(RankId rank) {
    const std::size_t number = rankSpec(rank).textEntry - kFirstRankEntry;
    std::vector<std::string> files;
    if (number >= kRankBadges.size()) return files;
    for (const int badge : kRankBadges[number]) {
        if (badge == kNone) continue;
        const std::string file(kBadgeFiles[static_cast<std::size_t>(badge)]);
        // The legend's doubled badge is drawn once.
        if (std::find(files.begin(), files.end(), file) == files.end()) files.push_back(file);
    }
    return files;
}

} // namespace openhogs::game
