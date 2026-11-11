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

#include <array>
#include <cstddef>
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

    // In game (gtext).
    std::string paused = "-GAME PAUSED-";
    std::string continueGame = "CONTINUE";
    std::string abortSkirmish = "ABORT SKIRMISH";
    std::string areYouSure = "ARE YOU SURE?";
    std::string victory = "VICTORY TO >S!!";
    std::string draw = "IT'S A DRAW!!";
    std::string getReady = "GET READY >S...";

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
};

// Picks the texts out of the two tables (decoded entries, as from
// formats::TextTable). Either table may be empty.
UiStrings loadUiStrings(const std::vector<std::string>& frontEndText, const std::vector<std::string>& gameText);

} // namespace openhogs::ui
