// Tests for ui::TextLayout: text to font characters, and the placeholders
// of the game's messages.
//
// The menus and the HUD are drawn from these layouts, and the messages of
// a match ("VICTORY TO >S!!") are filled in here. Mistakes show up on
// screen as overlapping letters, missing accents in the translations, or a
// raw ">S" in front of the players.

#include "openhogs/ui/TextLayout.h"

#include "openhogs/formats/GameText.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using openhogs::formats::BitmapFont;
using openhogs::formats::GlyphRect;

namespace {

std::uint8_t codeOf(std::string_view character) {
    return static_cast<std::uint8_t>(openhogs::formats::encodeGameText(character).front());
}

// A font where 'A' is 10 wide, 'B' 6 wide, 'À' 10 wide and 12 tall, and
// '?' 7 wide; everything else is missing. Space advances 4.
BitmapFont testFont() {
    BitmapFont font;
    font.glyphs.resize(0x70);
    font.glyphs[codeOf("A")] = GlyphRect{0, 0, 10, 12};
    font.glyphs[codeOf("B")] = GlyphRect{10, 0, 6, 12};
    font.glyphs[codeOf("À")] = GlyphRect{16, 0, 10, 12};
    font.glyphs[codeOf("?")] = GlyphRect{26, 0, 7, 12};
    font.lineHeight = 12;
    font.spaceWidth = 4;
    return font;
}

} // namespace

TEST_CASE("Characters are placed one after the other at their own widths", "[ui][text]") {
    // The fonts are proportional: a fixed advance would leave gaps after
    // narrow letters and overlap wide ones.
    const auto quads = layoutText(testFont(), "AB A", 100.0f, 50.0f);
    REQUIRE(quads.size() == 3);
    CHECK(quads[0].x == 100.0f);
    CHECK(quads[0].y == 50.0f);
    CHECK(quads[1].x == 110.0f);
    CHECK(quads[2].x == 120.0f);  // 'B' (6) + space (4)
    CHECK(measureText(testFont(), "AB A") == 30.0f);
}

TEST_CASE("Scaling multiplies positions and sizes", "[ui][text]") {
    // The HUD draws the same font at several sizes.
    const auto quads = layoutText(testFont(), "AB", 0.0f, 0.0f, 2.0f);
    REQUIRE(quads.size() == 2);
    CHECK(quads[0].width == 20.0f);
    CHECK(quads[0].height == 24.0f);
    CHECK(quads[1].x == 20.0f);
    CHECK(measureText(testFont(), "AB", 2.0f) == 32.0f);
}

TEST_CASE("Lower case and accented letters use the game's capitals", "[ui][text]") {
    // The fonts only have capitals; translations such as Italian write
    // "Modalità", which must show as "MODALITÀ", not lose the accent.
    const auto quads = layoutText(testFont(), "aà", 0.0f, 0.0f);
    REQUIRE(quads.size() == 2);
    CHECK(quads[0].source.x == 0);
    CHECK(quads[1].source.x == 16);
}

TEST_CASE("Characters the game cannot show become question marks", "[ui][text]") {
    // A stray character in a translation (an emoji, a Polish letter) must
    // not stop the game or swallow the rest of the line.
    const auto codes = encodeForDisplay("A€B");
    REQUIRE(codes.size() == 3);
    CHECK(codes[1] == codeOf("?"));
    CHECK(layoutText(testFont(), "A€B", 0.0f, 0.0f).size() == 3);
}

TEST_CASE("Placeholders take strings and numbers in order", "[ui][text]") {
    // The match messages of gtext use ">S" for names and ">N" for numbers.
    CHECK(fillPlaceholders("VICTORY TO >S!!", {"SUSHI-SWINE"}) == "VICTORY TO SUSHI-SWINE!!");
    CHECK(fillPlaceholders("GET READY >S...", {"TEAM LARD"}) == "GET READY TEAM LARD...");
    CHECK(fillPlaceholders("ROUND >N", {}, {7}) == "ROUND 7");
}

TEST_CASE("A digit before N pads the number with zeros", "[ui][text]") {
    // "MISSION >2N : >S" numbers the campaign missions with two digits.
    CHECK(fillPlaceholders("MISSION >2N : >S", {"THE BRIDGE"}, {3}) == "MISSION 03 : THE BRIDGE");
    CHECK(fillPlaceholders(">2N", {}, {12}) == "12");
}

TEST_CASE("Placeholders without a value disappear, other '>' stay", "[ui][text]") {
    // A missing name must never show as a raw ">S"; an ordinary '>' (as in
    // an arrow) is not a placeholder.
    CHECK(fillPlaceholders("VICTORY TO >S!!", {}) == "VICTORY TO !!");
    CHECK(fillPlaceholders("A > B", {"X"}) == "A > B");
    CHECK(fillPlaceholders("END>", {"X"}) == "END>");
}
