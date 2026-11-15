// OriginalCareer.h
//
// The original game's career and promotion board (screen types 0 and 13
// of the PC executable's front end: CAREER PATH; see
// docs/formats/front-end.md), as it draws and animates it:
//
//   - an information board with a dark monitor (`alpha02`-`alpha08`,
//     unfolding) that slides in from the left and drops from the top;
//   - a bar hanging above it (`propoint`, its middle repeated) that holds
//     the title;
//   - for CAREER PATH, the four careers' icons in a row below the board
//     (`pcHweap`, `sappr`, `snipr`, `pcmedic`), the chosen one blinking.
//
// The executable also stands the hog on a pedestal (`standpc`) in the
// monitor, a 3D model OpenHogs does not draw there (decision 107); the
// monitor shows the hog, the points and the choices in words instead.
//
// Positions are pixels of the 640 x 480 screen; nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::ui {

inline constexpr std::size_t kCareerIcons = 4;

class OriginalCareerBoard {
public:
    void open(std::size_t selected);
    void select(std::size_t selected) { selected_ = selected; }

    void tick();
    void advance(double seconds);

    // The pictures, top one first. `careerIcons`: show the row of career
    // icons (CAREER PATH), the chosen one blinking.
    std::vector<MenuPiece> pieces(bool careerIcons) const;

    bool showsText() const;
    MenuTextLine titleLine() const;
    // The monitor's lines, from the top.
    MenuTextLine monitorLine(std::size_t line) const;

    bool settled() const;

private:
    float boardX() const;
    float boardY() const;
    float barY() const;

    int boardX_ = 0, boardXVelocity_ = 0;  // DAT_00512CE4, 1024-wide units
    int boardY_ = 0, boardYVelocity_ = 0;  // DAT_00512964, 820-high units
    int barY_ = 0, barVelocity_ = 0;       // DAT_0051296C
    int frame_ = 6;                        // alpha02 .. alpha08
    int blink_ = 0;                        // DAT_00512E80: hidden while > 0
    std::size_t selected_ = 0;
    double pending_ = 0.0;
};

} // namespace openhogs::ui
