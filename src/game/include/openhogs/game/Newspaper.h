// Newspaper.h - which front page the original prints after a campaign victory.
//
// After every won campaign mission (not the Boot Camp) the original shows
// a newspaper for up to ten seconds: the nation's paper (its masthead and
// newsprint), a headline-and-story bitmap and a photo. Which story and
// which photo depend on how many hogs the squad lost and on how many
// medals the mission brought; the missions that end an island, and the
// last mission, have stories of their own. This file holds only that
// choice (FUN_0045c700 in warhogs_.exe); drawing the page is the app's job.
//
// The pictures live in Language/Tims/Papers/ (see docs/formats/campaign.md).

#pragma once

#include <string>

namespace openhogs::game {

// What the choice depends on.
struct NewspaperFacts {
    int nation = 0;         // the campaign's nation (0 to 5, scene::characterTeams order)
    int missionNumber = 1;  // the mission just won, 1 to kMissionCount
    int hogsLost = 0;       // squad hogs that died in that mission
    int medals = 0;         // medals the mission brought
};

// The three pictures of the page, as paths in the game's files.
struct NewspaperPage {
    std::string paper;    // 640 x 480 background, e.g. Language/Tims/Papers/British.bmp
    std::string story;    // 314 x 204 headline and story (text1 ... text26)
    std::string picture;  // 300 x 299 photo (pic01 ... pic05)
};

// Where the story and the photo go on the 640 x 480 paper, in its pixels.
inline constexpr int kNewspaperStoryX = 23;
inline constexpr int kNewspaperStoryY = 144;
inline constexpr int kNewspaperPictureX = 309;
inline constexpr int kNewspaperPictureY = 111;

// How long the page stays up unless a key is pressed.
inline constexpr double kNewspaperSeconds = 10.0;

// The page the original prints for `facts`.
NewspaperPage newspaperFor(const NewspaperFacts& facts);

} // namespace openhogs::game
