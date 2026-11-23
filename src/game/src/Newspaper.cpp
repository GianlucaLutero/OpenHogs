// Newspaper.cpp - see Newspaper.h for an overview.

#include "openhogs/game/Newspaper.h"

#include "openhogs/game/Campaign.h"
#include "openhogs/game/Levels.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace openhogs::game {

namespace {

const std::string kFolder = "Language/Tims/Papers/";

// The papers in scene::characterTeams order (Japanese before Russian).
constexpr std::array<std::string_view, 6> kPapers{"British", "American", "French", "German", "Japan", "Russian"};

// The levels whose victory has its own story: the last mission of each
// island and the final mission, in the order of their stories (text21 is
// "HOGSHEAD FALLS!", the end of the first island).
constexpr std::array<int, 6> kIslandEndLevels{2, 7, 16, 19, 23, 24};
constexpr int kFirstIslandEndStory = 21;

// The squad slots the original counts: only the first five.
constexpr int kCountedHogs = 5;

// The story of an island end, or 0 for other missions.
int islandEndStory(int missionNumber) {
    const auto level = findLevel(campaignMission(missionNumber).map);
    if (!level) return 0;
    const auto found = std::find(kIslandEndLevels.begin(), kIslandEndLevels.end(), level->number);
    if (found == kIslandEndLevels.end()) return 0;
    return kFirstIslandEndStory + static_cast<int>(found - kIslandEndLevels.begin());
}

} // namespace

NewspaperPage newspaperFor(const NewspaperFacts& facts) {
    const int lost = std::clamp(facts.hogsLost, 0, kCountedHogs);
    // Each kind of victory has four stories; the mission number picks one,
    // so two missions in a row never print the same words.
    const int variant = facts.missionNumber & 3;

    int picture = 0;
    int story = 0;
    if (lost == 0 && facts.medals > 1) {
        picture = 1;  // VICTORY! NATION ECSTATIC
        story = variant + 1;
    } else if (lost == 0) {
        picture = 2;  // ENEMY GIVEN A PROPER SEEING TO
        story = variant + 5;
    } else if (lost <= 2) {
        picture = 3;  // VICTORY ONCE MORE
        story = variant + 9;
    } else if (lost == 3) {
        picture = 4;  // THEY DON'T LIKE IT UP 'EM
        story = variant + 13;
    } else {
        picture = 5;  // OUR LADS ROGERED
        story = variant + 16;
    }
    // An island falls (or the war ends) unless the whole squad was lost.
    if (lost < kCountedHogs) {
        if (const int special = islandEndStory(facts.missionNumber); special != 0) story = special;
    }

    const auto nation = static_cast<std::size_t>(std::clamp(facts.nation, 0, static_cast<int>(kPapers.size()) - 1));
    NewspaperPage page;
    page.paper = kFolder + std::string(kPapers[nation]) + ".bmp";
    page.story = kFolder + "text" + std::to_string(story) + ".bmp";
    page.picture = kFolder + "pic0" + std::to_string(picture) + ".bmp";
    return page;
}

} // namespace openhogs::game
