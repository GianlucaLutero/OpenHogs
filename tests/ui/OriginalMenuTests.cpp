// Tests for ui::OriginalMenu: the original game's list menu (the brass
// panel of the main menu) and its animation.
//
// The positions and timings come from the original executable; these tests
// protect what the player sees: the panel arriving and coming to rest in
// its place, the dial pointing at the chosen entry, the blinking lamp, and
// the words sitting inside their bars.

#include "openhogs/ui/OriginalMenu.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <string>

using namespace openhogs::ui;
using Catch::Approx;

namespace {

// Ticks until the menu stops moving (or gives up).
int ticksToSettle(OriginalMenu& menu, int limit = 500) {
    int ticks = 0;
    while (!menu.settled() && ticks < limit) {
        menu.tick();
        ++ticks;
    }
    return ticks;
}

// The first piece showing `picture`.
MenuPiece pieceNamed(const OriginalMenu& menu, const std::string& picture) {
    const auto pieces = menu.pieces();
    const auto found = std::find_if(pieces.begin(), pieces.end(),
                                    [&](const MenuPiece& piece) { return piece.picture == picture; });
    REQUIRE(found != pieces.end());
    return *found;
}

} // namespace

TEST_CASE("The front end's spring always reaches its target and stays there", "[ui][originalmenu]") {
    // Every slide of the menus uses this spring; if it could stop short or
    // keep wobbling, a panel would never come to rest in its place.
    for (const int start : {-650, -36, -1, 1, 300}) {
        int value = start;
        int velocity = 0;
        int ticks = 0;
        while ((value != 0 || velocity != 0) && ticks < 200) {
            const int before = value;
            value = frontEndSpring(value, 0, velocity, 8, 17, 80);
            // It always moves while it is away from the target.
            if (before != 0) CHECK(value != before);
            ++ticks;
        }
        CHECK(value == 0);
        CHECK(velocity == 0);
        CHECK(frontEndSpring(value, 0, velocity, 8, 17, 80) == 0);
    }
}

TEST_CASE("The opening menu drops in from above and comes to rest in the original's place", "[ui][originalmenu]") {
    // The original's main menu arrives with the racks sliding in and the
    // panel falling from the top; at rest the panel must be exactly where
    // the executable draws it, or the bars, lamps and words would not line up.
    OriginalMenu menu;
    menu.open(0);
    CHECK(pieceNamed(menu, "fullmenu").y < -300.0f);
    CHECK(menu.rackOffset() < 0);
    CHECK_FALSE(menu.showsText());

    const int ticks = ticksToSettle(menu);
    CHECK(ticks < 100);  // a few seconds at most
    CHECK(menu.settled());
    const auto panels = menu.pieces();
    const auto panel = std::find_if(panels.rbegin(), panels.rend(),
                                    [](const MenuPiece& piece) { return piece.picture == "fullmenu"; });
    CHECK(panel->x == Approx(50.0f));
    CHECK(panel->y == Approx(0.0f));
    CHECK(pieceNamed(menu, "title1").x == Approx(286.0f));
    CHECK(menu.showsText());
}

TEST_CASE("The dial turns one picture per tick to point at the chosen entry", "[ui][originalmenu]") {
    // The green arrow is how the original shows the selection; it must end
    // on the picture that points at the entry (0, 4, 7 or 11 of the twelve).
    OriginalMenu menu;
    menu.open(0);
    ticksToSettle(menu);
    CHECK(pieceNamed(menu, "dial0001").x == Approx(130.0f));

    menu.select(2);
    menu.tick();
    CHECK(menu.dialFrame() == 1);
    ticksToSettle(menu);
    CHECK(menu.dialFrame() == 7);
    menu.select(3);
    ticksToSettle(menu);
    CHECK(menu.dialFrame() == 11);
    menu.select(0);
    ticksToSettle(menu);
    CHECK(menu.dialFrame() == 0);
}

TEST_CASE("Only the chosen entry's lamp blinks", "[ui][originalmenu]") {
    // The lamp next to the chosen bar blinks (lit for ten ticks, off for
    // five); the others go out, so the player sees at once which is chosen.
    OriginalMenu menu;
    menu.open(1);
    int litTicks = 0;
    int offTicks = 0;
    for (int tick = 0; tick < 34; ++tick) {  // two whole blinks
        menu.tick();
        if (menu.lampFrame(1) == 2) ++litTicks;
        if (menu.lampFrame(1) == 0) ++offTicks;
        CHECK(menu.lampFrame(0) == 0);
        CHECK(menu.lampFrame(3) == 0);
    }
    CHECK(litTicks == 20);
    CHECK(offTicks == 10);
}

TEST_CASE("The words of the menu sit inside the title plate and the bars", "[ui][originalmenu]") {
    // The words and the brass come from different tables of the executable
    // (text boxes in 1024 x 820 units, sprites in pixels); this checks the
    // conversion puts each line inside its picture.
    OriginalMenu menu;
    menu.open(0);
    ticksToSettle(menu);
    constexpr float kLetterHeight = 18.0f;  // chars3 at the original's size

    const auto plate = pieceNamed(menu, "title1");
    const auto title = menu.titleLine();
    CHECK(title.top > plate.y);
    CHECK(title.top + kLetterHeight < plate.y + 64.0f);
    CHECK(std::abs(title.centreX - (plate.x + 104.0f)) < 6.0f);

    const auto pieces = menu.pieces();
    std::vector<MenuPiece> bars;
    for (const auto& piece : pieces) {
        if (piece.picture == "chose1") bars.push_back(piece);
    }
    REQUIRE(bars.size() == kOriginalMenuEntries);
    for (std::size_t i = 0; i < bars.size(); ++i) {
        const auto line = menu.entryLine(i);
        CHECK(line.top > bars[i].y);
        CHECK(line.top + kLetterHeight < bars[i].y + 35.0f);
        CHECK(std::abs(line.centreX - (bars[i].x + 80.0f)) < 12.0f);
    }
}
