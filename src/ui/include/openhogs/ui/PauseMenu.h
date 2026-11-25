// PauseMenu.h
//
// The menu over a paused match, as the PC executable has it (FUN_0045a9b0):
//
//     -GAME PAUSED-
//     CONTINUE        back to the match (Esc does the same)
//     MASTER VOLUME   a bar of 20 notches, changed with left / right
//     SFX VOLUME      the same for the sound effects
//     SPEECH ON/OFF   the hogs' and the sergeant's voices
//     ABORT MISSION   (ABORT SKIRMISH outside the campaign)
//                     -> ARE YOU SURE?  YES: leave the match / NO: back
//
// Like FrontEnd it only decides; the game pauses, resumes, changes the
// volumes or ends the match when told to. The layout is in
// OriginalPauseLayout below.

#pragma once

#include "openhogs/ui/FrontEnd.h"  // MenuSound, VolumeLevels
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

namespace openhogs::ui {

struct PauseResult {
    enum class Request { None, Resume, Abort, VolumesChanged };
    Request request = Request::None;
    MenuSound sound = MenuSound::None;
    VolumeLevels volumes;  // VolumesChanged: the new levels
};

class PauseMenu {
public:
    // `campaign`: the abort item says ABORT MISSION.
    PauseMenu(const UiStrings& strings, bool campaign);

    // Shows the first page again with the volumes in use (called each time
    // the game pauses).
    void reset(const VolumeLevels& volumes);

    PauseResult handle(MenuInput input);

    const Menu& menu() const { return confirming_ ? confirm_ : main_; }
    bool confirming() const { return confirming_; }

    // The menu's items, in their order on the screen.
    enum Item { kContinue, kMasterVolume, kSfxVolume, kSpeech, kAbort };
    enum ConfirmItem { kYes, kNo };

    // A volume bar's lit notches out of kVolumeNotches (the executable
    // shows a 0..100 volume as volume / 5 bars).
    static constexpr int kVolumeNotches = 20;
    int litNotches(Item volumeItem) const;

private:
    Menu main_;
    Menu confirm_;
    bool confirming_ = false;
    VolumeLevels volumes_;
    int speechLevel_ = kMaxVolumeLevel;  // brought back when SPEECH is switched on again
};

// Where the executable puts the pause menu, in pixels of the 640 x 480
// screen around its centre (FUN_0045a9b0, FUN_0045b580, the rectangles
// set up by the HUD's constructor).
struct OriginalPauseLayout {
    // The panel: 260 x 300 round the centre, a black veil at half strength
    // inside a frame of dashtims' pause1..pause8 (16 x 16 corners and
    // stretched edges, drawn 9 and 14 pixels out from its left and top).
    static constexpr float kHalfWidth = 130.0f;
    static constexpr float kHalfHeight = 150.0f;
    // The ARE YOU SURE? panel: 240 wide, from 115 above the centre to it.
    static constexpr float kConfirmHalfWidth = 120.0f;
    static constexpr float kConfirmTop = -115.0f;
    // The rows, from the panel's top: the title, then the five items.
    static constexpr float kTitleY = 30.0f;
    static constexpr float kItemY[5] = {85.0f, 115.0f, 165.0f, 225.0f, 265.0f};
    // A volume bar: 25 pixels under its item's name, starting 117 left of
    // the centre.
    static constexpr float kBarDrop = 25.0f;
    static constexpr float kBarLeft = -117.0f;
    // ARE YOU SURE? at 90 above the centre, YES and NO at 50 above it,
    // their text starting 30 left and 42 right of the centre.
    static constexpr float kConfirmTitleY = -90.0f;
    static constexpr float kConfirmChoiceY = -50.0f;
    static constexpr float kYesX = -30.0f;
    static constexpr float kNoX = 42.0f;
};

} // namespace openhogs::ui
