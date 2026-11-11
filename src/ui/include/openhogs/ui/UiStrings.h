// UiStrings.h
//
// The texts of the menus and the HUD, taken from the game's own text
// tables so that a language pack translates them:
//
//   Language/Text/fetext  front end: menu entries, team names
//   Language/Text/gtext   in game: messages, pause menu, class names
//
// Every text has an English default, used when the table is missing or
// the entry is empty, so the menus always work. The entry numbers are
// listed in docs/formats/text.md.

#pragma once

#include "openhogs/game/Campaign.h"
#include "openhogs/game/Ranks.h"

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openhogs::ui {

struct UiStrings {
    // Main menu (fetext).
    std::string mainMenu = "MAIN MENU";
    std::string onePlayer = "ONE PLAYER";
    std::string multiPlayer = "MULTI-PLAYER";
    std::string options = "OPTIONS";
    std::string quit = "QUIT APPLICATION";
    std::string selectLevel = "MULTI-PLAYER SELECT LEVEL";
    std::string volumeControl = "VOLUME CONTROL";
    std::string master = "MASTER";
    std::string effects = "SFX";
    std::string voices = "VOICES";
    std::string music = "MUSIC";
    std::string back = "BACK";
    std::string yes = "YES";
    std::string no = "NO";
    std::string loading = "LOADING, PLEASE WAIT.";
    std::string player = "PLAYER";  // a team played by a human
    std::string cpu = "CPU";        // a team played by the computer
    std::string startGame = "START GAME";
    std::string off = "OFF";
    std::array<std::string, 4> teamLetters{"TEAM A", "TEAM B", "TEAM C", "TEAM D"};
    // The label of the `index`-th team of a match: TEAM A to D, then TEAM 5...
    std::string teamLabel(std::size_t index) const;
    std::string fieldConditions = "FIELD CONDITIONS";
    std::string pigs = "PIGS";
    std::string turnTime = "TURN TIME";
    std::string health = "HEALTH";
    std::string normal = "NORMAL";
    std::string half = "HALF";
    std::string doubled = "DOUBLE";

    // In game (gtext).
    std::string paused = "-GAME PAUSED-";
    std::string continueGame = "CONTINUE";
    std::string abortSkirmish = "ABORT SKIRMISH";
    std::string areYouSure = "ARE YOU SURE?";
    std::string victory = "VICTORY TO >S!!";
    std::string draw = "IT'S A DRAW!!";
    std::string getReady = "GET READY >S...";
    std::string enemyTurn = "THE ENEMY RETALIATES...";  // a computer team's turn
    std::string suddenDeath = "SUDDEN DEATH";            // fetext

    // Team names, in the order of scene::characterTeams() (british,
    // american, french, german, japanese, russian, teamlard).
    std::array<std::string, 7> teamNames{"TOMMY'S TROTTERS", "UNCLE HAMS HOGS", "GARLIC GRUNTS", "SOW-A-KRAUTS",
                                         "SUSHI-SWINE",      "PIGGYSTROIKA",    "TEAM LARD"};

    // Name of a team by its nation index (wraps around like game::teamNation).
    const std::string& teamName(int team) const;

    // Class names, by scene::CharacterClass id ("grunt", "ace"...).
    std::vector<std::pair<std::string, std::string>> classNames{
        {"ace", "ACE"},       {"grunt", "GRUNT"},       {"heavy", "GUNNER"},
        {"legend", "LEGEND"}, {"medic", "MEDIC"},       {"sapper", "SAPPER"},
        {"saboteur", "SABOTEUR"}, {"sniper", "SNIPER"}, {"spy", "SPY"}};

    // The name of a class; the id in capitals if it is unknown.
    std::string className(std::string_view classId) const;

    // Campaign menus (fetext).
    std::string newGame = "NEW GAME";
    std::string loadGame = "LOAD GAME";
    std::string skirmish = "DEATH MATCH";  // ONE PLAYER against the computer on any level
    std::string selectTeam = "SELECT TEAM";
    std::string startMission = "START MISSION";
    std::string promote = "PROMOTE";
    std::string swapPosition = "SWAP POSITION";
    std::string careerPath = "CAREER PATH";
    std::array<std::string, 4> careers{"HEAVY WEAPONS", "MEDIC", "ESPIONAGE", "ENGINEER"};
    // Not in the game's tables (OpenHogs' own words).
    std::string promotionPoints = "PROMOTION POINTS: >N";
    std::string campaignComplete = "SAUSTRALASIA IS OURS!";

    // Campaign texts (gtext).
    std::string missionTitle = "MISSION >2N : >S";
    std::string missionResults = "MISSION RESULTS";
    std::string missionFailed = "MISSION FAILED!";
    std::string missionAborted = "MISSION ABORTED";
    std::string levelComplete = "LEVEL COMPLETE";
    std::string survivalBonus = "SURVIVAL BONUS";
    std::string retry = "RETRY";
    std::string editSquad = "EDIT SQUAD";
    std::string rip = "RIP";

    // The hog names of each nation (fetext), in scene::characterTeams()
    // order; the campaign squad takes its names from here.
    std::array<std::vector<std::string>, 7> hogNames{{
        {"NOBBY", "GINGER", "DEN", "MONTY", "BASIL", "PONSONBY", "PERCY", "SMITH", "JONES"},
        {"CHUCKY", "SLY", "KEANU", "ABRAHAM", "JOHN-BOY", "JIM-BOB", "BOBBY-JIM", "BOBBY-JOE", "JOEY-BOB"},
        {"BASTILLE", "LE CONT", "GERARD", "M. CHIEN", "SANGLIER", "COCHON", "PORC", "GOINFRE", "GLOUTON"},
        {"HERMAN", "WOLFIE", "LEDERHOS", "HERR GEL", "HERR KUT", "SCHNITZEL", "HERR DRY", "HERR RAID", "SCHWEIN"},
        {"SHOGUN", "KENDO", "KUNG FU", "FENG SHUI", "NINJA", "SUSHI", "KEMPO", "TENKO", "RAW FISH"},
        {"YEHUDI", "HUSKI", "JETSKI", "SNOWSKI", "PESSKI", "MUSKI", "DUSKI", "RIMSKI", "MULE"},
        {"SIMON", "TAPPER", "JOHN", "ANDY", "MARK", "JAKE", "MARTYN", "IZZY", "RICKI"},
    }};

    // Mission names (gtext), by game::Mission::nameEntry.
    std::map<std::size_t, std::string> missionNames;
    std::string missionName(const game::Mission& mission) const;

    // Campaign rank names, in game::allRanks() order (from gtext).
    std::vector<std::string> rankNames;
    std::string rankName(game::RankId rank) const;
};

// Picks the texts out of the two tables (decoded entries, as from
// formats::TextTable). Either table may be empty.
UiStrings loadUiStrings(const std::vector<std::string>& frontEndText, const std::vector<std::string>& gameText);

} // namespace openhogs::ui
