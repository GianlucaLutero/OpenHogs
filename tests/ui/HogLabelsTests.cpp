// Tests for ui::HogLabels: the labels over the hogs, as the PC executable
// lays them out (FUN_00459b20).
//
// The labels tell the players who is who on the field. One in the wrong
// place points at the wrong hog; a spy's label in the enemy's turn gives
// it away; a label that never fades in or out hides the view.

#include "openhogs/ui/HogLabels.h"

#include "../game/GameTestSupport.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>

using namespace openhogs::ui;
using namespace openhogs::game;
using namespace openhogs::tests;
using Catch::Matchers::WithinAbs;

namespace {

constexpr glm::vec2 kScreen{640.0f, 480.0f};

} // namespace

// The label stands on the top of its hog: the hog's square is 182400 /
// depth pixels with the original's focal length, and the 70 x 48 box sits
// centred right above it. Wrong numbers float the label away from its hog.
TEST_CASE("A label stands on top of its hog's square", "[ui][labels]") {
    const auto labels = layoutHogLabels({{0, {320.0f, 240.0f}, 1824.0f}}, kScreen, kOriginalFocalLength, true);
    REQUIRE(labels.size() == 1);
    const auto& label = labels[0];
    CHECK_THAT(label.hogRect.size.x, WithinAbs(100.0f, 0.01f));  // 182400 / 1824
    CHECK_THAT(label.box.position.x, WithinAbs(320.0f - 35.0f, 0.01f));
    CHECK_THAT(label.box.position.y + label.box.size.y, WithinAbs(240.0f - 50.0f, 0.01f));
    CHECK(label.place == LabelPlace::Above);
    CHECK_FALSE(label.arrow.has_value());

    // A wider view (a shorter focal length) makes the hog smaller on the
    // screen, so its label comes down closer to its centre.
    const auto wide = layoutHogLabels({{0, {320.0f, 240.0f}, 1824.0f}}, kScreen, kOriginalFocalLength / 2.0f, true);
    CHECK_THAT(wide[0].hogRect.size.x, WithinAbs(50.0f, 0.01f));
}

// Labels are drawn from the farthest hog to the nearest, so the near ones
// stay readable on top; when two would overlap, the later one moves to a
// corner of its hog and an arrow points back at it.
TEST_CASE("Overlapping labels move aside with an arrow", "[ui][labels]") {
    const std::vector<HogOnScreen> hogs{
        {7, {330.0f, 330.0f}, 3000.0f},  // near hog, drawn last
        {4, {320.0f, 300.0f}, 6000.0f},  // far hog right behind it
    };
    const auto labels = layoutHogLabels(hogs, kScreen, kOriginalFocalLength, true);
    REQUIRE(labels.size() == 2);
    CHECK(labels[0].hog == 4);
    CHECK(labels[1].hog == 7);
    // The far hog's label stays put; the near one would cover it and moves.
    CHECK(labels[0].place == LabelPlace::Above);
    CHECK(labels[1].place != LabelPlace::Above);
    CHECK(labels[1].arrow.has_value());
    CHECK_FALSE(labels[1].box.overlaps(labels[0].box));

    // The overview cameras (the executable's modes 7 and 9) leave every
    // label on top of its hog.
    const auto overview = layoutHogLabels(hogs, kScreen, kOriginalFocalLength, false);
    CHECK(overview[1].place == LabelPlace::Above);
}

// A label that would leave the screen moves to a side that fits.
TEST_CASE("A label near the screen's edge moves back onto it", "[ui][labels]") {
    const auto labels = layoutHogLabels({{0, {600.0f, 40.0f}, 3000.0f}}, kScreen, kOriginalFocalLength, true);
    REQUIRE(labels.size() == 1);
    CHECK(labels[0].box.inside(kScreen));
    CHECK(labels[0].place == LabelPlace::BelowLeft);
}

// Every living hog has a label; dead ones and hogs hidden in a crate do
// not. An espionage hog's label only
// shows in its own team's turn, or the enemy would find it at once.
TEST_CASE("Which hogs carry a label", "[ui][labels]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 500, 0, 0), makeHog(map, 1000, 0, 2),
                          makeHog(map, 1500, 0, 2)};
    hogs[1].alive = false;
    hogs[3].rank = RankId::Spy;
    World world(map, hogs);
    static const auto animations = makeAnimations();
    world.update(kStep, PlayerInput{}, animations);

    REQUIRE(world.turns()->activeTeam() == 0);
    CHECK(labelledHogs(world) == std::vector<std::size_t>{0, 2});
}

