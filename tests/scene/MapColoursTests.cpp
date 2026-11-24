// Tests for scene::gradeColour: the recolouring the original gives each
// level's textures (level table fields 1-4, applied by Data/_d3d.dll).
//
// The values below are worked out by hand from the renderer's arithmetic
// on 5-bit channels. A slip here turns night maps green or makes every
// map darker than the original's.

#include "openhogs/scene/MapColours.h"

#include <catch2/catch_test_macros.hpp>

using openhogs::scene::gradeColour;
using openhogs::scene::PaletteGrading;

namespace {

std::uint16_t colour(int red, int green, int blue) {
    return static_cast<std::uint16_t>(red | (green << 5) | (blue << 10));
}

} // namespace

// Even a level with no settings is brightened: the PC renderer adds 3 to
// every channel (its global at 0x10022058). Without it every OpenHogs map
// is a little darker than the PC game's.
TEST_CASE("gradeColour adds the renderer's brightness to every level", "[scene][colours]") {
    CHECK(gradeColour(colour(10, 20, 30), PaletteGrading{}) == colour(13, 23, 31));  // clamped at 31
    // The snow maps' -2 leaves +1.
    CHECK(gradeColour(colour(10, 20, 30), PaletteGrading{0, -2, 0, {0, 0, 0}}) == colour(11, 21, 31));
}

// The transparent colour must stay transparent (it cuts out leaves and
// fences), the semi-transparency bit must survive, and no colour may turn
// into the transparent black.
TEST_CASE("gradeColour keeps transparency apart", "[scene][colours]") {
    const PaletteGrading darker{0, -10, 0, {0, 0, 0}};
    CHECK(gradeColour(0x0000, darker) == 0x0000);
    CHECK(gradeColour(colour(2, 2, 2), darker) == colour(1, 0, 0));
    CHECK((gradeColour(static_cast<std::uint16_t>(0x8000 | colour(5, 5, 5)), PaletteGrading{}) & 0x8000) != 0);
}

// The night grading (BAY, CREEPY2, the moon): two steps towards grey,
// +3+3 brightness, then 60 per cent of a blue tint. Pure green grass must
// come out blue-grey, as the night map does in the footage.
TEST_CASE("gradeColour turns the night map blue", "[scene][colours]") {
    const PaletteGrading night{2, 3, 60, {3, 3, 13}};
    // Green 0,20,0: grey 6; desaturated 2,18,2; brightened 8,24,8; grey
    // 13; tint (3+8)*13>>4 = 8, 8 and (13+8)*13>>4 = 17; mixed 60/40:
    // (8*60+8*40)/100 = 8, (8*60+24*40)/100 = 14, (17*60+8*40)/100 = 13.
    CHECK(gradeColour(colour(0, 20, 0), night) == colour(8, 14, 13));
}

// Five desaturation steps mean "all grey" (OASIS and SEPIA1 use 2).
TEST_CASE("gradeColour turns colours grey with desaturation 5", "[scene][colours]") {
    const PaletteGrading grey{5, -3, 0, {0, 0, 0}};  // -3 cancels the renderer's +3
    CHECK(gradeColour(colour(30, 0, 0), grey) == colour(10, 10, 10));
}
