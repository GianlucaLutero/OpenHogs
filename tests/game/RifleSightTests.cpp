// Tests for the rifle sight in a match (World::sightView). They protect
// the rules seen in the original: only guns can be aimed through it, the
// hog stands still while the player looks (it may still turn and aim),
// and the view looks exactly along the line the bullet will take.

#include "GameTestSupport.h"

#include "openhogs/game/World.h"

using namespace openhogs;
using namespace openhogs::tests;

namespace {

std::vector<GameEvent> run(World& world, const PlayerInput& input, int steps) {
    static const auto animations = makeAnimations();
    std::vector<GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto step = world.update(kStep, input, animations);
        all.insert(all.end(), step.begin(), step.end());
    }
    return all;
}

World duelWith(const MapCollision& map, WeaponId weapon) {
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 1500, 0, 1)};
    hogs[0].weapons = {weapon};
    hogs[0].aimDegrees = 0.0f;  // level, at the enemy's height
    return World(map, hogs);
}

} // namespace

TEST_CASE("Only guns are aimed through the rifle sight", "[game][sight]") {
    const auto map = flatMap();
    PlayerInput look;
    look.sight = true;

    World rifle = duelWith(map, WeaponId::Rifle);
    run(rifle, look, 2);
    REQUIRE(rifle.sightView());
    run(rifle, PlayerInput{}, 1);
    CHECK_FALSE(rifle.sightView());  // let go: back to the usual view

    World grenade = duelWith(map, WeaponId::Grenade);
    run(grenade, look, 2);
    CHECK_FALSE(grenade.sightView());
}

TEST_CASE("Looking through the sight the hog turns and aims but does not walk", "[game][sight]") {
    const auto map = flatMap();
    World world = duelWith(map, WeaponId::SniperRifle);
    PlayerInput input;
    input.sight = true;
    input.movement.forward = true;
    input.movement.turnLeft = true;
    input.aimUp = true;
    const float yawBefore = world.hogs()[0].yawRadians;
    run(world, input, 25);
    const Hog& hog = world.hogs()[0];
    CHECK(hog.position.x == Approx(0.0f).margin(0.01f));
    CHECK(hog.yawRadians != Approx(yawBefore));
    CHECK(hog.aimDegrees > 0.0f);

    // The view looks along the line of fire, from the muzzle.
    const auto sight = world.sightView();
    REQUIRE(sight);
    CHECK(glm::length(sight->eye - muzzlePosition(hog)) == Approx(0.0f).margin(0.01f));
    CHECK(glm::dot(sight->direction, aimDirection(hog)) == Approx(1.0f));
}

TEST_CASE("A shot fired through the sight hits what the crosshair is on", "[game][sight]") {
    // Straight ahead at an enemy: the sniper rifle, without spread, hits.
    const auto map = flatMap();
    World world = duelWith(map, WeaponId::SniperRifle);
    PlayerInput aim;
    aim.sight = true;
    run(world, aim, 2);
    aim.fire = true;
    run(world, aim, 1);
    run(world, PlayerInput{}, 10);
    CHECK(world.hogs()[1].health < kMaxHealth);
}

TEST_CASE("The mouse looks around only through the sight", "[game][sight]") {
    // Through the sight the mouse turns the hog and raises its aim (the
    // owner's choice, decision 131); outside it the same movement must do
    // nothing, or every mouse twitch would spoil a careful aim.
    const auto map = flatMap();
    World world = duelWith(map, WeaponId::SniperRifle);
    PlayerInput look;
    look.lookYawDegrees = 10.0f;
    look.lookPitchDegrees = 5.0f;
    const float yawBefore = world.hogs()[0].yawRadians;
    run(world, look, 1);
    CHECK(world.hogs()[0].yawRadians == Approx(yawBefore));
    CHECK(world.hogs()[0].aimDegrees == Approx(0.0f));

    look.sight = true;
    run(world, look, 1);
    CHECK(world.hogs()[0].yawRadians == Approx(yawBefore + glm::radians(10.0f)));
    CHECK(world.hogs()[0].aimDegrees == Approx(5.0f));
}
