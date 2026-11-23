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
    std::string reallyQuit = "REALLY QUIT APP?";  // the question before quitting
    std::string selectLevel = "MULTI-PLAYER SELECT LEVEL";
    std::string volumeControl = "VOLUME CONTROL";
    std::string audio = "AUDIO";        // OPTIONS: the volume screen
    std::string controls = "CONTROLS";
    std::string credits = "CREDITS";
    std::string mode = "MODE";          // VOLUME CONTROL: the sound mode
    std::array<std::string, 3> soundModes{"MONO", "STEREO", "SURROUND"};
    std::string on = "ON";
    // CREDITS: name and job, one row each (fetext 503-652, the original's
    // credits; empty without the game's text tables).
    std::vector<std::pair<std::string, std::string>> creditRows;
    // CONTROLS: the original's names of the actions (fetext 349-360).
    std::array<std::string, 12> controlNames{"FORWARD", "BACK", "LEFT", "RIGHT", "ACTION", "FIRE // SELECT",
                                             "JUMP", "WEAPONS MENU", "AIM // LOOK", "ANGLE DOWN", "ANGLE UP", "PAUSE"};
    // ...and the PC version's extra keys, which have no name in the tables.
    std::string nextHog = "NEXT HOG";
    std::string previousWeapon = "PREVIOUS WEAPON";  // until the weapon menu replaces Q/E
    std::string nextWeapon = "NEXT WEAPON";
    std::string endTurn = "END TURN";
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
    std::string deathmatchLimit = "DEATHMATCH LIMIT";  // fetext 299
    std::string selectPig = "SELECT PIG";              // fetext 302
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
    std::string findsProvisions = ">S FINDS USEFUL PROVISIONS!";  // a crate picked up
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
    std::string rename = "RENAME";
    std::string saveTeam = "SAVE TEAM";
    std::string loadArmy = "LOAD";             // the loading board's title (screen 10)
    std::string saveArmy = "SAVE ARMY";        // the saving board's title (screen 21)
    std::string nameYourPig = "PLEASE NAME YOUR PIG";
    std::string nameYourTeam = "PLEASE NAME YOUR TEAM";
    std::string emptySlot = "---";             // a slot with nothing saved (the executable's own text)
    std::string missionNumber = "MISSION ";    // the saving board's panel: "MISSION 4" (fetext 248)
    // The team-name cheat that gives 99 promotion points (fetext 747).
    std::string naughtyPigs = "NAUGHTY PIGS";
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
    std::string specialBonus = "SPECIAL BONUS";
    std::string none = "NONE";  // a medal not won (gtext 96, also "no weapon")

    // The battle results after a skirmish or a match between people
    // (gtext 195-208), and the special awards in their order: THE
    // ANNIHLATOR!!!, TRIPLE RIP REAPER!!!, MASTER SADIST!!!, DOUBLE DEATH
    // DEALER!, SADISM AWARD!, PIG OF THE BATTLE (game::SpecialAwardKind).
    // The Boot Camp's instructions, gtext 211 to 234 (game/Training.h).
    std::vector<std::string> trainingTexts{
        "FOLLOW THE GREEN PATH AND COLLECT THE CRATE.",
        "PRESS RETURN BUTTON FOR SKILL MENU.",
        "PRESS SPACE TO SELECT YOUR WEAPON.",
        "PRESS SPACE TO ATTACK THE DUMMY.",
        "FOLLOW THE YELLOW PATH AND COLLECT THE CRATE.",
        " ",
        "PRESS RETURN BUTTON FOR SKILL MENU AND SPACE TO SELECT YOUR WEAPON.",
        "HOLD CTRL BUTTON TO AIM - PRESS SPACE TO FIRE.",
        "COLLECT THE CRATE.",
        "HOLD CTRL BUTTON TO AIM - PRESS SPACE TO FIRE.",
        "FOLLOW THE RED PATH AND COLLECT THE CRATE.",
        "HOLD SPACE TO SET POWER AND RELEASE TO THROW - PRESS SPACE AGAIN TO DETONATE.",
        "HOLD SPACE BUTTON TO SET POWER AND RELEASE TO THROW - PRESS SPACE AGAIN TO DETONATE.",
        "FOLLOW THE BLUE PATH AND COLLECT THE CRATE.",
        "SELECT YOUR WEAPON THEN USE PAGE UP AND PAGE DOWN TO SET A FLAT AIM ANGLE.",
        "FOLLOW THE PURPLE PATH AND CLIMB OVER THE GATE.",
        "USE SHIFT BUTTON TO JUMP THE GAP.",
        "FOLLOW THE PATH THROUGH THE MINEFIELD AND COLLECT THE CRATE.",
        "PRESS SPACE TO SET CHARGE AND THEN STAND CLEAR!",
        " ",
        "ENTER THE BUILDING AND COLLECT THE CRATE.",
        "FOLLOW THE PATH THROUGH THE MINEFIELD AND COLLECT THE CRATE.",
        "USE BACKSPACE BUTTON TO ENTER AND EXIT BUILDINGS OR VEHICLES.",
        "HOLD SPACE TO SET POWER AND RELEASE TO FIRE."};
    // The instruction of gtext entry `text` (211-234), or "".
    std::string trainingText(int text) const;

    std::string battleResults = "BATTLE RESULTS";
    std::string enemyGrilled = "ENEMY GRILLED";
    std::string friendsFried = "FRIENDS FRIED";
    std::string victoryBonus = "VICTORY BONUS";
    std::string specialAwards = "SPECIAL AWARDS";
    std::string total = "TOTAL";
    std::string wins = "WINS!!";
    std::string specialAward = "SPECIAL AWARD";
    std::vector<std::string> awardNames{"THE ANNIHLATOR!!!", "TRIPLE RIP REAPER!!!", "MASTER SADIST!!!",
                                        "DOUBLE DEATH DEALER!", "SADISM AWARD!", "PIG OF THE BATTLE"};
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

    // Level names (gtext 11 + level number), by game::LevelInfo::nameEntry
    // (which is also game::Mission::nameEntry).
    std::map<std::size_t, std::string> missionNames;
    std::string missionName(const game::Mission& mission) const;
    // A map's level name (ROUTINE PATROL for ROAD, THE LAKE for LAKE...),
    // or the map's own name for maps that are not levels.
    std::string levelName(std::string_view map) const;

    // Campaign rank names, in game::allRanks() order (from gtext).
    std::vector<std::string> rankNames;
    std::string rankName(game::RankId rank) const;
};

// Picks the texts out of the two tables (decoded entries, as from
// formats::TextTable). Either table may be empty.
UiStrings loadUiStrings(const std::vector<std::string>& frontEndText, const std::vector<std::string>& gameText);

} // namespace openhogs::ui
