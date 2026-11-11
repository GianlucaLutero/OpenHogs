// Tests for ui::HudLayout and ui::FloatingNumbers: the original HUD's
// instruments and the damage numbers.
//
// The positions and rules come from the original executable; these tests
// protect what a player reads from the instruments (the aim, the power,
// the seconds left) and that the HUD keeps its place on wide screens.

#include "openhogs/ui/FloatingNumbers.h"
#include "openhogs/ui/HudLayout.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using Catch::Approx;

TEST_CASE("HUD pieces keep their distance from their side of the screen", "[ui][hudlayout]") {
    // On a 16:9 screen the canvas is wider than the original 640 pixels:
    // the clock must stay in the bottom-right corner and the power gauge
    // in the middle, or they would float into the scene.
    CHECK(anchoredX(503.0f, HudAnchor::Right, 640.0f) == Approx(503.0f));
    CHECK(anchoredX(503.0f, HudAnchor::Right, 853.0f) == Approx(716.0f));
    CHECK(anchoredX(160.0f, HudAnchor::Centre, 853.0f) == Approx(266.5f));
    CHECK(anchoredX(20.0f, HudAnchor::Left, 853.0f) == Approx(20.0f));
}

TEST_CASE("The aim arrow points the way the hog aims", "[ui][hudlayout]") {
    // The arrow is the only reading of the aim: flat must point left
    // (towards the dial), up must point up, and the head must stay on the
    // dial's 50-pixel radius.
    const glm::vec2 pivot{536.0f, 73.5f};
    const auto headCentre = [&](float degrees) {
        const auto corners = aimPointerCorners(degrees, pivot);
        return (corners[0] + corners[3]) / 2.0f;
    };
    CHECK(headCentre(0.0f).x == Approx(486.0f));
    CHECK(headCentre(0.0f).y == Approx(73.5f));
    CHECK(headCentre(90.0f).x == Approx(536.0f).margin(1e-3));
    CHECK(headCentre(90.0f).y == Approx(23.5f));
    CHECK(headCentre(-90.0f).y == Approx(123.5f));
    CHECK(glm::length(headCentre(37.0f) - pivot) == Approx(50.0f));
}

TEST_CASE("The power needle runs along the gauge's scale", "[ui][hudlayout]") {
    // No power puts the needle at the start of the scale, full power at
    // its end; anything outside 0..1 must not push it off the tube.
    CHECK(powerNeedleX(0.0f) == Approx(255.0f));
    CHECK(powerNeedleX(1.0f) == Approx(406.0f));
    CHECK(powerNeedleX(0.5f) == Approx(330.5f));
    CHECK(powerNeedleX(2.0f) == Approx(406.0f));
    CHECK(powerNeedleX(-1.0f) == Approx(255.0f));
}

TEST_CASE("The clock shows the seconds rounded up and rolls before each change", "[ui][hudlayout]") {
    // The player must never see 0 while the hog can still act, and the
    // digits only roll in the last moment before they change.
    auto digits = clockDigits(59.5);
    CHECK(digits.tens == 6);
    CHECK(digits.units == 0);
    CHECK(digits.unitsRoll == 0.0f);

    digits = clockDigits(0.3);
    CHECK(digits.tens == 0);
    CHECK(digits.units == 1);

    // 7.05 s: "8" is about to become "7", the units digit is rolling.
    digits = clockDigits(7.05);
    CHECK(digits.units == 8);
    CHECK(digits.unitsRoll > 0.0f);
    CHECK(digits.tensRoll == 0.0f);

    // 9.05 s: "10" becomes "9", so the tens digit rolls too.
    digits = clockDigits(9.05);
    CHECK(digits.tens == 1);
    CHECK(digits.units == 0);
    CHECK(digits.tensRoll == digits.unitsRoll);
    CHECK(digits.tensRoll > 0.0f);
}

