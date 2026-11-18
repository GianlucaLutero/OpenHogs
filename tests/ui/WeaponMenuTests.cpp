// Tests for the weapon menu (ui::WeaponMenu): which weapon lands where,
// how the cursor moves and what can be picked.
//
// The menu is the way to choose among a rank's many weapons; a weapon on
// no tab, a cursor that gets stuck or a weapon picked with no uses left
// would leave a player unable to use what the hog carries.

#include "openhogs/ui/WeaponMenu.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using openhogs::game::WeaponId;

namespace {

std::vector<WeaponMenuEntry> entriesOf(const std::vector<WeaponId>& weapons) {
    std::vector<WeaponMenuEntry> entries;
    for (std::size_t i = 0; i < weapons.size(); ++i) entries.push_back({i, weapons[i], std::nullopt});
    return entries;
}

} // namespace

TEST_CASE("Weapons go on the tab their picture shows", "[ui][weaponmenu]") {
    // The panel's tabs show a pistol, two hands, a grenade and a parachute.
    CHECK(weaponMenuTab(WeaponId::Rifle) == 0);
    CHECK(weaponMenuTab(WeaponId::Trotter) == 1);
    CHECK(weaponMenuTab(WeaponId::HealingHands) == 1);
    CHECK(weaponMenuTab(WeaponId::Grenade) == 2);
    CHECK(weaponMenuTab(WeaponId::Tnt) == 2);
    CHECK(weaponMenuTab(WeaponId::Jetpack) == 3);
}

TEST_CASE("The menu opens on the weapon in hand and walks across the tabs", "[ui][weaponmenu]") {
    // Opening shows the weapon the hog holds; moving right past the last
    // column goes on to the next tab, so every weapon can be reached.
    WeaponMenu menu;
    menu.open(entriesOf({WeaponId::Trotter, WeaponId::Rifle, WeaponId::Grenade}), 1);
    REQUIRE(menu.isOpen());
    CHECK(menu.tab() == 0);
    REQUIRE(menu.current());
    CHECK(menu.current()->weapon == WeaponId::Rifle);

    menu.handle(MenuInput::Right);
    menu.handle(MenuInput::Right);
    menu.handle(MenuInput::Right);  // past the third column
    CHECK(menu.tab() == 1);
    CHECK(menu.current()->weapon == WeaponId::Trotter);
    menu.handle(MenuInput::Left);   // back past the first column
    CHECK(menu.tab() == 0);

    menu.handle(MenuInput::Up);     // the column wraps round
    CHECK(menu.slot() == (kWeaponMenuRows - 1) * kWeaponMenuColumns + kWeaponMenuColumns - 1);
}

TEST_CASE("Picking gives the weapon's place in the hog's list; used up weapons cannot be picked", "[ui][weaponmenu]") {
    WeaponMenu menu;
    auto entries = entriesOf({WeaponId::Trotter, WeaponId::Grenade, WeaponId::Tnt});
    entries[2].uses = 0;
    menu.open(entries, 1);

    menu.handle(MenuInput::Right);  // the TNT, none left
    CHECK_FALSE(menu.handle(MenuInput::Confirm).chosen);
    CHECK(menu.isOpen());

    menu.handle(MenuInput::Left);
    const auto picked = menu.handle(MenuInput::Confirm);
    REQUIRE(picked.chosen);
    CHECK(*picked.chosen == 1);
    CHECK_FALSE(menu.isOpen());
}
