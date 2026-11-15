// Tests for ui::OriginalMenu: the original game's list menu (the brass
// panel of the main menu) and its animation.
//
// The positions and timings come from the original executable; these tests
// protect what the player sees: the panel arriving and coming to rest in
// its place, the dial pointing at the chosen entry, the blinking lamp, and
// the words sitting inside their bars.

#include "openhogs/ui/OriginalDialog.h"
#include "openhogs/ui/OriginalMenu.h"
#include "openhogs/ui/OriginalSelectList.h"
#include "openhogs/ui/OriginalVolume.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
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

// --- The yes/no question (ui::OriginalDialog) --------------------------------

namespace {

MenuPiece dialogPiece(const OriginalDialog& dialog, const std::string& picture) {
    const auto pieces = dialog.pieces();
    const auto found = std::find_if(pieces.begin(), pieces.end(),
                                    [&](const MenuPiece& piece) { return piece.picture == picture; });
    REQUIRE(found != pieces.end());
    return *found;
}

int ticksToSettle(OriginalDialog& dialog, int limit = 500) {
    int ticks = 0;
    while (!dialog.settled() && ticks < limit) {
        dialog.tick();
        ++ticks;
    }
    return ticks;
}

} // namespace

TEST_CASE("The question slides in, unfolds and rests where the original draws it", "[ui][originaldialog]") {
    // The plate drops from the top and the box comes in from the right,
    // then opens; at rest they must be at the executable's positions or the
    // answers would not sit in the windows.
    OriginalDialog dialog;
    dialog.open(1);
    CHECK(dialogPiece(dialog, "name0").y < 0.0f);
    CHECK(dialogPiece(dialog, "yesno01").x > 640.0f);
    CHECK_FALSE(dialog.showsAnswers());

    CHECK(ticksToSettle(dialog) < 100);
    CHECK(dialogPiece(dialog, "name0").x == Approx(123.0f));
    CHECK(dialogPiece(dialog, "name0").y == Approx(16.0f));
    const auto box = dialogPiece(dialog, "yes");
    CHECK(box.x == Approx(319.0f));
    CHECK(box.y == Approx(345.0f));
    CHECK(dialog.showsAnswers());
}

TEST_CASE("The question's dial points at the chosen answer", "[ui][originaldialog]") {
    // yesdial1 points at YES and yesdial7 at NO; the arrow turns one picture
    // per tick between them.
    OriginalDialog dialog;
    dialog.open(1);
    ticksToSettle(dialog);
    CHECK(dialog.dialFrame() == 6);
    CHECK_NOTHROW(dialogPiece(dialog, "yesdial7"));
    dialog.select(0);
    dialog.tick();
    CHECK(dialog.dialFrame() == 5);
    ticksToSettle(dialog);
    CHECK_NOTHROW(dialogPiece(dialog, "yesdial1"));
}

TEST_CASE("The answers are written inside the box's windows", "[ui][originaldialog]") {
    // The windows of `yes` run from column 53 to 115 and 123 to 185, rows
    // 36 to 58: YES and NO must be centred in them.
    OriginalDialog dialog;
    dialog.open(0);
    ticksToSettle(dialog);
    const auto box = dialogPiece(dialog, "yes");
    const auto yes = dialog.answerLine(0);
    const auto no = dialog.answerLine(1);
    CHECK(yes.centreX == Approx(box.x + 84.0f));
    CHECK(no.centreX == Approx(box.x + 154.0f));
    CHECK(yes.top > box.y + 36.0f);
    CHECK(yes.top + 18.0f < box.y + 58.0f + 2.0f);
}

// --- VOLUME CONTROL (ui::OriginalVolumePanel) ---------------------------------

namespace {

MenuPiece volumePiece(const OriginalVolumePanel& panel, const std::string& prefix, std::size_t index = 0) {
    std::vector<MenuPiece> found;
    for (const auto& piece : panel.pieces(10, 0)) {
        if (piece.picture.rfind(prefix, 0) == 0) found.push_back(piece);
    }
    REQUIRE(index < found.size());
    return found[index];
}

void settle(OriginalVolumePanel& panel) {
    for (int tick = 0; tick < 500 && !panel.settled(); ++tick) panel.tick();
    REQUIRE(panel.settled());
}

} // namespace

TEST_CASE("The volume panel comes up, unfolds and only then shows its knobs", "[ui][originalvolume]") {
    // Like the executable, the knobs and the words appear on the open panel
    // (vol00) only; on the unfolding pictures they would float in the air.
    OriginalVolumePanel panel;
    panel.open(1);
    CHECK(volumePiece(panel, "vol0").y > 480.0f);  // starts below the screen
    CHECK(volumePiece(panel, "vol0").picture == "vol01");
    CHECK_FALSE(panel.showsText());
    for (const auto& piece : panel.pieces(10, 0)) CHECK(piece.picture.rfind("vollit", 0) != 0);

    settle(panel);
    const auto open = volumePiece(panel, "vol0");
    CHECK(open.picture == "vol00");
    CHECK(open.x == Approx(100.0f));
    CHECK(open.y == Approx(240.0f));
    CHECK(panel.showsText());
}

