// GameVersion.h
//
// Which release of Hogs of War a match follows where the PC and the
// PlayStation versions differ (docs/formats/executable.md, "Differences
// between versions"; the Hogs of War Wiki's single-player page):
//
//   - on the PlayStation the computer's hogs walk to better places before
//     they shoot; on PC they never walk (part of the AI code was missing
//     from the PC port, as one of its developers said);
//   - the PlayStation loops a team's tune through its turn; the PC plays
//     it once;
//   - only the PlayStation has the hard mode: after the campaign is won it
//     starts again from mission 1 with every enemy one rank higher, and
//     ends with a second film (FMV 09, an empty file on PC).
//
// The PC rules are the default, as OpenHogs plays the PC release's files.
// Settings: [Game] Version = pc or ps1 (config/openhogs.ini).

#pragma once

#include <optional>
#include <string_view>

namespace openhogs::game {

enum class GameVersion { Pc, PlayStation };

struct VersionRules {
    bool computerWalks = false;
    bool turnTuneLoops = false;
    bool hardMode = false;
};

VersionRules versionRules(GameVersion version);

// "pc" or "ps1" (any case); empty for anything else.
std::optional<GameVersion> parseGameVersion(std::string_view text);

} // namespace openhogs::game
