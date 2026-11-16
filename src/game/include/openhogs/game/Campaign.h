// Campaign.h
//
// The one-player campaign: the conquest of Saustralasia, island by island,
// with a squad of eight hogs that are promoted as they survive.
//
// The missions and their order come from the game's executable (see
// docs/formats/campaign.md): 25 missions on six islands,
//
//     Hogshead 1-5, Saustralia 6-10, Trotsville 11-15, Bellyopolis 16-20,
//     Arstria 21-24, Isle of Swill 25,
//
// each on its own map, with a turn time of its own. Before them comes the
// Boot Camp (mission 0, the CAMP map), the training of Training.h.
//
// The rules follow the game's manual:
//   - the squad has eight hogs; the first five (three in mission 1) go on
//     each mission, the others stay in reserve;
//   - finishing a mission earns one promotion point (LEVEL COMPLETE), one
//     more if nobody died (SURVIVAL BONUS), one for each promotion point
//     picked up (SPECIAL BONUS) and five more after missions 5, 10, 15, 20
//     and 25 (the executable's rule, FUN_004845e0; see docs/formats/campaign.md);
//   - points buy promotions (see Ranks.h);
//   - losing three hogs in a mission costs the first one to die for good,
//     four cost the first two; losing all of them fails the mission, which
//     must be played again. A hog lost for good is replaced by a new grunt.
//
// The campaign is saved as a small text file (save() / loadCampaign()).

#pragma once

#include "openhogs/game/Hog.h"
#include "openhogs/game/Ranks.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::core {
class IniFile;
}

namespace openhogs::game {

inline constexpr int kMissionCount = 25;
// The training mission a new campaign starts with.
inline constexpr int kBootCampMission = 0;
inline constexpr std::size_t kSquadSize = 8;
inline constexpr std::size_t kDeployedHogs = 5;

// Points for a mission.
inline constexpr int kMissionPoints = 1;
inline constexpr int kSurvivalPoints = 1;
inline constexpr int kIslandPoints = 5;

// Team Lard, the enemy of the last mission (index into scene::characterTeams).
inline constexpr int kTeamLardNation = 6;
// The six nations a player can fight for (Team Lard is not one of them).
inline constexpr int kPlayableNations = 6;

struct Mission {
    int number = 0;              // 1 to 25
    std::string_view map;        // Maps/<map>.PMG ...
    std::size_t nameEntry = 0;   // gtext entry with the mission's name
    int island = 0;              // 0 Hogshead ... 5 Isle of Swill
    double turnSeconds = 60.0;
};

// Mission `number` (0, the Boot Camp, to 25). Throws std::out_of_range otherwise.
const Mission& campaignMission(int number);
// Missions 1 to 25 (the Boot Camp is campaignMission(kBootCampMission)).
std::span<const Mission> campaignMissions();

// True for the last mission of an island (5, 10, 15, 20, 24 and 25).
bool conquersIsland(int missionNumber);

// True for the missions worth the island bonus. The executable gives it
// after every fifth mission (5, 10, 15, 20, 25), so not after 24, which
// ends Arstria.
bool earnsIslandBonus(int missionNumber);

// How many special medals (promotion points to pick up) mission `number`
// offers, from the executable's table at 0x4D3560: 0 for the first eight,
// up to 5 in mission 24. The results page shows that many medals.
int specialMedals(int missionNumber);

struct SquadMember {
    std::string name;
    RankId rank = RankId::Grunt;
};

// What happened in a mission, as the game saw it.
struct MissionReport {
    bool won = false;
    // The deployed hogs (0 = the first of the squad) in the order they died.
    std::vector<std::size_t> deathOrder;
    // Promotion points picked up on the map (PROPOINT pickups).
    int pickedUpPoints = 0;
};

// What the mission meant for the campaign.
struct MissionOutcome {
    bool completed = false;
    bool survivalBonus = false;
    bool islandConquered = false;   // the island bonus was earned
    int specialMedalsWon = 0;       // promotion points picked up (can be fewer than 0)
    int specialMedalsOffered = 0;   // specialMedals() of the mission
    int pointsEarned = 0;
    std::vector<std::string> lostForGood;  // names of the hogs that will not come back
};

class Campaign {
public:
    // A new campaign for `nation` (0 to 5, in scene::characterTeams order).
    // `names` are the nation's hog names (from the text table): the first
    // eight name the squad, the rest new recruits.
    Campaign(int nation, std::vector<std::string> names);

    int nation() const { return nation_; }
    int nextMission() const { return nextMission_; }  // kMissionCount + 1 when finished
    bool finished() const { return nextMission_ > kMissionCount; }
    int points() const { return points_; }
    const std::vector<SquadMember>& squad() const { return squad_; }

    // How many hogs go on the next mission when its map has `slots` places
    // for the player's team (at most kDeployedHogs).
    std::size_t deploymentSize(std::size_t slots) const;

    // Swaps two hogs of the squad (to bring a reserve into the team).
    void swap(std::size_t first, std::size_t second);

    bool canPromote(std::size_t member, RankId rank) const;
    // Promotes a hog, paying the points. Returns false (and changes
    // nothing) if the promotion is not allowed or not affordable.
    bool promote(std::size_t member, RankId rank);

    // Applies the result of the next mission with `deployed` hogs.
    MissionOutcome recordMission(const MissionReport& report, std::size_t deployed);

    // The nation of the `enemy`-th enemy team (0 = the first) in mission
    // `missionNumber`: in each island the player meets every other nation
    // once; the last mission is against Team Lard.
    int enemyNation(int missionNumber, std::size_t enemy) const;

    // The campaign as text (an INI file), and back.
    std::string save() const;

private:
    Campaign() = default;
    std::string newRecruitName() const;
    friend Campaign loadCampaign(const core::IniFile& ini);

    int nation_ = 0;
    int nextMission_ = kBootCampMission;
    int points_ = 0;
    std::vector<SquadMember> squad_;
    std::vector<std::string> names_;
    std::vector<std::string> fallen_;  // hogs lost for good: their names are not reused
};

// Reads a campaign written by Campaign::save(). Throws core::FormatError
// for a file that is not a valid campaign.
Campaign loadCampaign(const core::IniFile& ini);

// Puts the first `deployed` squad hogs in place of the player's team on a
// map: the map's hogs marked for the player, in order, take the squad's
// names, ranks, health, weapons and body models; the map's extra player
// hogs are removed. Returns the player's team (-1 if the map has none).
int deploySquad(std::vector<Hog>& hogs, const Campaign& campaign, std::size_t deployed);

// Places for the player's team on a map: its hogs marked for the player.
std::size_t playerSlots(const std::vector<Hog>& hogs);

} // namespace openhogs::game
