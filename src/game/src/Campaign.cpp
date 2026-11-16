// Campaign.cpp - see Campaign.h for an overview.

#include "openhogs/game/Campaign.h"

#include "openhogs/core/BinaryReader.h"  // FormatError
#include "openhogs/core/IniFile.h"
#include "openhogs/game/Levels.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <stdexcept>

namespace openhogs::game {

namespace {

// The campaign as stored in warhogs_.exe (docs/formats/executable.md):
//   - the order of the missions, as level numbers (table at 0xD05F0);
//   - the turn time of each mission (0xD0660; 99: no hurry).
// The levels' maps and names come from Levels.h. Islands and the mission
// numbers of their conquest follow the manual and the retail game's
// achievement list.
Mission makeMission(int number, int levelNumber, int island, double turnSeconds) {
    return {number, level(levelNumber).map, level(levelNumber).nameEntry, island, turnSeconds};
}

const std::array<Mission, kMissionCount> kMissions{{
    makeMission(1, 25, 0, 99),   // ESTU      THE WAR FOUNDATION
    makeMission(2, 0, 0, 99),    // ROAD      ROUTINE PATROL
    makeMission(3, 1, 0, 60),    // TRENCH    TRENCH WARFARE
    makeMission(4, 3, 0, 60),    // DEVI      MORNING GLORY!
    makeMission(5, 2, 0, 60),    // RUMBLE    ISLAND INVASION
    makeMission(6, 5, 1, 60),    // ZULUS     UNDER SIEGE
    makeMission(7, 4, 1, 45),    // TWIN      COMMUNICATION BREAKDOWN
    makeMission(8, 6, 1, 45),    // SNIPER    THE SPYING GAME
    makeMission(9, 9, 1, 45),    // MASHED    THE VILLAGE PEOPLE
    makeMission(10, 7, 1, 45),   // GUNS      BANGERS 'N' MASH
    makeMission(11, 11, 2, 45),  // LIBERATE  SAVING PRIVATE RIND
    makeMission(12, 8, 2, 45),   // OASIS     JUST DESERTS
    makeMission(13, 13, 2, 45),  // FJORDS    GLACIER GUNS
    makeMission(14, 14, 2, 45),  // EYRIE     BATTLE STATIONS
    makeMission(15, 16, 2, 45),  // BAY       FORTIFIED SWINE
    makeMission(16, 12, 3, 45),  // MEDIX     OVER THE TOP
    makeMission(17, 15, 3, 45),  // BRIDGE    GENEVA CONVENTION
    makeMission(18, 17, 3, 30),  // DESVAL    I SPY...
    makeMission(19, 18, 3, 30),  // SNAKE     CHEMICAL COMPOUND
    makeMission(20, 19, 3, 30),  // EMPLACE   ACHILLES HEAL
    makeMission(21, 21, 4, 30),  // SUPLINE   HIGH AND DRY
    makeMission(22, 20, 4, 30),  // KEEP      ASSASSINATION
    makeMission(23, 22, 4, 30),  // TESTER    HERO WARSHIP
    makeMission(24, 23, 4, 15),  // FOOT      HAMBURGER HILL
    makeMission(25, 24, 5, 30),  // FINAL     WELL, WELL, WELL!
}};

// The Boot Camp: level 10 (CAMP), first in the executable's mission order.
const Mission kBootCamp = makeMission(kBootCampMission, 10, 0, 99);

// Losing this many hogs in a mission starts costing hogs for good.
constexpr std::size_t kLossesForgiven = 2;
constexpr int kOpponentsPerIsland = kPlayableNations - 1;

const char* const kSection = "Campaign";

std::string memberSection(std::size_t index) {
    return "Hog" + std::to_string(index + 1);
}

} // namespace

const Mission& campaignMission(int number) {
    if (number == kBootCampMission) return kBootCamp;
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

bool earnsIslandBonus(int missionNumber) {
    constexpr int kIslandBonusEvery = 5;
    return missionNumber > 1 && missionNumber <= kMissionCount && missionNumber % kIslandBonusEvery == 0;
}

int specialMedals(int missionNumber) {
    // warhogs_.exe at 0x4D3560, by mission (index 0 is Boot Camp). It
    // matches the promotion points of each map: e.g. mission 24 (FOOT)
    // has five hogs to lead to a point, mission 16 (MEDIX) two guns.
    constexpr std::array<int, kMissionCount + 1> kSpecialMedals{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                                                                1, 1, 2, 2, 2, 2, 2, 3, 3, 4, 5, 0};
    if (missionNumber < 0 || missionNumber > kMissionCount) return 0;
    return kSpecialMedals[static_cast<std::size_t>(missionNumber)];
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

    // The executable (FUN_004845e0): in the last mission, against five
    // enemy teams, fewer than three deaths still earn the survival medal;
    // mission 15 (BAY) gives one special medal to start with.
    constexpr int kLastMissionDeathsAllowed = 2;
    constexpr int kFreeSpecialMission = 15;
    outcome.survivalBonus = nextMission_ == kMissionCount ? deaths <= kLastMissionDeathsAllowed : deaths == 0;
    outcome.islandConquered = earnsIslandBonus(nextMission_);
    outcome.specialMedalsOffered = specialMedals(nextMission_);
    outcome.specialMedalsWon = report.pickedUpPoints + (nextMission_ == kFreeSpecialMission ? 1 : 0);
    outcome.pointsEarned = kMissionPoints + (outcome.survivalBonus ? kSurvivalPoints : 0) +
                           (outcome.islandConquered ? kIslandPoints : 0) + outcome.specialMedalsWon;
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

void Campaign::startHardMode() {
    if (!finished() || hardMode_) return;
    hardMode_ = true;
    nextMission_ = 1;  // the Boot Camp is not played again
}

std::string Campaign::save() const {
    std::ostringstream out;
    out << "; OpenHogs campaign\n[" << kSection << "]\n";
    out << "Nation = " << nation_ << "\n";
    out << "NextMission = " << nextMission_ << "\n";
    out << "HardMode = " << (hardMode_ ? 1 : 0) << "\n";
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
    campaign.nextMission_ = ini.getInt(kSection, "NextMission", 1);  // saves from before the Boot Camp: mission 1
    campaign.points_ = ini.getInt(kSection, "Points", 0);
    campaign.hardMode_ = ini.getInt(kSection, "HardMode", 0) != 0;
    campaign.names_ = ini.getList(kSection, "Names");
    campaign.fallen_ = ini.getList(kSection, "Fallen");
    if (campaign.nation_ < 0 || campaign.nation_ >= kPlayableNations || campaign.nextMission_ < kBootCampMission ||
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

namespace {

// An enemy of a mission: the rank its map object gives it, with that rank's
// health and weapons (one rank higher in hard mode, in that rank's body).
void giveMapRank(Hog& hog, bool hardMode) {
    const auto mapRank = rankFromMapType(hog.mapRank);
    if (!mapRank) return;  // keeps its class's loadout
    const RankId rank = hardMode ? hardModeRank(*mapRank) : *mapRank;
    const RankSpec& spec = rankSpec(rank);
    if (rank != *mapRank) {
        if (const auto* body = scene::findCharacterClass(spec.classId)) hog.characterClass = body;
    }
    hog.rank = rank;
    hog.maxHealth = spec.maxHealth;
    hog.health = spec.maxHealth;
    hog.weapons = spec.weapons;
    hog.ownAmmo = std::map<WeaponId, int>(spec.uses.begin(), spec.uses.end());
    hog.weaponSlot = 0;
}

} // namespace

int deploySquad(std::vector<Hog>& hogs, const Campaign& campaign, std::size_t deployed) {
    int team = -1;
    std::size_t placed = 0;
    std::vector<Hog> result;
    for (auto& hog : hogs) {
        if (!hog.playerControlled) {
            giveMapRank(hog, campaign.hardMode());
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
        hog.ownAmmo = std::map<WeaponId, int>(rank.uses.begin(), rank.uses.end());
        hog.weaponSlot = 0;
        result.push_back(std::move(hog));
    }
    hogs = std::move(result);
    return team;
}

} // namespace openhogs::game
