// FrontEnd.h
//
// The menus shown before a match, as a small state machine:
//
//     MAIN MENU ── ONE PLAYER ───> NEW GAME ── SELECT TEAM ──> (new campaign)
//        │                         LOAD GAME ────────────────> (saved campaign)
//        │                         DEATH MATCH ── SELECT LEVEL ── a map ──> (match against the computer)
//        │    ── MULTI-PLAYER ─> SELECT LEVEL ── a map ──> PLAYERS ── START GAME ──> (match)
//        │    ── OPTIONS ──────> VOLUME CONTROL (sliders)
//        │    ── QUIT ─────────> ARE YOU SURE? ── YES ──> (quit)
//
// The campaign itself is run by CampaignMenu. DEATH MATCH is a skirmish
// against the computer: the team that plays first is the player's, the
// others the computer's. MULTI-PLAYER lets each team of the map be played by a person
// (PLAYER) or the computer (CPU).
//
// The teams of a map are only known once its files are read, so picking a
// level asks the application to read them (Request::LevelChosen) and give
// them back with setLevelTeams().
//
// "Back" (Esc) returns to the previous page; on the main menu it asks
// whether to quit. The front end only decides; the application starts the
// match, changes the volume or quits when told to (FrontEndResult).

#pragma once

#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

#include <string>
#include <vector>

namespace openhogs::ui {

enum class FrontEndScreen { Main, OnePlayer, SelectTeam, SelectLevel, Players, Options, ConfirmQuit };

// Volume levels as shown by the sliders: 0 (silent) .. kMaxVolumeLevel.
inline constexpr int kMaxVolumeLevel = 10;

struct VolumeLevels {
    int master = kMaxVolumeLevel;
    int effects = kMaxVolumeLevel;
    int speech = kMaxVolumeLevel;
    int music = kMaxVolumeLevel;
};

// What a menu input led to, for sound and for the application.
enum class MenuSound { None, Move, Select, Back };

struct FrontEndResult {
    enum class Request {
        None,
        LevelChosen,  // read the teams of `map` and call setLevelTeams()
        StartMatch,   // play `map`; `computerTeams` are played by the computer
        NewCampaign,  // start a campaign for `nation`
        LoadCampaign, // continue the saved campaign
        Quit,
        VolumesChanged,
    };
    Request request = Request::None;
    std::string map;
    std::vector<int> computerTeams;
    int nation = 0;
    MenuSound sound = MenuSound::None;
};

class FrontEnd {
public:
    FrontEnd(UiStrings strings, std::vector<std::string> maps, VolumeLevels volumes);

    // Whether LOAD GAME is offered (a saved campaign exists).
    void setSavedCampaign(bool exists);

    FrontEndResult handle(MenuInput input);

    FrontEndScreen screen() const { return screen_; }
    const Menu& menu() const;
    VolumeLevels volumes() const { return volumes_; }

    // Goes back to the main menu (e.g. after a match).
    void showMainMenu();

    // The teams of the level just chosen, the one that plays first first
    // (game::mapTeams). For ONE PLAYER the match starts straight away; for
    // MULTI-PLAYER the PLAYERS page opens. A map without teams goes back
    // to the level list.
    FrontEndResult setLevelTeams(const std::vector<int>& teams);

private:
    FrontEndResult handleMain(const MenuResponse& response);
    FrontEndResult handleOnePlayer(const MenuResponse& response);
    FrontEndResult handleSelectTeam(const MenuResponse& response);
    FrontEndResult handleSelectLevel(const MenuResponse& response);
    FrontEndResult handlePlayers(const MenuResponse& response);
    FrontEndResult handleOptions(const MenuResponse& response);
    FrontEndResult handleConfirmQuit(const MenuResponse& response);
    void open(FrontEndScreen screen);

    UiStrings strings_;
    std::vector<std::string> maps_;
    VolumeLevels volumes_;
    FrontEndScreen screen_ = FrontEndScreen::Main;
    bool onePlayer_ = false;     // the level list was opened from ONE PLAYER
    std::string chosenMap_;
    std::vector<int> levelTeams_;
    Menu main_;
    Menu onePlayerMenu_;
    Menu selectTeam_;
    Menu selectLevel_;
    Menu players_;
    Menu options_;
    Menu confirmQuit_;
};

} // namespace openhogs::ui
