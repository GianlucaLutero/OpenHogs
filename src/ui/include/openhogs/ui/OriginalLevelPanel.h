// OriginalLevelPanel.h
//
// The original game's level panel (screen type 20, MULTI-PLAYER SELECT
// LEVEL, of the PC executable's front end; see docs/formats/front-end.md),
// as the executable draws and animates it:
//
//   - a big brass panel (`lselpc`) with six slots on the left, each with a
//     lamp at both ends (`lit1`-`lit3`) and an arrow towards a dark monitor
//     on the right; the chosen slot's arrow is lit (`lselpc2`);
//   - a name plate above (`sqname01`) that holds the title.
//
// The original lists level types in the slots (SURVIVAL-NOVICE...) and the
// last slot is LEVEL SETUP; FIELD CONDITIONS (screen type 21, which draws
// nothing of its own) is written on the monitor of this panel. OpenHogs
// lists the maps themselves in the first five slots (scrolling) and BACK
// in the last one, and writes FIELD CONDITIONS on the monitor.
//
// The panel rises from the bottom and the plate drops from the top, with
// the executable's springs. Positions are pixels of the 640 x 480 screen.
// Nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <array>
#include <cstddef>
#include <vector>

namespace openhogs::ui {

inline constexpr std::size_t kLevelPanelSlots = 6;      // five, then the last one apart
inline constexpr std::size_t kLevelPanelMonitorRows = 10;  // text boxes 114-123

// Which menu item goes in which slot: the last item (BACK) always in the
// last slot, the others five at a time in the first five slots, scrolled
// so the chosen one shows. -1: an empty slot.
struct LevelSlotLayout {
    std::array<int, kLevelPanelSlots> items{};
    std::size_t selectedSlot = 0;
};
LevelSlotLayout levelSlotLayout(std::size_t itemCount, std::size_t selected);

class OriginalLevelPanel {
public:
    // Starts the opening animation with `slot` chosen.
    void open(std::size_t slot);

    // The chosen slot: its lamps light and its arrow.
    void select(std::size_t slot);

    void tick();
    void advance(double seconds);

    // The pictures, top one first.
    std::vector<MenuPiece> pieces() const;

    // The words are written once the panel is in place.
    bool showsText() const;
    MenuTextLine titleLine() const;
    MenuTextLine slotLine(std::size_t slot) const;
    MenuTextLine monitorLine(std::size_t row) const;

    // State, for tests.
    int panelY() const { return panelY_; }
    bool settled() const;

private:
    float panelOffsetY() const;
    float plateOffsetY() const;

    int panelY_ = 0;  // 820-high units below its place (DAT_00512950)
    int panelVelocity_ = 0;
    int plateY_ = 0;  // DAT_00512954
    int plateVelocity_ = 0;
    std::size_t slot_ = 0;
    double pending_ = 0.0;
};

// The four team holders of the MULTI-PLAYER screen (screen 16, drawn with
// the longer list, ui::OriginalSelectList): on the left, one per team, a
// brass holder (`com_man`) showing who plays it, a joypad (`pcpad`) for a
// person or a chip (`pcchip`) for the computer. A holder slides in from the
// left when its team takes part and out when it is switched off.
class OriginalTeamHolders {
public:
    enum class Player { Person, Computer, Off };
    static constexpr std::size_t kHolders = 4;

    // Starts with every holder out of sight.
    void open();
    // Who plays each team (missing teams are off).
    void setPlayers(const std::vector<Player>& players);

    void tick();
    void advance(double seconds);

    std::vector<MenuPiece> pieces() const;

    int offset(std::size_t holder) const { return offsets_.at(holder); }
    bool settled() const;

private:
    std::vector<Player> players_ = std::vector<Player>(kHolders, Player::Off);
    std::vector<int> offsets_ = std::vector<int>(kHolders, 0);
    std::vector<int> velocities_ = std::vector<int>(kHolders, 0);
    std::vector<Player> shown_ = std::vector<Player>(kHolders, Player::Off);  // the picture while it leaves
    double pending_ = 0.0;
};

} // namespace openhogs::ui