TEST_CASE("The knobs slide along the rails with the volume", "[ui][originalvolume]") {
    // x = 230 + level * 12.5 (FUN_0041e780): silent at the left end of the
    // rails of vol00 (x 260 on screen), full at the right end (386).
    OriginalVolumePanel panel;
    panel.open(1);
    settle(panel);
    const auto master = volumePiece(panel, "vollit", 0);  // level 10
    const auto effects = volumePiece(panel, "vollit", 1);  // level 0
    CHECK(effects.x == Approx(230.0f));
    CHECK(master.x == Approx(355.0f));
    CHECK(master.y == Approx(320.0f));
    CHECK(effects.y == Approx(351.0f));
    // The knob's middle (24 pixels in) lies on the rails' extent.
    CHECK(effects.x + 24.0f < 262.0f);
    CHECK(master.x + 24.0f > 375.0f);
}

TEST_CASE("The selected slider's knob lights up, the other one fades", "[ui][originalvolume]") {
    // The only sign of which slider is selected is its lit knob.
    OriginalVolumePanel panel;
    panel.open(kVolumeMasterEntry);
    settle(panel);
    CHECK(panel.knobFrame(0) == 2);
    CHECK(panel.knobFrame(1) == 0);
    panel.select(kVolumeEffectsEntry);
    panel.tick();
    panel.tick();
    CHECK(panel.knobFrame(0) == 0);
    CHECK(panel.knobFrame(1) == 2);
}

TEST_CASE("Leaving the volume panel lets the list come back down", "[ui][originalvolume]") {
    // OPTIONS rises out of the panel's way and must return to its place
    // once the panel has gone, or the next menu would hang 105 pixels high.
    OriginalMenu list;
    list.open(0);
    for (int tick = 0; tick < 300 && !list.settled(); ++tick) list.tick();
    list.raise(true);
    for (int tick = 0; tick < 300 && !list.settled(); ++tick) list.tick();
    CHECK(list.panelY() == -180);
    CHECK(list.showsText());

    OriginalVolumePanel panel;
    panel.open(0);
    settle(panel);
    panel.close();
    int ticks = 0;
    while (!panel.closed() && ticks < 100) {
        panel.tick();
        ++ticks;
    }
    CHECK(panel.closed());
    CHECK(ticks < 25);  // gone in under a second
    list.raise(false);
    for (int tick = 0; tick < 300 && !list.settled(); ++tick) list.tick();
    CHECK(list.panelY() == 0);
}

// --- The longer list (ui::OriginalSelectList) ---------------------------------

namespace {

MenuPiece listPiece(const OriginalSelectList& list, const std::string& prefix) {
    for (const auto& piece : list.pieces()) {
        if (piece.picture.rfind(prefix, 0) == 0 && piece.sourceHeight == 0) return piece;
    }
    FAIL("no piece " << prefix);
    return {};
}

void settle(OriginalSelectList& list) {
    for (int tick = 0; tick < 1000 && !list.settled(); ++tick) list.tick();
    REQUIRE(list.settled());
}

} // namespace

TEST_CASE("The longer list's monitor rises into the executable's place", "[ui][originalselectlist]") {
    // The monitor comes up from below the screen (546 units) and must rest
    // at (385, 160), where the entries' text boxes fall on its dark screen.
    OriginalSelectList list;
    list.open(0);
    CHECK(listPiece(list, "counsele").y > 400.0f);
    CHECK_FALSE(list.showsText());
    settle(list);
    const auto monitor = listPiece(list, "counsele");
    CHECK(monitor.x == Approx(385.0f));
    CHECK(monitor.y == Approx(160.0f));
    CHECK(list.showsText());
    // Every entry is written inside the monitor's screen (x 425-545,
    // y 190-390 on screen).
    for (std::size_t entry = 0; entry < kOriginalSelectListEntries; ++entry) {
        const auto line = list.entryLine(entry);
        CHECK(line.centreX == Approx(485.6f).margin(0.1f));
        CHECK(line.top > 190.0f);
        CHECK(line.top + 18.0f < 400.0f);
    }
}

TEST_CASE("The selector slides to the chosen entry, its knob turning", "[ui][originalselectlist]") {
    // Five steps of one tick per entry, the knob showing one of its six
    // pictures per step; at rest the bar lies over the entry's words.
    OriginalSelectList list;
    list.open(0);
    settle(list);
    const float firstBar = listPiece(list, "select").y;
    list.select(2);
    list.tick();
    CHECK(list.selector() == 1);
    CHECK(listPiece(list, "selec").picture == "selec01");
    settle(list);
    CHECK(list.selector() == 10);
    const float bar = listPiece(list, "select").y;
    CHECK(bar - firstBar == Approx(2.0f * 23.4f).margin(1.0f));
    // The bar (64 pixels tall) is centred within a few pixels of the words.
    const auto line = list.entryLine(2);
    CHECK(std::abs((bar + 32.0f) - (line.top + 9.0f)) < 8.0f);
}