TEST_CASE("The clock lamp warns when time runs out", "[ui][hudlayout]") {
    // Amber under 10 seconds, red under 5; a new turn with a full clock
    // turns it green again.
    CHECK(nextClockLamp(ClockLamp::Green, 30.0, false) == ClockLamp::Green);
    CHECK(nextClockLamp(ClockLamp::Green, 9.5, false) == ClockLamp::Amber);
    CHECK(nextClockLamp(ClockLamp::Amber, 4.0, false) == ClockLamp::Red);
    CHECK(nextClockLamp(ClockLamp::Red, 30.0, false) == ClockLamp::Red);
    CHECK(nextClockLamp(ClockLamp::Red, 60.0, true) == ClockLamp::Green);
}

TEST_CASE("Instruments slide in and out and the aim fades", "[ui][hudlayout]") {
    // A gauge that is not needed must leave the screen completely, and
    // one that is must end exactly at its place.
    HudSlide slide;
    CHECK_FALSE(slide.visible());
    CHECK(slide.offset() == Approx(200.0f));
    for (int i = 0; i < HudSlide::kTicks; ++i) slide.tick(true);
    CHECK(slide.offset() == Approx(0.0f));
    slide.tick(true);
    CHECK(slide.offset() == Approx(0.0f));
    for (int i = 0; i < HudSlide::kTicks; ++i) slide.tick(false);
    CHECK_FALSE(slide.visible());

    HudFade fade;
    for (int i = 0; i < 20; ++i) fade.tick(true);
    CHECK(fade.level() == HudFade::kMaximum);
    CHECK(fade.pointerAlpha() == Approx(250.0f / 255.0f));
    CHECK(fade.fanAlpha() < 0.25f);  // the fan is only a faint glow
}

TEST_CASE("The HUD animates at 25 ticks per second whatever the frame rate", "[ui][hudlayout]") {
    // At 60 frames per second the instruments must move as fast as at 25,
    // and a long pause must not make them jump around for seconds.
    HudTicker ticker;
    int ticks = 0;
    for (int frame = 0; frame < 60; ++frame) ticks += ticker.advance(1.0 / 60.0);
    CHECK(ticks == 25);
    CHECK(ticker.advance(10.0) == HudSlide::kTicks);
}

TEST_CASE("Turn messages take the colours of the executable's teams", "[ui][hudlayout]") {
    // The colours follow the uniforms; Russian and Japanese are listed in
    // a different order in the character files and must not be swapped.
    CHECK(teamMessageColour(0) == glm::vec4(70.0f / 255.0f, 240.0f / 255.0f, 70.0f / 255.0f, 1.0f));  // British
    CHECK(teamMessageColour(4).r > teamMessageColour(4).b);                                              // Japanese
    CHECK(teamMessageColour(4).g > 0.9f);
    CHECK(teamMessageColour(5).g < 0.3f);  // Russian: red
    CHECK(teamMessageColour(6) == teamMessageColour(13));
}

TEST_CASE("Damage numbers rise, fade and disappear", "[ui][floatingnumbers]") {
    // A hit must always show a number (at least 1), which floats up and
    // is gone after its life so the screen does not fill with old hits.
    FloatingNumbers numbers;
    numbers.add({0.0f, 100.0f, 0.0f}, 0.2f, glm::vec4(1.0f));
    numbers.add({0.0f, 100.0f, 0.0f}, 34.6f, glm::vec4(1.0f));
    REQUIRE(numbers.numbers().size() == 2);
    CHECK(numbers.numbers()[0].text == "1");
    CHECK(numbers.numbers()[1].text == "35");

    numbers.update(FloatingNumbers::kLifeSeconds / 2.0);
    CHECK(numbers.numbers()[0].position.y > 100.0f);
    CHECK(numbers.numbers()[0].colour.a == Approx(0.5f));

    numbers.update(FloatingNumbers::kLifeSeconds);
    CHECK(numbers.numbers().empty());
}

TEST_CASE("Damage digits shrink with distance within limits", "[ui][floatingnumbers]") {
    // Far numbers stay readable (10 pixels, as in the executable) and
    // close ones do not cover the screen.
    CHECK(floatingDigitSize(1500.0f) == Approx(24.0f));
    CHECK(floatingDigitSize(3000.0f) < floatingDigitSize(1500.0f));
    CHECK(floatingDigitSize(1.0e6f) == Approx(10.0f));
    CHECK(floatingDigitSize(10.0f) == Approx(48.0f));
}