// Hogs without a name of their own (the enemies, skirmish teams) take the
// names of their nation's list, the n-th hog of a team the n-th name, as
// the footage's enemies do; the label switches between name and rank every
// 1.28 seconds (bit 7 of the clock in hundredths).
TEST_CASE("A label shows the hog's name and rank in turn", "[ui][labels]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 2), makeHog(map, 1500, 0, 2)};
    World world(map, hogs);
    world.setTeamNation(2, 2);  // French
    UiStrings strings;

    CHECK(describeHogLabel(world, strings, 1).text == "BASTILLE");
    CHECK(describeHogLabel(world, strings, 2).text == "LE CONT");
    CHECK(describeHogLabel(world, strings, 2).health == 100);
    CHECK(describeHogLabel(world, strings, 2).nation == 2);
    CHECK(describeHogLabel(world, strings, 2).showHealth);

    static const auto animations = makeAnimations();
    while (world.timeSeconds() < kHogLabelSwapSeconds + 0.1) world.update(kStep, PlayerInput{}, animations);
    CHECK(describeHogLabel(world, strings, 2).text == "GRUNT");
}

// A hog inside a building or vehicle shows the object's name and health
// instead of its own (FUN_0045a5c0), as the footage's enemy gunner in its
// artillery does (ARTIGLIERIA 200, part 17, 30:45): the players see what
// they must knock down to get at it.
TEST_CASE("A hog inside a building is labelled with the building", "[ui][labels]") {
    CollisionBox box;
    box.centre = {300.0f, 100.0f, 0.0f};
    box.halfSize = {100.0f, 100.0f, 100.0f};
    box.object = 0;
    const auto map = flatMap({box});
    std::vector<Hog> hogs{makeHog(map, 300, 0, 0), makeHog(map, 1500, 0, 2)};
    World world(map, hogs);
    world.setDestructibles({{0, ObjectClass::Building, 200.0f, false}});
    world.setMounts({{0, MountKind::Artillery, "BIG_GUN"}});  // the hog stands inside: it starts in it
    REQUIRE(world.hogs()[0].sheltered());
    UiStrings strings;

    const auto labelled = labelledHogs(world);
    CHECK(std::find(labelled.begin(), labelled.end(), 0u) != labelled.end());
    const auto label = describeHogLabel(world, strings, 0);
    CHECK(label.text == "ARTILLERY");
    CHECK(label.health == 200);
    CHECK(label.nation == world.nationOf(0));
}

// A hurt hog's label runs its health down instead of dropping it at once,
// slowly at first and then faster, as on the footage (part 3, 1:45-1:48:
// 30 to 17 in about 2.3 s); healing shows at once. Without it the label
// would contradict the original's pace; with a wrong end point it would
// show a health the hog does not have.
TEST_CASE("A label's health runs down to the hog's", "[ui][labels]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1500, 0, 2)};
    hogs[1].health = 30.0f;
    World world(map, hogs);
    HealthCountdown countdown;
    countdown.update(world, 0.0);  // starts following the hogs

    hogs[1].health = 17.0f;
    World hurt(map, hogs);  // the same hogs, the second one hurt
    countdown.update(hurt, 1.0);
    CHECK_THAT(countdown.shown(hurt, 1), WithinAbs(30.0f - 2.5f, 0.01f));  // 2.5 x 1^2
    countdown.update(hurt, 1.0);
    CHECK_THAT(countdown.shown(hurt, 1), WithinAbs(30.0f - 10.0f, 0.01f));  // 2.5 x 2^2
    countdown.update(hurt, 1.0);
    CHECK_THAT(countdown.shown(hurt, 1), WithinAbs(17.0f, 0.01f));  // never below the real health
    CHECK(describeHogLabel(hurt, UiStrings{}, 1, countdown.shown(hurt, 1)).health == 17);
    CHECK_THAT(countdown.shown(hurt, 0), WithinAbs(100.0f, 0.01f));  // untouched

    countdown.update(world, 0.1);  // healed back to 30: at once
    CHECK_THAT(countdown.shown(world, 1), WithinAbs(30.0f, 0.01f));
}

// The labels fade in while the camera stands still and out while it
// moves, as fast as the executable (+25 and -40 of 255 a tick); the arrows
// wait until the labels are fully there.
TEST_CASE("Labels fade with the camera's movement", "[ui][labels]") {
    HogLabelFade fade;
    CHECK(fade.alpha() == 0.0f);
    fade.update(0.2, true);  // 5 ticks: 125
    CHECK_THAT(fade.alpha(), WithinAbs(125.0f / 255.0f, 0.001f));
    CHECK_FALSE(fade.full());
    fade.update(1.0, true);
    CHECK(fade.full());
    fade.update(0.12, false);  // 3 ticks: 255 - 120
    CHECK_THAT(fade.alpha(), WithinAbs(135.0f / 255.0f, 0.001f));
}
