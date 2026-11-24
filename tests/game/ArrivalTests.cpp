// Tests for the opening of a campaign mission: World::beginArrival and
// game::arrivalCameraPose.
//
// The original opens every campaign mission (but the Boot Camp) with the
// squads coming down by parachute under the mission's title before the
// first turn (docs/video/campaign.md). If the turn clock ran meanwhile,
// the hogs were hurt by the drop, or the first turn never came, every
// mission would start broken.

#include "openhogs/game/World.h"

#include "GameTestSupport.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs::game;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> run(World& world, double seconds, const PlayerInput& input = {}) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (double t = 0.0; t < seconds - 1e-9; t += kStep) {
        const auto events = world.update(kStep, input, animations);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

bool has(const std::vector<GameEvent>& events, GameEventType type) {
    return std::any_of(events.begin(), events.end(), [type](const GameEvent& event) { return event.type == type; });
}

} // namespace

TEST_CASE("A campaign mission opens with the squads parachuting in", "[game][arrival]") {
    // Everybody starts high up under a parachute and comes down unhurt;
    // the turn clock waits, the player's keys do nothing, and the first
    // turn is announced once all have landed and the hold has passed.
    const auto map = flatMap();
    World world(map, {makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 1)});
    const float groundY = world.hogs()[0].position.y;
    const double clock = world.turns()->secondsLeft();
    const float firstX = world.hogs()[0].position.x;
    world.beginArrival();
    REQUIRE(world.arriving());
    for (const auto& hog : world.hogs()) {
        CHECK(hog.parachute);
        CHECK(hog.position.y == Approx(groundY + kArrivalDropHeight));
    }

    PlayerInput forward;
    forward.movement.forward = true;
    auto events = run(world, 4.0, forward);
    CHECK(world.arriving());
    CHECK(world.hogs()[0].position.y < groundY + kArrivalDropHeight);
    CHECK(world.turns()->secondsLeft() == Approx(clock));
    CHECK_FALSE(has(events, GameEventType::TurnStarted));

    events = run(world, 4.0);  // down after about 6.5 seconds
    for (const auto& hog : world.hogs()) {
        CHECK_FALSE(hog.parachute);
        CHECK(hog.health == kMaxHealth);
        CHECK(hog.position.y == Approx(groundY).margin(1.0));
    }
    CHECK(world.hogs()[0].position.x == Approx(firstX));  // straight down: the keys did not move it
    CHECK(world.arriving());  // the title stays up a while

    events = run(world, kArrivalHoldSeconds);
    CHECK(world.phase() == MatchPhase::Playing);
    CHECK(has(events, GameEventType::TurnStarted));
}

TEST_CASE("Enter lands the squads and starts the first turn", "[game][arrival]") {
    // The opening can be skipped: everybody on the ground at once, the
    // turn on the next step.
    const auto map = flatMap();
    World world(map, {makeHog(map, 0, 0, 0), makeHog(map, 1000, 0, 1)});
    const float groundY = world.hogs()[1].position.y;
    world.beginArrival();
    run(world, 0.5);
    world.skipArrival();
    CHECK(world.hogs()[1].position.y == Approx(groundY).margin(1.0));
    CHECK_FALSE(world.hogs()[1].parachute);
    const auto events = run(world, kStep);
    CHECK(world.phase() == MatchPhase::Playing);
    CHECK(has(events, GameEventType::TurnStarted));
}

TEST_CASE("The opening's camera looks up at the hog from behind, above the ground", "[game][arrival]") {
    // As in the footage: the hog against the sky, the camera behind it
    // and lower; never under the ground when the hog is low.
    const auto terrain = flatGround(100);
    const glm::vec3 facing(0.0f, 0.0f, -1.0f);
    const auto high = arrivalCameraPose(glm::vec3(0.0f, 2000.0f, 0.0f), facing, terrain);
    CHECK(high.pitchDegrees > 10.0f);
    CHECK(high.position.z > 0.0f);  // behind: the hog faces -Z
    CHECK(high.position.y < 2000.0f);
    const auto low = arrivalCameraPose(glm::vec3(0.0f, 150.0f, 0.0f), facing, terrain);
    CHECK(low.position.y > 100.0f);
}
