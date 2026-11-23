// OriginalSaveBoard.h
//
// The original game's loading and saving board (screen types 8 and 9 of
// the PC executable's front end: LOAD, SAVE ARMY; see
// docs/formats/front-end.md), as it draws and animates it:
//
//   - the board `pcsav` with eight slots, each with a green lamp
//     (`pclit0`-`pclit2`, off to lit), rising from below the screen;
//   - beside it, an information panel that swings open (`pcsvinf0`-`4`)
//     and shows, for the chosen slot, the rank badges of the squad saved
//     there and the number of its next mission.
//
// The chosen slot's lamp blinks; the others go dark. The slots' names
// (the teams saved there, or "---") are written on the board.
//
// Positions are pixels of the 640 x 480 screen; nothing here draws.

#pragma once

#include "openhogs/game/Ranks.h"
#include "openhogs/ui/OriginalMenu.h"

#include <array>
#include <cstddef>
#include <vector>

namespace openhogs::ui {

inline constexpr std::size_t kSaveBoardSlots = 8;

class OriginalSaveBoard {
public:
    void open(std::size_t selected);
    void select(std::size_t selected);

    void tick();
    void advance(double seconds);

    // The pictures, top one first. `squad`: the ranks of the hogs saved in
    // the chosen slot (empty for an empty slot), shown once the panel is open.
    std::vector<MenuPiece> pieces(const std::vector<game::RankId>& squad) const;

    // Whether the board has arrived, so the slots' names can be written.
    bool showsText() const;
    // Whether the panel is open, so its contents can be shown.
    bool showsInfo() const;
    // Where a slot's name goes.
    MenuTextLine slotLine(std::size_t slot) const;
    // Where the panel's MISSION line starts (written from x, not centred).
    MenuTextLine missionLine() const;

    bool settled() const;

private:
    int drop_ = 0;          // DAT_00512950, 820-high units
    int dropVelocity_ = 0;  // DAT_00512C1C
    int panelFrame_ = 0;    // pcsvinf0 .. pcsvinf4
    std::array<int, kSaveBoardSlots> lamps_{};      // 0 (off) .. 2 (lit)
    std::array<int, kSaveBoardSlots> lampTicks_{};  // the chosen lamp's blinking
    std::size_t selected_ = 0;
    double pending_ = 0.0;
};

} // namespace openhogs::ui
