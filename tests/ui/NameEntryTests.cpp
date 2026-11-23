// Tests for ui::NameEntry (typing a name on the letter grid) and
// ui::OriginalSaveBoard (the loading and saving board's moving parts).
//
// A grid that lets the cursor fall off the letters, a name longer than the
// executable allows, or a lamp that does not follow the chosen slot would
// all show on the original screens these classes drive.

#include "openhogs/ui/NameEntry.h"
#include "openhogs/ui/OriginalSaveBoard.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace openhogs::ui;

TEST_CASE("Typed letters go into the name and the cursor waits on END", "[ui][names]") {
    // As FUN_0042e440: a typed letter is added and the cursor goes back to
    // END, so pressing Enter afterwards finishes the name.
    NameEntry entry("PLEASE NAME YOUR PIG", "DEN", kHogNameLength);
    CHECK(entry.handle(MenuInput::Left) == NameEntry::Result::Moved);
    CHECK(entry.type('k') == NameEntry::Result::Changed);
    CHECK(entry.type('7') == NameEntry::Result::Changed);
    CHECK(entry.type('#') == NameEntry::Result::None);  // not in the font
    CHECK(entry.name() == "K7");
    CHECK(entry.keyAtCursor() == NameEntry::Key::End);
    CHECK(entry.handle(MenuInput::Confirm) == NameEntry::Result::Done);
}

TEST_CASE("A name stops at the executable's length", "[ui][names]") {
    // Seven letters for a hog: an eighth is refused, not cut silently later.
    NameEntry entry("PLEASE NAME YOUR PIG", "", kHogNameLength);
    for (const char letter : std::string("ABCDEFG")) CHECK(entry.type(letter) == NameEntry::Result::Changed);
    CHECK(entry.type('H') == NameEntry::Result::Refused);
    CHECK(entry.name() == "ABCDEFG");
}

TEST_CASE("END keeps the old name until something is typed", "[ui][names]") {
    // The executable copies its buffer only after a key was used; a name
    // typed and then wiped out cannot be confirmed.
    NameEntry untouched("PLEASE NAME YOUR TEAM", "TOMMY'S TROTTERS", kTeamNameLength);
    CHECK(untouched.handle(MenuInput::Confirm) == NameEntry::Result::Done);
    CHECK_FALSE(untouched.edited());

    NameEntry wiped("PLEASE NAME YOUR TEAM", "TOMMY'S TROTTERS", kTeamNameLength);
    wiped.type('A');
    wiped.erase();
    CHECK(wiped.handle(MenuInput::Confirm) == NameEntry::Result::Refused);
}

TEST_CASE("The grid's cursor stays on letters and keys", "[ui][names]") {
    // The last row is short and the keys' column has three rows: moving
    // past them must stop, and picking DELETE or SPACE must edit the name.
    NameEntry entry("PLEASE NAME YOUR PIG", "", kHogNameLength);
    CHECK(entry.handle(MenuInput::Down) == NameEntry::Result::None);  // below END
    CHECK(entry.handle(MenuInput::Up) == NameEntry::Result::Moved);
    CHECK(entry.keyAtCursor() == NameEntry::Key::Space);
    CHECK(entry.handle(MenuInput::Confirm) == NameEntry::Result::Changed);
    CHECK(entry.name() == " ");
    CHECK(entry.handle(MenuInput::Up) == NameEntry::Result::Moved);
    CHECK(entry.keyAtCursor() == NameEntry::Key::Delete);
    CHECK(entry.handle(MenuInput::Confirm) == NameEntry::Result::Changed);
    CHECK(entry.name().empty());
    // Left from DELETE: the last letter of the first row (K).
    CHECK(entry.handle(MenuInput::Left) == NameEntry::Result::Moved);
    CHECK(entry.letterAt(entry.column(), entry.row()) == 'K');
    // Down the last column: the short last row has no letter there.
    entry.handle(MenuInput::Down);
    entry.handle(MenuInput::Down);
    CHECK(entry.handle(MenuInput::Down) == NameEntry::Result::None);
    CHECK(entry.letterAt(entry.column(), entry.row()) == '0');
}

TEST_CASE("The chosen slot's lamp blinks while the others go dark", "[ui][saveboard]") {
    // FUN_00427c90: the chosen lamp runs animation 0x3EE (half lit and lit,
    // 5 ticks each); the others step down to off.
    OriginalSaveBoard board;
    board.open(3);
    for (int i = 0; i < 2; ++i) board.tick();
    const auto lampOf = [&](std::size_t slot) {
        for (const auto& piece : board.pieces({})) {
            if (piece.picture.rfind("pclit", 0) == 0 && piece.y > 0.0f) {
                // The lamps are listed first, slot by slot.
                if (slot == 0) return piece.picture;
                --slot;
            }
        }
        return std::string();
    };
    CHECK(lampOf(0) == "pclit0");
    CHECK(lampOf(3) == "pclit1");
    for (int i = 0; i < 5; ++i) board.tick();
    CHECK(lampOf(3) == "pclit2");
}

TEST_CASE("The saving board's panel shows the saved squad's badges once open", "[ui][saveboard]") {
    // FUN_0042e310: a Gunner shows the heavy weapons badge, a Medic the
    // medic badge beside its stripe, a Grunt nothing; and only when the
    // panel has swung open.
    using openhogs::game::RankId;
    OriginalSaveBoard board;
    board.open(0);
    const std::vector<RankId> squad{RankId::Gunner, RankId::Medic, RankId::Grunt};
    const auto has = [&](const std::string& picture) {
        for (const auto& piece : board.pieces(squad)) {
            if (piece.picture == picture) return true;
        }
        return false;
    };
    CHECK_FALSE(has("pcHweap"));
    for (int i = 0; i < 200; ++i) board.tick();
    REQUIRE(board.showsInfo());
    CHECK(has("pcHweap"));
    CHECK(has("pcmedic"));
    CHECK(has("strp1"));
    CHECK(has("pcsvinf4"));
}
