// FrontEnd.h
//
// The menus shown before a match, as a small state machine:
//
//     MAIN MENU ── ONE PLAYER ───> NEW GAME ── SELECT TEAM ──> (new campaign)
//        │                         LOAD GAME ────────────────> (saved campaign)
//        │                         DEATH MATCH ── SELECT LEVEL ── a map ──> (match against the computer)
//        │    ── MULTI-PLAYER ─> SELECT LEVEL ── a map ──> PLAYERS ── START GAME ──> (match)
//        │                                                     └─ FIELD CONDITIONS (hogs per team,
//        │                                                        turn time, health, sudden death)
//        │    ── OPTIONS ──────> AUDIO ── VOLUME CONTROL (MODE, MASTER, SFX, VOICES)
//        │                       CONTROLS (the keys) · CREDITS (the original's, rolling)
//        │    ── QUIT ─────────> REALLY QUIT APP? ── YES ──> (quit)
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

#include "openhogs/game/FieldConditions.h"
#include "openhogs/ui/KeyBindings.h"
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::ui {

enum class FrontEndScreen {
    Main,
    OnePlayer,
    SelectTeam,
    SelectLevel,
    Players,
    FieldConditions,
    Options,
    Volume,
    Controls,
    Credits,
    ConfirmQuit,
    BattleResults,  // after a skirmish or a match between people
};

// Volume levels as shown by the sliders: 0 (silent) .. kMaxVolumeLevel.
inline constexpr int kMaxVolumeLevel = 10;

// The original's sound modes, in the order of its MODE entry.
enum class SoundModeChoice { Mono, Stereo, Surround };

// The volume settings. The original's VOLUME CONTROL shows MODE, MASTER,
// SFX and VOICES (on or off: off is speech 0); the music level is not on
// it (the original had no music control) and only comes from the
// configuration.
struct VolumeLevels {
    int master = kMaxVolumeLevel;
    int effects = kMaxVolumeLevel;
    int speech = kMaxVolumeLevel;
    int music = kMaxVolumeLevel;
    SoundModeChoice mode = SoundModeChoice::Stereo;
};

// What a menu input led to, for sound and for the application.
enum class MenuSound { None, Move, Select, Back };

// How a match is set up from the menus.
struct MatchSettings {
    std::vector<int> computerTeams;      // played by the computer
    std::map<int, int> teamNations;      // team -> nation (empty: the usual ones)
    game::FieldConditions conditions;    // teams switched off, hogs per team, health
    double turnSeconds = 60.0;
    int suddenDeathRound = 0;            // 0 = no sudden death
    int deathmatchLimit = 0;             // points to win a deathmatch; 0 = last team standing
    bool selectPig = false;              // SELECT PIG: the hog is chosen at each turn
};

struct FrontEndResult {
    enum class Request {
        None,
        LevelChosen,  // read the teams of `map` and call setLevelTeams()
        StartMatch,   // play `map` with `settings`
        NewCampaign,  // start a campaign for `nation`
        LoadCampaign, // continue the saved campaign
        Quit,
        VolumesChanged,
        ControlsChanged,  // a key was given to a control: save keyBindings()
    };
    Request request = Request::None;
    std::string map;
    MatchSettings settings;
    int nation = 0;
    MenuSound sound = MenuSound::None;
};

class FrontEnd {
public:
    FrontEnd(UiStrings strings, std::vector<std::string> maps, VolumeLevels volumes,
             KeyBindings keyBindings = KeyBindings::defaults());

    // Whether LOAD GAME is offered (a saved campaign exists).
    void setSavedCampaign(bool exists);

    FrontEndResult handle(MenuInput input);

    FrontEndScreen screen() const { return screen_; }
    const Menu& menu() const;
    // The menu that stays on screen behind the current one (OPTIONS behind
    // VOLUME CONTROL, as the original), or nullptr.
    const Menu* menuUnder() const;

    // Who plays each team on the MULTI-PLAYER page, in team order:
    // 0 a person, 1 the computer, 2 nobody (switched off).
    std::vector<int> teamControls() const;
    VolumeLevels volumes() const { return volumes_; }

    // CONTROLS: activating a control's row makes it wait for a key (the row
    // blinks, as in the original). While a control waits, the application
    // sends the next key pressed to keyPressed() instead of menu inputs:
    // Escape cancels, any other key is given to the control.
    std::optional<Control> waitingForKey() const { return waitingControl_; }
    FrontEndResult keyPressed(std::string_view key);
    const KeyBindings& keyBindings() const { return keyBindings_; }

    // Goes back to the main menu (e.g. after a match).
    void showMainMenu();

    // The BATTLE RESULTS page after a match: `lines` are written on it
    // (see battleResultLines()); CONTINUE goes back to the main menu.
    void showBattleResults(std::vector<std::string> lines);
    const std::vector<std::string>& resultLines() const { return resultLines_; }

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
    bool nationTaken(int nation, std::size_t exceptTeam) const;  // by another playing team
    bool nationTakenTwice() const;
    void skipTakenNation(std::size_t team, int step);
    FrontEndResult handleFieldConditions(const MenuResponse& response);
    MatchSettings fieldSettings() const;
    FrontEndResult handleOptions(const MenuResponse& response);
    FrontEndResult handleVolume(const MenuResponse& response);
    FrontEndResult handleOptionPage(const MenuResponse& response);  // CREDITS
    FrontEndResult handleControls(const MenuResponse& response);
    void buildControlsMenu();
    FrontEndResult handleConfirmQuit(const MenuResponse& response);
    void open(FrontEndScreen screen);

    UiStrings strings_;
    std::vector<std::string> maps_;
    VolumeLevels volumes_;
    int speechWhenOn_ = kMaxVolumeLevel;  // the speech level VOICES ON goes back to
    KeyBindings keyBindings_;
    std::optional<Control> waitingControl_;  // CONTROLS: the row waiting for a key
    FrontEndScreen screen_ = FrontEndScreen::Main;
    bool onePlayer_ = false;     // the level list was opened from ONE PLAYER
    std::string chosenMap_;
    std::vector<int> levelTeams_;
    Menu main_;
    Menu onePlayerMenu_;
    Menu selectTeam_;
    Menu selectLevel_;
    Menu players_;
    Menu fieldConditions_;
    Menu options_;
    Menu volume_;
    Menu controls_;
    Menu credits_;
    Menu confirmQuit_;
    Menu battleResults_;
    std::vector<std::string> resultLines_;
};

} // namespace openhogs::ui
