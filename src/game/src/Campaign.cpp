// Campaign.cpp - see Campaign.h for an overview.

#include "openhogs/game/Campaign.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>

namespace openhogs::game {

namespace {

// The campaign as stored in warhogs_.exe (docs/formats/campaign.md):
//   - a table of the game's 26 numbered levels (maps), whose names are the
//     gtext entries 11 + level number;
//   - the order of the missions, as level numbers;
//   - the turn time of each mission (99: no hurry).
// Islands and the mission numbers of their conquest follow the manual and
// the retail game's achievement list.
constexpr std::size_t kFirstLevelNameEntry = 11;

constexpr std::array<Mission, kMissionCount> kMissions{{
    {1, "DEMO", kFirstLevelNameEntry + 25, 0, 99},
    {2, "ROAD", kFirstLevelNameEntry + 0, 0, 99},
    {3, "TRENCH", kFirstLevelNameEntry + 1, 0, 60},
    {4, "DEVI", kFirstLevelNameEntry + 3, 0, 60},
    {5, "RUMBLE", kFirstLevelNameEntry + 2, 0, 60},
    {6, "ZULUS", kFirstLevelNameEntry + 5, 1, 60},
    {7, "TWIN", kFirstLevelNameEntry + 4, 1, 45},
    {8, "SNIPER", kFirstLevelNameEntry + 6, 1, 45},
    {9, "MASHED", kFirstLevelNameEntry + 9, 1, 45},
    {10, "GUNS", kFirstLevelNameEntry + 7, 1, 45},
    {11, "LIBERATE", kFirstLevelNameEntry + 11, 2, 45},
    {12, "OASIS", kFirstLevelNameEntry + 8, 2, 45},
    {13, "FJORDS", kFirstLevelNameEntry + 13, 2, 45},
    {14, "EYRIE", kFirstLevelNameEntry + 14, 2, 45},
    {15, "BAY", kFirstLevelNameEntry + 16, 2, 45},
    {16, "MEDIX", kFirstLevelNameEntry + 12, 3, 45},
    {17, "BRIDGE", kFirstLevelNameEntry + 15, 3, 45},
    {18, "DESVAL", kFirstLevelNameEntry + 17, 3, 30},
    {19, "SNAKE", kFirstLevelNameEntry + 18, 3, 30},
    {20, "EMPLACE", kFirstLevelNameEntry + 19, 3, 30},
    {21, "TESTER", kFirstLevelNameEntry + 21, 4, 30},
    {22, "SUPLINE", kFirstLevelNameEntry + 20, 4, 30},
    {23, "FOOT", kFirstLevelNameEntry + 22, 4, 30},
    {24, "FINAL", kFirstLevelNameEntry + 23, 4, 15},
    {25, "ESTU", kFirstLevelNameEntry + 24, 5, 30},
}};

// Losing this many hogs in a mission starts costing hogs for good.
constexpr std::size_t kLossesForgiven = 2;
constexpr int kOpponentsPerIsland = kPlayableNations - 1;

const char* const kSection = "Campaign";

std::string memberSection(std::size_t index) {
    return "Hog" + std::to_string(index + 1);
}

} // namespace

const Mission& campaignMission(int number) {
    if (number < 1 || number > kMissionCount) {
        throw std::out_of_range("No campaign mission " + std::to_string(number));
    }
    return kMissions[static_cast<std::size_t>(number - 1)];
}

std::span<const Mission> campaignMissions() {
    return kMissions;
}

bool conquersIsland(int missionNumber) {
    if (missionNumber < 1 || missionNumber > kMissionCount) return false;
    if (missionNumber == kMissionCount) return true;
    return campaignMission(missionNumber + 1).island != campaignMission(missionNumber).island;
}

Campaign::Campaign(int nation, std::vector<std::string> names) : nation_(nation), names_(std::move(names)) {
    if (nation < 0 || nation >= kPlayableNations) {
        throw std::invalid_argument("A campaign is fought for one of the six nations");
    }
    for (std::size_t i = 0; i < kSquadSize; ++i) {
        squad_.push_back({i < names_.size() ? names_[i] : newRecruitName(), RankId::Grunt});
    }
}

std::string Campaign::newRecruitName() const {
    // The first of the nation's names that nobody in the squad has and no
    // fallen hog had, or else a number.
    const auto taken = [this](const std::string& name) {
        return std::any_of(squad_.begin(), squad_.end(), [&](const SquadMember& member) { return member.name == name; }) ||
               std::find(fallen_.begin(), fallen_.end(), name) != fallen_.end();
    };
    for (const auto& name : names_) {
        if (!taken(name)) return name;
    }
    for (std::size_t number = 1;; ++number) {
        const std::string name = "PIG " + std::to_string(number);
        if (!taken(name)) return name;
    }
}

std::size_t Campaign::deploymentSize(std::size_t slots) const {
    return std::min({slots, kDeployedHogs, squad_.size()});
}

void Campaign::swap(std::size_t first, std::size_t second) {
    std::swap(squad_.at(first), squad_.at(second));
}

bool Campaign::canPromote(std::size_t member, RankId rank) const {
    if (member >= squad_.size()) return false;
    const auto allowed = promotionsFrom(squad_[member].rank);
    return std::find(allowed.begin(), allowed.end(), rank) != allowed.end() && points_ >= promotionCost(rank);
}

bool Campaign::promote(std::size_t member, RankId rank) {
    if (!canPromote(member, rank)) return false;
    points_ -= promotionCost(rank);
    squad_[member].rank = rank;
    return true;
}

MissionOutcome Campaign::recordMission(const MissionReport& report, std::size_t deployed) {
    MissionOutcome outcome;
    const std::size_t deaths = report.deathOrder.size();
    // Losing everybody fails the mission even if the enemy also fell.
    outcome.completed = report.won && deaths < deployed;
    if (!outcome.completed) {
        return outcome;  // nothing changes: the mission is played again
    }

    // Hogs lost for good: the first to die, one per death beyond the
    // forgiven ones. They are replaced by new recruits.
    const std::size_t lost = deaths > kLossesForgiven ? deaths - kLossesForgiven : 0;
    for (std::size_t i = 0; i < lost; ++i) {
        SquadMember& member = squad_.at(report.deathOrder[i]);
        outcome.lostForGood.push_back(member.name);
        fallen_.push_back(member.name);
        member.name.clear();
        member.rank = RankId::Grunt;
    }
    for (std::size_t i = 0; i < lost; ++i) {
        squad_[report.deathOrder[i]].name = newRecruitName();
    }

    outcome.survivalBonus = deaths == 0;
    outcome.islandConquered = conquersIsland(nextMission_);
    outcome.pointsEarned = kMissionPoints + (outcome.survivalBonus ? kSurvivalPoints : 0) +
                           (outcome.islandConquered ? kIslandPoints : 0);
    points_ += outcome.pointsEarned;
    ++nextMission_;
    return outcome;
}

int Campaign::enemyNation(int missionNumber, std::size_t enemy) const {
    if (missionNumber == kMissionCount && enemy == 0) {
        return kTeamLardNation;
    }
    // The other five nations, in characterTeams order.
    std::vector<int> others;
    for (int nation = 0; nation < kPlayableNations; ++nation) {
        if (nation != nation_) others.push_back(nation);
    }
    const auto first = static_cast<std::size_t>((missionNumber - 1) % kOpponentsPerIsland);
    return others[(first + enemy) % others.size()];
}

std::string Campaign::save() const {
    std::ostringstream out;
    out << "; OpenHogs campaign\n[" << kSection << "]\n";
    out << "Nation = " << nation_ << "\n";
    out << "NextMission = " << nextMission_ << "\n";
    out << "Points = " << points_ << "\n";
    out << "Names = ";
    for (std::size_t i = 0; i < names_.size(); ++i) {
        out << (i > 0 ? ", " : "") << names_[i];
    }
    out << "\n";
    out << "Fallen = ";
    for (std::size_t i = 0; i < fallen_.size(); ++i) {
        out << (i > 0 ? ", " : "") << fallen_[i];
    }
    out << "\n";
    for (std::size_t i = 0; i < squad_.size(); ++i) {
        out << "\n[" << memberSection(i) << "]\n";
        out << "Name = " << squad_[i].name << "\n";
        out << "Rank = " << rankSpec(squad_[i].rank).key << "\n";
    }
    return out.str();
}

Campaign loadCampaign(const core::IniFile& ini) {
    if (!ini.hasSection(kSection)) {
        throw core::FormatError("Not a campaign file (no [Campaign] section)");
    }
    Campaign campaign;
    campaign.nation_ = ini.getInt(kSection, "Nation", -1);
    campaign.nextMission_ = ini.getInt(kSection, "NextMission", 1);
    campaign.points_ = ini.getInt(kSection, "Points", 0);
    campaign.names_ = ini.getList(kSection, "Names");
    campaign.fallen_ = ini.getList(kSection, "Fallen");
    if (campaign.nation_ < 0 || campaign.nation_ >= kPlayableNations || campaign.nextMission_ < 1 ||
        campaign.nextMission_ > kMissionCount + 1 || campaign.points_ < 0) {
        throw core::FormatError("Campaign file: nation, mission or points out of range");
    }
    for (std::size_t i = 0; i < kSquadSize; ++i) {
        const std::string section = memberSection(i);
        if (!ini.hasSection(section)) {
            throw core::FormatError("Campaign file: missing [" + section + "]");
        }
        const auto rank = findRank(ini.getString(section, "Rank", ""));
        if (!rank) {
            throw core::FormatError("Campaign file: unknown rank in [" + section + "]");
        }
        campaign.squad_.push_back({ini.getString(section, "Name", ""), *rank});
    }
    return campaign;
}

std::size_t playerSlots(const std::vector<Hog>& hogs) {
    return static_cast<std::size_t>(std::count_if(hogs.begin(), hogs.end(), [](const Hog& hog) { return hog.playerControlled; }));
}

int deploySquad(std::vector<Hog>& hogs, const Campaign& campaign, std::size_t deployed) {
    int team = -1;
    std::size_t placed = 0;
    std::vector<Hog> result;
    for (auto& hog : hogs) {
        if (!hog.playerControlled) {
            result.push_back(std::move(hog));
            continue;
        }
        team = hog.team;
        if (placed >= deployed) {
            continue;  // more places than hogs on this mission
        }
        hog.squadIndex = placed;
        const SquadMember& member = campaign.squad()[placed++];
        const RankSpec& rank = rankSpec(member.rank);
        if (const auto* bodyClass = scene::findCharacterClass(rank.classId)) {
            hog.characterClass = bodyClass;
        }
        hog.name = member.name;
        hog.rank = member.rank;
        hog.maxHealth = rank.maxHealth;
        hog.health = rank.maxHealth;
        hog.weapons = rank.weapons;
        hog.weaponSlot = 0;
        result.push_back(std::move(hog));
    }
    hogs = std::move(result);
    return team;
}

} // namespace openhogs::game
