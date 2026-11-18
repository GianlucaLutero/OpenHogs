// WeaponMenu.cpp - see WeaponMenu.h for an overview.

#include "openhogs/ui/WeaponMenu.h"

namespace openhogs::ui {

std::size_t weaponMenuTab(game::WeaponId weapon) {
    using game::WeaponKind;
    switch (game::weaponSpec(weapon).kind) {
    case WeaponKind::Bullet: return 0;  // the pistol tab
    case WeaponKind::Thrown:
    case WeaponKind::Shell:
    case WeaponKind::Placed: return 2;  // the grenade tab
    case WeaponKind::Jetpack:
    case WeaponKind::SpecialOps:
    case WeaponKind::Airstrike: return 3;  // the parachute tab
    case WeaponKind::Melee:
    case WeaponKind::Heal:
    case WeaponKind::Suicide:
    case WeaponKind::SkipTurn:
    case WeaponKind::Surrender:
    case WeaponKind::Shockwave:
    case WeaponKind::Hide:
    case WeaponKind::PickPocket:
    case WeaponKind::InOut:
    case WeaponKind::MapView:
    case WeaponKind::Binoculars: break;
    }
    return 1;  // the hands tab
}

void WeaponMenu::open(const std::vector<WeaponMenuEntry>& entries, std::size_t selectedIndex) {
    pages_ = {};
    std::array<std::size_t, kWeaponMenuTabs> filled{};
    tab_ = 0;
    slot_ = 0;
    for (const auto& entry : entries) {
        const std::size_t tab = weaponMenuTab(entry.weapon);
        if (filled[tab] >= kWeaponMenuSlots) continue;  // a full tab: the rest stays off the menu
        const std::size_t slot = filled[tab]++;
        pages_[tab][slot] = entry;
        if (entry.weaponIndex == selectedIndex) {
            tab_ = tab;
            slot_ = slot;
        }
    }
    open_ = true;
}

WeaponMenu::Result WeaponMenu::handle(MenuInput input) {
    Result result;
    if (!open_) return result;
    const std::size_t column = slot_ % kWeaponMenuColumns;
    const std::size_t row = slot_ / kWeaponMenuColumns;
    switch (input) {
    case MenuInput::Up:
        slot_ = ((row + kWeaponMenuRows - 1) % kWeaponMenuRows) * kWeaponMenuColumns + column;
        result.moved = true;
        break;
    case MenuInput::Down:
        slot_ = ((row + 1) % kWeaponMenuRows) * kWeaponMenuColumns + column;
        result.moved = true;
        break;
    case MenuInput::Left:
        if (column == 0) {
            // Past the left edge: the previous tab, its last column.
            tab_ = (tab_ + kWeaponMenuTabs - 1) % kWeaponMenuTabs;
            slot_ = row * kWeaponMenuColumns + kWeaponMenuColumns - 1;
        } else {
            --slot_;
        }
        result.moved = true;
        break;
    case MenuInput::Right:
        if (column == kWeaponMenuColumns - 1) {
            tab_ = (tab_ + 1) % kWeaponMenuTabs;
            slot_ = row * kWeaponMenuColumns;
        } else {
            ++slot_;
        }
        result.moved = true;
        break;
    case MenuInput::Confirm:
        if (current() && current()->usable()) {
            result.chosen = current()->weaponIndex;
            open_ = false;
        }
        break;
    case MenuInput::Back:
        open_ = false;
        result.closed = true;
        break;
    }
    return result;
}

} // namespace openhogs::ui
