// Cutscenes.h
//
// When the original plays its videos (Language/Streams/*.bik), as found in
// warhogs_.exe (FUN_0047de90, see docs/formats/executable.md):
//
//   - at start-up: the Infogrames logo, the Sheffield studio logo, then the
//     intro in two parts (FMV 01, FMV 02); any key skips the rest;
//   - in the campaign, after the last mission of an island, when the next
//     mission is 6, 11, 16 or 21: FMV 03, 04, 05, 06; before mission 25
//     (Arstria conquered): FMV 07;
//   - when the campaign is won: FMV 08, then FMV 09.
//
// In the PC release FMV 02 and FMV 09 are one-byte placeholders (the
// PlayStation version has them); a player skips over files that cannot be
// played.

#pragma once

#include <string>
#include <vector>

namespace openhogs::game {

// The videos played when the game starts, in order.
std::vector<std::string> introCutscenes();

// The videos to play after a campaign mission was completed: `nextMission`
// is the mission now ahead (Campaign::nextMission(), 26 when the campaign
// is won).
std::vector<std::string> campaignCutscenes(int nextMission);

} // namespace openhogs::game
