// Tests for the text box (ui/TextBox.h). The box tells the player what a
// crate held and, in the Boot Camp, what to do next: no message may be
// lost when several come at once, a short one must stay long enough to be
// read, and a long one must scroll all the way through before the box
// goes away.

#include "openhogs/ui/TextBox.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using Catch::Approx;

namespace {

// Ticks until the box has nothing more to show (or the limit).
int ticksUntilEmpty(TextBox& box, int limit = 10000) {
    int ticks = 0;
    while (box.current() && ticks < limit) {
        box.tick();
        ++ticks;
    }
    return ticks;
}

} // namespace

TEST_CASE("A short message comes down, stays three seconds and goes back up", "[ui][textbox]") {
    // The executable's timings: 10 ticks down along its bouncing table,
    // 300 hundredths of a second counted from the start, 10 ticks up.
    TextBox box;
    box.post("BAZOOKA X3", 100.0f);
    REQUIRE(box.current());
    CHECK(box.lowered() == Approx(0.0f));
    CHECK(box.textX() == Approx((TextBox::kWindowWidth - 100.0f) / 2.0f));  // centred

    float highest = 0.0f;
    for (int tick = 0; tick < 9; ++tick) {
        box.tick();
        highest = std::max(highest, box.lowered());
    }
    CHECK(box.lowered() == Approx(1.0f));
    CHECK(highest == Approx(1.0f));

    TextBox timed;
    timed.post("BAZOOKA X3", 100.0f);
    const int ticks = ticksUntilEmpty(timed);
    CHECK(ticks == TextBox::kMessageTicks + TextBox::kSlideSteps);
}

TEST_CASE("A long message scrolls through the window before the box leaves", "[ui][textbox]") {
    // It enters at the window's right edge and must have gone past the left
    // edge, whatever the box's three seconds say.
    TextBox box;
    const float width = 600.0f;
    box.post("A VERY LONG INSTRUCTION FROM THE SERGEANT ABOUT GRENADES", width, 5.0f);
    CHECK(box.scrolls());
    CHECK(box.textX() == Approx(TextBox::kWindowWidth));
    float lastX = box.textX();
    bool gonePast = false;
    for (int tick = 0; tick < 1000 && box.current() && !gonePast; ++tick) {
        box.tick();
        CHECK(box.textX() <= lastX);  // always to the left
        lastX = box.textX();
        gonePast = lastX <= -width;
    }
    CHECK(gonePast);  // the box was still there until the whole text had gone by
    const int ticks = ticksUntilEmpty(box);
    CHECK(ticks <= TextBox::kSlideSteps + 1);
}

TEST_CASE("Messages posted together are shown one after the other, up to eight", "[ui][textbox]") {
    // Two crates picked up in one step must both be announced; a flood of
    // messages is cut at the queue's size rather than growing forever.
    TextBox box;
    box.post("FIRST", 50.0f);
    box.post("SECOND", 50.0f);
    REQUIRE(box.current());
    CHECK(box.current()->text == "FIRST");
    for (int tick = 0; tick < TextBox::kMessageTicks + TextBox::kSlideSteps; ++tick) box.tick();
    REQUIRE(box.current());
    CHECK(box.current()->text == "SECOND");
    CHECK(box.lowered() == Approx(0.0f));  // the second comes down afresh

    TextBox full;
    for (int i = 0; i < 12; ++i) full.post("MESSAGE " + std::to_string(i), 50.0f);
    int shown = 0;
    std::string last;
    for (int tick = 0; tick < 5000 && full.current(); ++tick) {
        if (full.current()->text != last) {
            last = full.current()->text;
            ++shown;
        }
        full.tick();
    }
    CHECK(shown == static_cast<int>(TextBox::kQueueSize));
}

TEST_CASE("The teams' strength stays four seconds in the big tube", "[ui][textbox]") {
    // FUN_0045ec50 asks for 400 hundredths of a second.
    TextBox box;
    box.postTeamStrength();
    REQUIRE(box.current());
    CHECK(box.current()->kind == TextBoxKind::TeamStrength);
    CHECK(ticksUntilEmpty(box) == TextBox::kTeamStrengthTicks + TextBox::kSlideSteps);
}
