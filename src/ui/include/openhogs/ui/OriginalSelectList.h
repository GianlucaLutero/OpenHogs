// OriginalSelectList.h
//
// The original game's longer list (screen type 2 of the PC executable's
// front end: SELECT TEAM and the like; see docs/formats/front-end.md), as
// the executable draws and animates it. It is built from the same
// "settings machine" as the yes/no question:
//
//   - a monitor (`counsele`) whose dark screen shows the entries, standing
//     on a long stem, with a cog strip (`selcog1`-`6`) beside it;
//   - a selector that slides down the monitor to the chosen entry: a bar
//     (`select`), a red knob that turns while it moves (`selec00`-`05`)
//     and a lamp (`lit1`-`lit3`);
//   - the name plate at the top that holds the title, and on the right the
//     closed box with its tray and pipes (ui::addNamePlate, addTray,
//     addBox).
//
// When it opens, the monitor rises from the bottom of the screen, the plate
// drops from the top and the box slides in from the right, each with the
// executable's spring.
//
// Positions are pixels of the original 640 x 480 screen; the animation
// counts in front-end ticks. Nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::ui {

// The monitor has room for this many entries (the executable's text boxes
// 5 to 12 of type 2).
inline constexpr std::size_t kOriginalSelectListEntries = 8;

class OriginalSelectList {
public:
    // Starts the opening animation with `selected` chosen.
    void open(std::size_t selected);

    // The player moved to another entry: the selector slides there.
    void select(std::size_t entry);

    void tick();
    void advance(double seconds);

    // The pictures, top one first.
    std::vector<MenuPiece> pieces() const;

    // The words are written once the monitor is in place.
    bool showsText() const;
    MenuTextLine titleLine() const;
    MenuTextLine entryLine(std::size_t entry) const;

    // State, for tests.
    int listY() const { return listY_; }
    int selector() const { return selector_; }
    bool settled() const;

    // The sounds of the ticks since the last call (the cog while the
    // monitor rises, the steam), and the steam it blows out of its pipe.
    std::vector<MachineSound> takeSounds();
    const SteamPuffs& steam() const { return steam_; }

private:
    float listOffsetY() const;   // the monitor's distance from its place, pixels
    float plateOffsetY() const;
    float boxOffsetX() const;

    int listY_ = 0;  // 820-high units below its place (DAT_00512950)
    int listVelocity_ = 0;
    int plateY_ = 0;  // DAT_00512954
    int plateVelocity_ = 0;
    int boxX_ = 0;  // 1024-wide units right of its place (DAT_00512cdc)
    int boxVelocity_ = 0;
    int selector_ = 0;  // element 0: five steps per entry
    int selectorTarget_ = 0;
    double pending_ = 0.0;
    int rattleTicks_ = 0;
    SteamPuffs steam_;
    std::vector<MachineSound> sounds_;
};

} // namespace openhogs::ui
