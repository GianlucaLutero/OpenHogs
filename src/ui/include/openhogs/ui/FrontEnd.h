// FrontEnd.h
//
// The menus shown before a match, as a small state machine:
//
//     MAIN MENU ── MULTI-PLAYER ──> SELECT LEVEL ── a map ──> (start the match)
//        │    ── OPTIONS ───────> VOLUME CONTROL (sliders)
//        │    ── QUIT ──────────> ARE YOU SURE? ── YES ──> (quit)
//        │
//        └─ ONE PLAYER is shown but disabled until the campaign exists.
//
// "Back" (Esc) always returns to the main menu; on the main menu it asks
// whether to quit. The front end only decides; the application starts the
// match, changes the volume or quits when told to (FrontEndResult).

#pragma once

#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

#include <string>
#include <vector>

namespace openhogs::ui {

enum class FrontEndScreen { Main, SelectLevel, Options, ConfirmQuit };

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
    enum class Request { None, StartMatch, Quit, VolumesChanged };
    Request request = Request::None;
    std::string map;  // StartMatch: the chosen map
    MenuSound sound = MenuSound::None;
};

class FrontEnd {
public:
    FrontEnd(UiStrings strings, std::vector<std::string> maps, VolumeLevels volumes);

    FrontEndResult handle(MenuInput input);

    FrontEndScreen screen() const { return screen_; }
    const Menu& menu() const;
    VolumeLevels volumes() const { return volumes_; }

    // Goes back to the main menu (e.g. after a match).
    void showMainMenu();

private:
    FrontEndResult handleMain(const MenuResponse& response);
    FrontEndResult handleSelectLevel(const MenuResponse& response);
    FrontEndResult handleOptions(const MenuResponse& response);
    FrontEndResult handleConfirmQuit(const MenuResponse& response);
    void open(FrontEndScreen screen);

    UiStrings strings_;
    std::vector<std::string> maps_;
    VolumeLevels volumes_;
    FrontEndScreen screen_ = FrontEndScreen::Main;
    Menu main_;
    Menu selectLevel_;
    Menu options_;
    Menu confirmQuit_;
};

} // namespace openhogs::ui
