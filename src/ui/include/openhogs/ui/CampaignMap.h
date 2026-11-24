// CampaignMap.h
//
// The campaign map shown before every campaign mission but the Boot Camp,
// as the original executable draws it (FUN_00482c30 and the functions it
// calls; docs/formats/campaign.md, "The campaign map"):
//
//   1. The map of Saustralasia (Language/Tims/PigMap/BigMap.bmp) with its
//      25 territories, one per mission, each in the colour of the nation
//      holding it, and the six islands' names. The next mission's
//      territory flickers white. Two seconds, or until a key.
//   2. The territory grows into the picture of its island (hogphy.mad ...
//      iosphy.mad) while its colour fades out and the picture fades in,
//      a shadow trailing behind: 32 ticks of 50 ms.
//   3. The island: a flag on each of its missions, in the colour of the
//      nation holding it, planted one every three ticks; then four arrows
//      in the player's colour bounce around the next mission. Ten seconds,
//      or until a key.
//
// This file holds the executable's tables (positions in the 640 x 480
// picture) and the timing; the game application draws it.

#pragma once

#include <array>
#include <cstddef>

#include <glm/vec3.hpp>

namespace openhogs::ui {

// --- The executable's tables ----------------------------------------------

struct MapPoint {
    int x = 0;
    int y = 0;
};

inline constexpr int kMapTerritories = 25;  // missions 1 to 25
inline constexpr int kMapIslands = 6;

// Top-left corner of each territory's picture (PMAPTIMS.MAD entry i is
// mission i + 1's territory), from the table at VA 0x4D3658.
inline constexpr std::array<MapPoint, kMapTerritories> kTerritoryPositions{{
    {92, 159}, {140, 256}, {99, 233}, {155, 183}, {136, 113},    // Hogshead
    {239, 166}, {248, 110}, {210, 189}, {196, 113}, {266, 165},  // Saustralia
    {267, 256}, {242, 242}, {198, 285}, {217, 326}, {241, 265},  // Trotsville
    {370, 185}, {302, 198}, {309, 159}, {312, 109}, {345, 94},   // Bellyopolis
    {446, 169}, {426, 254}, {410, 168}, {406, 215},              // Arstria
    {498, 189},                                                  // Isle of Swill
}};

// The islands' names (LANGTIMS.MAD entry i), same table.
inline constexpr std::array<MapPoint, kMapIslands> kIslandNamePositions{{
    {129, 230}, {209, 170}, {226, 284}, {320, 148}, {414, 209}, {476, 173},
}};
// The islands' pictures: where they end up and their size (VA 0x4D4A00).
inline constexpr std::array<MapPoint, kMapIslands> kIslandPicturePositions{{
    {226, 81}, {321, 84}, {330, 193}, {10, 99}, {481, 86}, {334, 160},
}};
inline constexpr std::array<MapPoint, kMapIslands> kIslandPictureSizes{{
    {250, 322}, {250, 319}, {250, 250}, {294, 291}, {150, 326}, {150, 150},
}};

// Where each mission is on its island's picture (VA 0x4D4AC0).
inline constexpr std::array<MapPoint, kMapTerritories> kMissionPointsOnIsland{{
    {88, 159}, {154, 310}, {70, 270}, {175, 231}, {135, 123},
    {153, 239}, {177, 105}, {93, 279}, {77, 159}, {207, 244},
    {203, 128}, {155, 55}, {69, 168}, {104, 219}, {145, 147},
    {156, 199}, {85, 225}, {54, 167}, {103, 83}, {171, 119},
    {123, 99}, {89, 253}, {56, 71}, {63, 169},
    {72, 92},
}};

// PMAPTIMS.MAD: 25 territories, then these.
inline constexpr std::size_t kArrowRightPicture = 25;  // ar1: right of the mission, pointing left
inline constexpr std::size_t kArrowLeftPicture = 26;   // ar2: left of it, pointing right
inline constexpr std::size_t kArrowAbovePicture = 27;  // ar3
inline constexpr std::size_t kArrowBelowPicture = 28;  // ar4
inline constexpr std::size_t kFlagPicture = 29;
inline constexpr std::size_t kFlagPolePicture = 30;
inline constexpr int kFlagWidth = 40;
inline constexpr int kFlagHeight = 32;
inline constexpr int kFlagPoleWidth = 8;
inline constexpr int kFlagPoleHeight = 62;

// The nations' colours on the map (VA 0x4D34E0), in scene::characterTeams
// order (the executable keeps them in its own nation order, mapped by
// FUN_004508e0): British, American, French, German, Japanese, Russian,
// Team Lard; the eighth is for a territory nobody holds.
inline constexpr std::array<glm::vec3, 8> kNationMapColours{{
    {39.0f, 170.0f, 105.0f},
    {143.0f, 172.0f, 212.0f},
    {110.0f, 125.0f, 195.0f},
    {130.0f, 130.0f, 130.0f},
    {205.0f, 187.0f, 98.0f},
    {214.0f, 123.0f, 133.0f},
    {158.0f, 46.0f, 158.0f},
    {109.0f, 56.0f, 32.0f},
}};
// The islands' names are tinted (250, 240, 235).
inline constexpr glm::vec3 kIslandNameColour{250.0f, 240.0f, 235.0f};

// Which island picture goes with a mission (1 to 25): 0 Hogshead ...
// 4 Arstria, 5 the Isle of Swill (mission 25 only).
int mapIslandOfMission(int mission);
// The missions (1 to 25) whose flags stand on island `island`.
struct MissionRange {
    int first = 1;
    int last = 1;  // inclusive
};
MissionRange mapIslandMissions(int island);

// --- The animation ----------------------------------------------------------

inline constexpr double kMapSeconds = 2.0;          // the whole map, before the zoom
inline constexpr double kMapTickSeconds = 0.05;     // the executable's 50 ms steps
inline constexpr int kZoomTicks = 32;
inline constexpr double kIslandSeconds = 10.0;      // the island, unless a key is pressed
inline constexpr double kFlickerSeconds = 0.016;    // the next territory's flicker (bit 4 of the clock in ms)

// How far the zoom has gone at each of its 32 ticks, in hundredths
// (VA 0x4D4A40): slow, then faster, then held.
inline constexpr std::array<int, kZoomTicks> kZoomCurve{
    0, 1, 2, 3, 5, 7, 9, 11, 14, 17, 20, 23, 27, 31, 35, 39,
    44, 49, 54, 59, 65, 71, 77, 83, 90, 97, 100, 100, 100, 100, 100, 100};
// The shadow under the growing picture follows at three quarters of the
// way, as black of up to 64/255.
inline constexpr float kZoomShadowLag = 0.75f;
inline constexpr float kZoomShadowAlpha = 64.0f / 255.0f;

// The arrows' bounce, by a step that goes 0..5 and back every 50 ms
// (VA 0x4D4B88).
inline constexpr std::array<int, 6> kArrowBounce{0, 1, 2, 4, 7, 12};

enum class CampaignMapPhase { Map, Zoom, Island, Done };

class CampaignMapAnimation {
public:
    // For the next mission (1 to 25).
    explicit CampaignMapAnimation(int mission);

    int mission() const { return mission_; }
    int island() const { return mapIslandOfMission(mission_); }
    CampaignMapPhase phase() const { return phase_; }
    bool done() const { return phase_ == CampaignMapPhase::Done; }

    void update(double seconds);
    // A key: it ends the map's two seconds or the island's ten; the zoom
    // cannot be cut short (as in the original).
    void keyPressed();

    // Map: whether the next mission's territory is white this instant.
    bool territoryFlashing() const;
    // Zoom: how far it has gone, 0 to 1.
    float zoomProgress() const;
    // Island: how many of its flags stand (from the first mission of the
    // island), and whether the arrows are shown, with their bounce.
    int flagsShown() const;
    bool arrowsShown() const;
    int arrowBounce() const;

private:
    int ticks() const;  // whole 50 ms steps into the current phase

    int mission_;
    CampaignMapPhase phase_ = CampaignMapPhase::Map;
    double phaseSeconds_ = 0.0;
};

} // namespace openhogs::ui
