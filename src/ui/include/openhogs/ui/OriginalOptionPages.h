// OriginalOptionPages.h
//
// Two pages of the original game's OPTIONS (docs/formats/front-end.md):
//
//   - CREDITS (screen type 16): the original's credits, a name and a job on
//     each row, rolling up the screen over the background at the
//     executable's pace (100 text units a second, FUN_00423e70 case 16);
//   - CONTROLS (screen type 15): a board (`pckey000`-`pckey006`) that turns
//     to face the player, with six rows of two windows: what an action is
//     called and its key (FUN_0042dea0). Longer lists scroll.
//
// Positions are pixels of the 640 x 480 screen. Nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::ui {

class CreditsRoll {
public:
    struct Row {
        std::size_t index = 0;  // which credits row
        MenuTextLine name;      // the left column
        MenuTextLine job;       // the right column
    };

    void open() { seconds_ = 0.0; }
    void advance(double seconds) { seconds_ += seconds; }

    // The rows on screen now, out of `rowCount`. The roll starts below the
    // screen and begins again once the last row has gone off the top.
    std::vector<Row> visibleRows(std::size_t rowCount) const;

private:
    double seconds_ = 0.0;
};

inline constexpr std::size_t kControlsBoardRows = 6;

class OriginalControlsBoard {
public:
    void open();
    void tick();
    void advance(double seconds);

    std::vector<MenuPiece> pieces() const;

    // The board faces the player: the words can be written.
    bool showsText() const { return frame_ == kFacingFrame; }
    MenuTextLine titleLine() const;
    MenuTextLine actionLine(std::size_t row) const;
    MenuTextLine keyLine(std::size_t row) const;

    int frame() const { return frame_; }
    static constexpr int kFacingFrame = 6;

private:
    int frame_ = kFacingFrame;  // pckey000 .. pckey006
    double pending_ = 0.0;
};

} // namespace openhogs::ui
