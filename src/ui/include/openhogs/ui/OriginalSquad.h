// OriginalSquad.h
//
// The original game's campaign squad page (screen type 5 of the PC
// executable's front end) and a hog's page drawn over it (type 6); see
// docs/formats/front-end.md:
//
//   - two boards of five portrait windows (`sqpic`, the right one
//     mirrored; `sqpics00`-`sqpics10` while they turn in), with each hog's
//     face (`face1a`...) in a window and its name on the blue bar beside;
//     the left board holds hogs 1-5, the right one hogs 6-8;
//   - a selector on each board (`sqdial`, a frame with a red knob, and two
//     lamps `lit1`-`lit3`) that slides to the chosen hog; only the board of
//     the chosen hog lights its lamps;
//   - the page's other choices (START MISSION, BACK) on small panels
//     (`sqoptsF`) at the bottom right, each with two lamps;
//   - a monitor at the bottom (`pigpro`) and the title plate (`sqname01`).
//
// A hog's page (PROMOTE, SWAP POSITION...) is a panel (`swap.mgl1`) that
// rises over the squad, with an arrow (`swap01`/`swap02`) at the chosen
// line.
//
// Positions are pixels of the 640 x 480 screen; nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::ui {

inline constexpr std::size_t kSquadHogs = 8;  // five on the left board, three on the right
inline constexpr std::size_t kSquadOptions = 2;  // START MISSION, BACK

class OriginalSquad {
public:
    // Starts the opening animation; `selected` counts the hogs first, then
    // the options (8 = START MISSION, 9 = BACK).
    void open(std::size_t selected);
    void select(std::size_t selected);

    // A hog's page rises over the squad, or goes away.
    void openHogPage(std::size_t line);
    void selectHogLine(std::size_t line);
    void closeHogPage();

    void tick();
    void advance(double seconds);

    // The pictures, top one first. `hogs` is how many hogs to show (up to 8).
    std::vector<MenuPiece> pieces(std::size_t hogs) const;

    // The boards face the player: faces and words can be shown.
    bool showsText() const { return boardFrame_ == kBoardOpenFrame; }
    MenuTextLine titleLine() const;
    MenuTextLine nameLine(std::size_t hog) const;
    MenuTextLine optionLine(std::size_t option) const;
    // The monitor's lines (the chosen hog's rank, the mission...).
    MenuTextLine monitorLine(std::size_t line) const;
    bool showsHogPage() const;
    MenuTextLine hogPageLine(std::size_t line) const;

    int boardFrame() const { return boardFrame_; }
    bool settled() const;
    static constexpr int kBoardOpenFrame = 6;

private:
    struct Selector {
        int position = 0;  // five steps a row (elements 4 and 5)
        int target = 0;
        int lamp = 0;      // 0..2: lit1..lit3
    };

    int boardFrame_ = kBoardOpenFrame;
    int plateY_ = 0, plateVelocity_ = 0;
    int optionsY_ = 0, optionsVelocity_ = 0;  // DAT_0051295c
    int monitorY_ = 0, monitorVelocity_ = 0;  // DAT_00512968
    int hogPageY_ = 500, hogPageVelocity_ = 0;  // DAT_00512960
    bool hogPage_ = false;
    std::size_t hogLine_ = 0;
    std::size_t selected_ = 0;
    Selector left_, right_;
    std::vector<int> optionLamps_ = std::vector<int>(kSquadOptions, 0);
    double pending_ = 0.0;
};

} // namespace openhogs::ui
