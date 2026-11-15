// OriginalDialog.h
//
// The original game's yes/no question (ARE YOU SURE?, REALLY QUIT APP?,
// PLAY TRAINING MISSION?), as the PC executable draws and animates it:
// screen type 12 of docs/formats/front-end.md.
//
// The question hangs on a name plate (`name0`) at the top of the screen;
// below, on the right, a brass box (`yesno01`-`yesno06`, then `yes`) opens
// up and shows two windows, YES and NO, with a dial (`yesdial1`-`yesdial7`)
// whose arrow turns to the chosen one. A closed tray (`info1`) and pipes
// (`mainbar0`, `mainbar1`) complete the machine.
//
// When the dialog opens, the plate drops from the top and the box slides
// in from the right with the original's springs; then the box unfolds.
//
// Positions are the executable's, in pixels of its 640 x 480 screen; the
// animation counts in front-end ticks (ui::kFrontEndTicksPerSecond).
// Nothing here draws: the game application draws the pieces.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::ui {

// Parts of the "settings machine" that the yes/no question shares with the
// longer list (screen type 2, ui::OriginalSelectList); each appends its
// pieces to `pieces`, top one first.
//  - the name plate that holds the title, its arms, `plateY` pixels from
//    its place;
//  - the closed tray (`info1`), with the box `box` pixels from its place;
//  - the box (`yesno01`-`06` while it opens, `yes` at `kOpenBoxFrame`)
//    and its pipes.
void addNamePlate(std::vector<MenuPiece>& pieces, float plateY);
void addTray(std::vector<MenuPiece>& pieces, float box);
void addBox(std::vector<MenuPiece>& pieces, float box, int boxFrame);

class OriginalDialog {
public:
    // Starts the opening animation with `selected` (0 = YES, 1 = NO).
    void open(std::size_t selected);

    // The player chose the other answer: the dial turns.
    void select(std::size_t answer);

    // One tick of the animation, and `seconds` in whole ticks.
    void tick();
    void advance(double seconds);

    // The pictures, from the top one down (the first covers the others).
    std::vector<MenuPiece> pieces() const;

    // The question, on the plate; the answers, in the box's windows once it
    // is open (answer 0 = YES, 1 = NO).
    MenuTextLine titleLine() const;
    bool showsAnswers() const { return boxFrame_ == kOpenBoxFrame; }
    MenuTextLine answerLine(std::size_t answer) const;

    bool settled() const;
    int boxFrame() const { return boxFrame_; }
    int dialFrame() const { return dialFrame_; }

    static constexpr int kOpenBoxFrame = 6;  // yesno01..yesno06, then `yes`

private:
    float boxOffsetX() const;  // pixels the box is still right of its place
    float plateOffsetY() const;

    int plateY_ = 0;           // the plate's drop (DAT_00512954)
    int plateVelocity_ = 0;
    int boxX_ = 0;             // the box's slide, 1024-wide units (DAT_00512cdc)
    int boxVelocity_ = 0;
    int boxFrame_ = 0;         // 0..6
    int dialFrame_ = 0;        // 0..6: yesdial1..yesdial7
    int dialTarget_ = 0;
    double pending_ = 0.0;
};

} // namespace openhogs::ui
