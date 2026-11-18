// WeaponMenu.h
//
// The original's weapon menu (Language/Tims/MENUTIMS.MAD `menu.bmp`): a
// brass panel that slides in from the right with four tabs along its top
// and a grid of 3 x 4 windows, one weapon icon in each, with the number
// of uses left. The executable draws the icons, their counts and a
// selection frame (`SELECT.BMP`) on it (FUN_00459550); this class is what
// the menu holds and how the keys move through it, with no drawing code.
//
// The tabs sort the weapons as the panel's tab pictures suggest: guns,
// things done by hand, explosives and launchers, and what comes from the
// sky. Which weapon goes on which tab in the original is not decoded
// (docs/open-decisions.md).

#pragma once

#include "openhogs/game/Weapons.h"
#include "openhogs/ui/Menu.h"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace openhogs::ui {

inline constexpr std::size_t kWeaponMenuTabs = 4;
inline constexpr std::size_t kWeaponMenuColumns = 3;
inline constexpr std::size_t kWeaponMenuRows = 4;
inline constexpr std::size_t kWeaponMenuSlots = kWeaponMenuColumns * kWeaponMenuRows;

// The tab a weapon is shown on: 0 guns, 1 by hand, 2 explosives and
// launchers, 3 from the sky.
std::size_t weaponMenuTab(game::WeaponId weapon);

// One weapon the hog can choose: its place in Hog::weapons and how many
// uses it has left (empty: it never runs out).
struct WeaponMenuEntry {
    std::size_t weaponIndex = 0;
    game::WeaponId weapon = game::WeaponId::Trotter;
    std::optional<int> uses;
    bool usable() const { return !uses || *uses > 0; }
};

class WeaponMenu {
public:
    // Opens the menu on `entries` (in the hog's order), with the cursor on
    // the weapon at `selectedIndex` of Hog::weapons.
    void open(const std::vector<WeaponMenuEntry>& entries, std::size_t selectedIndex);
    void close() { open_ = false; }
    bool isOpen() const { return open_; }

    // Moves the cursor (up and down within the column, left and right
    // across the columns and on to the next tab at the edges), picks
    // (Confirm: the chosen weapon's index in Hog::weapons, if it can be
    // used) or closes (Back).
    struct Result {
        std::optional<std::size_t> chosen;
        bool closed = false;
        bool moved = false;
    };
    Result handle(MenuInput input);

    std::size_t tab() const { return tab_; }
    std::size_t slot() const { return slot_; }  // row * columns + column
    // What the open tab shows in each window (empty windows have none).
    const std::array<std::optional<WeaponMenuEntry>, kWeaponMenuSlots>& slots() const { return pages_[tab_]; }
    // The weapon under the cursor, if any.
    const std::optional<WeaponMenuEntry>& current() const { return pages_[tab_][slot_]; }

private:
    std::array<std::array<std::optional<WeaponMenuEntry>, kWeaponMenuSlots>, kWeaponMenuTabs> pages_{};
    bool open_ = false;
    std::size_t tab_ = 0;
    std::size_t slot_ = 0;
};

} // namespace openhogs::ui
