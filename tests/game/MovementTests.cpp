// Tests for moving hogs: animation playback, idle behaviour, collisions
// with the map, walking, jumping, falling, swimming, turns and the world
// that ties them together.
//
// These are the rules the player feels with every key press, so they must
// behave the same on every computer and every run. The tests use flat or
// stepped hand-made terrains, single boxes and hogs placed by hand, with
// speeds and heights that can be worked out on paper.

#include "openhogs/game/AnimationPlayer.h"
#include "openhogs/game/HogMovement.h"
#include "openhogs/game/IdleAnimator.h"
#include "openhogs/game/MapCollision.h"
#include "openhogs/game/TurnSystem.h"
#include "openhogs/game/World.h"

#include "GameTestSupport.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <set>
#include <string>

using namespace openhogs;
using namespace openhogs::game;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

constexpr double kStep = 1.0 / 50.0;  // the game's simulation step

// A hog standing at (x, ground, z), facing +X, in `team`.
Hog makeHog(const MapCollision& map, float x, float z, int team = 0) {
    Hog hog;
    hog.characterClass = hogClassForObject("GR_ME");
    hog.team = team;
    hog.position = {x, map.terrain().heightAt(x, z), z};
    hog.idle.start(hog.animation);
    return hog;
}

MapCollision flatMap(std::vector<CollisionBox> boxes = {}, int height = 0) {
    return MapCollision(flatGround(height), std::move(boxes));
}

CollisionBox makeBox(glm::vec3 centre, glm::vec3 halfSize, float yaw = 0.0f) {
    CollisionBox box;
    box.centre = centre;
    box.halfSize = halfSize;
    box.yawRadians = yaw;
    return box;
}

// Runs `steps` simulation steps of hog 0 with the same input.
MovementEvents runSteps(std::vector<Hog>& hogs, const MapCollision& map, const MovementInput& input, int steps,
                        bool canMove = true) {
    static const auto animations = makeAnimations();
    MovementEvents total;
    for (int i = 0; i < steps; ++i) {
        const auto events = moveHog(hogs, 0, input, canMove, kStep, map, animations);
        total.distanceMoved += events.distanceMoved;
        total.footsteps += events.footsteps;
        total.tookOff = total.tookOff || events.tookOff;
        total.enteredWater = total.enteredWater || events.enteredWater;
        if (events.landed) {
            total.landed = true;
            total.fallHeight = events.fallHeight;
        }
    }
    return total;
}

MovementInput forward() {
    MovementInput input;
    input.forward = true;
    return input;
}

} // namespace

// Looping animations start again after the last frame and say so (that is
// when the idle behaviour picks the next one); "once" animations stop on
// their last frame. Footsteps are timed by frames passed, also across the
// loop's end.
TEST_CASE("AnimationPlayer: loops, plays once, reports frames passed", "[game][animation]") {
    const auto animations = makeAnimations();  // 12 frames each
    AnimationPlayer player;

    player.play(0, AnimationPlayer::Repeat::Loop);
    player.update(10.0 / 25.0, animations);  // 10 frames
    CHECK(player.frame() == Approx(10.0f));
    CHECK_FALSE(player.completedCycle());
    CHECK(player.passedFrame(6.0f));
    player.update(4.0 / 25.0, animations);  // past the end: frame 2
    CHECK(player.frame() == Approx(2.0f));
    CHECK(player.completedCycle());
    CHECK(player.passedFrame(0.0f));
    CHECK(player.passedFrame(11.0f));
    CHECK_FALSE(player.passedFrame(6.0f));

    player.play(8, AnimationPlayer::Repeat::Once);
    player.update(1.0, animations);
    CHECK(player.finished());
    CHECK(player.frame() == Approx(11.0f));
}

// Systems ask for an animation every step; asking again for the one that
// is playing must not restart it, or the hog would freeze on frame 0.
TEST_CASE("AnimationPlayer: asking again for the same animation does not restart it", "[game][animation]") {
    const auto animations = makeAnimations();
    AnimationPlayer player;
    player.play(0, AnimationPlayer::Repeat::Loop);
    player.update(5.0 / 25.0, animations);
    player.play(0, AnimationPlayer::Repeat::Loop);
    CHECK(player.frame() == Approx(5.0f));
    player.play(0, AnimationPlayer::Repeat::Loop, true);
    CHECK(player.frame() == 0.0f);
}

// Changing animation must not snap the body from one stance to the other:
// the cross-fade spreads the change over several steps.
TEST_CASE("AnimationPlayer: changes of animation are cross-faded", "[game][animation]") {
    const auto animations = makeAnimations();
    const auto skeleton = makeSkeleton();
    AnimationPlayer player;
    player.play(kStandingAnimation, AnimationPlayer::Repeat::Loop);
    player.update(1.0, animations);  // no fade left

    const auto hand = [&] { return glm::vec3(player.pose(skeleton, animations)[1][3]); };
    const glm::vec3 standing = hand();
    player.play(kRunAnimation, AnimationPlayer::Repeat::Loop);  // root turned by 1 radian
    float largestMove = 0.0f;
    glm::vec3 before = hand();
    CHECK(glm::length(before - standing) < 0.01f);  // the change starts from where the hog was
    for (int i = 0; i < 25; ++i) {
        player.update(kStep, animations);
        const glm::vec3 now = hand();
        largestMove = std::max(largestMove, glm::length(now - before));
        before = now;
    }
    // Standing and running place the hand 9.6 units apart.
    CHECK(glm::length(before - standing) == Approx(9.59f).margin(0.05));
    CHECK(largestMove < 2.0f);
}

// Idle behaviour must be reproducible (same seed, same sequence), differ
// between hogs, consist mostly of standing, and every fidget must be
// followed by standing again.
TEST_CASE("IdleAnimator: reproducible, mostly standing, fidgets return to standing", "[game][idle]") {
    const auto animations = makeAnimations();
    struct Idler {
        AnimationPlayer player;
        IdleAnimator idle;
    };
    Idler first{{}, IdleAnimator(7)};
    Idler same{{}, IdleAnimator(7)};
    Idler other{{}, IdleAnimator(8)};
    for (auto* idler : {&first, &same, &other}) {
        idler->idle.start(idler->player);
    }

    int standingSteps = 0;
    int differences = 0;
    std::set<std::size_t> played;
    std::size_t previous = first.player.animation();
    const int steps = 50 * 120;  // two minutes
    for (int i = 0; i < steps; ++i) {
        for (auto* idler : {&first, &same, &other}) {
            idler->player.update(kStep, animations);
            idler->idle.update(idler->player, animations);
        }
        REQUIRE(first.player.animation() == same.player.animation());
        REQUIRE(first.player.frame() == same.player.frame());
        differences += first.player.animation() != other.player.animation() ? 1 : 0;
        standingSteps += first.player.animation() == kStandingAnimation ? 1 : 0;

        if (first.player.animation() != previous && previous != kStandingAnimation) {
            REQUIRE(first.player.animation() == kStandingAnimation);
        }
        previous = first.player.animation();
        played.insert(first.player.animation());
    }
    CHECK(standingSteps > steps / 2);
    CHECK(played.size() > 1);  // some fidgets happened
    CHECK(differences > 0);
}

// A hog that stops running (or lands) must go back to standing instead of
// running on the spot.
TEST_CASE("IdleAnimator: any other animation gives way to standing", "[game][idle]") {
    const auto animations = makeAnimations();
    AnimationPlayer player;
    IdleAnimator idle(1);
    player.play(kRunAnimation, AnimationPlayer::Repeat::Loop);
    idle.update(player, animations);
    CHECK(player.animation() == kStandingAnimation);
}

// Only real, visible objects may block hogs: hogs themselves and objects
// without a model are left out, and box sizes (stored in 64-unit steps)
// become half sizes in world units.
TEST_CASE("MapCollision: boxes come from the map's objects", "[game][collision]") {
    auto crate = makeObject("CRATE1", 100, 200, 300);
    crate.boxSizeX = 3;
    crate.boxSizeY = 3;
    crate.boxSizeZ = 4;
    crate.angleY = 1024;
    auto hog = makeObject("GR_ME", 0, 0, 0);
    hog.boxSizeX = hog.boxSizeY = hog.boxSizeZ = 5;
    auto marker = makeObject("DUMMY", 0, 0, 0);
    marker.boxSizeX = marker.boxSizeY = marker.boxSizeZ = 2;
    auto flat = makeObject("CRATE1", 0, 0, 0);  // no box

    const auto boxes =
        collisionBoxes({crate, hog, marker, flat}, [](const std::string& name) { return name == "CRATE1"; });

    REQUIRE(boxes.size() == 1);
    checkClose(boxes[0].centre, {100, 200, 300});
    checkClose(boxes[0].halfSize, {96, 96, 128});
    CHECK(boxes[0].yawRadians == Approx(glm::half_pi<float>()));
}

// A box turns with its object: a long wall turned a quarter turn must block
// along its new direction and leave the old one free.
TEST_CASE("MapCollision: boxes turn with their object", "[game][collision]") {
    // 2000 long along its own X, 64 thick; turned 90 degrees it runs along Z.
    const auto wall = makeBox({0, 0, 0}, {1000, 500, 32}, glm::half_pi<float>());
    CHECK(wall.footprintDistance(0.0f, 900.0f) == 0.0f);
    CHECK(wall.footprintDistance(900.0f, 0.0f) == Approx(868.0f));
}

// Low boxes are floors (bridges, low crates), tall boxes are obstacles,
// and boxes high above (a bridge seen from below) are neither.
TEST_CASE("MapCollision: step onto low boxes, stop at tall ones, pass under high ones", "[game][collision]") {
    const auto map = flatMap({
        makeBox({0, 40, 0}, {200, 40, 200}),        // top at 80: a step
        makeBox({1000, 250, 0}, {100, 250, 100}),   // top at 500: a wall
        makeBox({0, 600, 1000}, {200, 32, 200}),    // 568 to 632: overhead
    });

    CHECK(map.groundHeight(0.0f, 0.0f, 0.0f) == Approx(80.0f));
    CHECK(map.groundHeight(1000.0f, 0.0f, 0.0f) == Approx(0.0f));    // too high to stand on from the ground
    CHECK(map.groundHeight(1000.0f, 0.0f, 500.0f) == Approx(500.0f));  // but fine when arriving from above
    CHECK_FALSE(map.hitsObstacle({0, 0, 0}));
    CHECK(map.hitsObstacle({1000.0f - 150.0f, 0.0f, 0.0f}));        // 50 units from the wall: overlaps
    CHECK_FALSE(map.hitsObstacle({1000.0f - 200.0f, 0.0f, 0.0f}));  // 100 units away: clear
    CHECK_FALSE(map.hitsObstacle({0, 0, 1000}));
}

// The run cycle stays on the spot, so the game moves the hog: at the speed
// measured from the stride, along the direction it faces, with two
// footsteps per cycle of the animation.
TEST_CASE("Movement: running moves the hog along its facing at the running speed", "[game][movement]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0)};
    hogs[0].yawRadians = glm::half_pi<float>();  // facing -Z

    const auto events = runSteps(hogs, map, forward(), 50);  // one second

    CHECK(events.distanceMoved == Approx(kRunSpeed).margin(1.0));
    checkClose(hogs[0].position, {0, 0, -kRunSpeed}, 1.0);
    CHECK(hogs[0].motion.state == MotionState::Running);
    CHECK(hogs[0].animation.animation() == kRunAnimation);
    // 25 frames of a 12-frame cycle: two cycles, four footsteps.
    CHECK(events.footsteps == 4);
}

// Turning works on the spot at a fixed rate, and stays possible when the
// movement allowance is used up (to face a target later); walking and
// jumping then do nothing.
TEST_CASE("Movement: turning on the spot, and no walking without movement left", "[game][movement]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0)};
    MovementInput input;
    input.turnLeft = true;
    input.forward = true;
    input.jump = true;

    runSteps(hogs, map, input, 50, false);

    CHECK(hogs[0].yawRadians == Approx(glm::radians(kTurnDegreesPerSecond)).margin(0.01));
    checkClose(hogs[0].position, {0, 0, 0});
    CHECK(hogs[0].motion.state == MotionState::Turning);
}

// Gentle slopes are followed; a slope steeper than kMaxWalkableSlope stops
// the hog, otherwise it would climb cliffs.
TEST_CASE("Movement: follows gentle ground, stops at a cliff", "[game][movement]") {
    // Rising 1 in 8 up to x = 512, then 1500 units over the next tile
    // (a slope of about 2.8, steeper than the limit).
    const MapCollision map(TerrainHeightField(makeTerrain([](int x, int) {
                               if (x >= 1024) return 64 + 1500;
                               return std::max(0, x) / 8;
                           })),
                           {});
    std::vector<Hog> hogs{makeHog(map, 0, 0)};

    runSteps(hogs, map, forward(), 100);  // two seconds: would reach x = 960

    CHECK(hogs[0].position.x > 500.0f);  // climbed the gentle slope
    CHECK(hogs[0].position.x < 520.0f);  // but stopped where the cliff starts
    CHECK(hogs[0].position.y == Approx(map.terrain().heightAt(hogs[0].position.x, 0.0f)).margin(0.5));
}

// Tiles the map marks as walls (trench walls...) cannot be walked up, even
// when not very steep, but can be walked down, so a hog never gets stuck.
TEST_CASE("Movement: wall tiles cannot be climbed, only descended", "[game][movement]") {
    // A gentle rise (1 in 8) for x >= 0, made of wall tiles.
    auto terrain = makeTerrain([](int x, int) { return std::max(0, x) / 8; });
    for (auto& block : terrain.blocks) {
        for (auto& tile : block.tiles) {
            tile.isWall = block.xOffset >= 0;
        }
    }
    const MapCollision map{TerrainHeightField(terrain), {}};

    std::vector<Hog> climber{makeHog(map, -300, 0)};
    runSteps(climber, map, forward(), 100);
    CHECK(climber[0].position.x <= 0.5f);

    std::vector<Hog> descender{makeHog(map, 1500, 0)};
    descender[0].yawRadians = glm::pi<float>();  // facing -X: downhill
    runSteps(descender, map, forward(), 50);
    CHECK(descender[0].position.x < 1100.0f);
}

// Walls stop hogs; walking at an angle into one slides along it, as the
// player expects, instead of sticking to it.
TEST_CASE("Movement: obstacles stop the hog, which slides along them", "[game][movement]") {
    // A wall along Z at x = 300.
    const auto map = flatMap({makeBox({300, 250, 0}, {32, 250, 1500})});
    std::vector<Hog> straight{makeHog(map, 0, 0)};
    runSteps(straight, map, forward(), 50);
    CHECK(straight[0].position.x < 300.0f - 32.0f - kHogRadius + 1.0f);
    CHECK(straight[0].motion.state == MotionState::Standing);  // not running on the spot

    std::vector<Hog> angled{makeHog(map, 0, 0)};
    angled[0].yawRadians = glm::radians(45.0f);  // towards +X and -Z
    runSteps(angled, map, forward(), 50);
    CHECK(angled[0].position.x < 300.0f - 32.0f - kHogRadius + 1.0f);
    CHECK(angled[0].position.z < -250.0f);  // kept going along the wall
}

// Hogs cannot walk through each other; but hogs that the map places on
// the same spot must be able to step apart instead of being stuck.
TEST_CASE("Movement: hogs block each other but can step apart", "[game][movement]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0), makeHog(map, 300, 0)};
    runSteps(hogs, map, forward(), 50);
    CHECK(hogs[0].position.x < 300.0f - 2.0f * kHogRadius + 1.0f);

    // The other hog is 10 units away along +X: walking towards it stays
    // blocked, walking away (facing -X) is free.
    std::vector<Hog> stacked{makeHog(map, 0, 0), makeHog(map, 10, 0)};
    runSteps(stacked, map, forward(), 50);
    CHECK(stacked[0].position.x == 0.0f);
    stacked[0].yawRadians = glm::pi<float>();
    runSteps(stacked, map, forward(), 50);
    CHECK(stacked[0].position.x < -200.0f);  // walked out of the other hog
}

// A jump rises to v^2 / 2g above the take-off, carries the hog forward
// when it was running, and ends with the landing animation, then standing.
TEST_CASE("Movement: jumping goes up, forward, lands and recovers", "[game][movement]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0)};
    MovementInput jump = forward();
    jump.jump = true;

    const auto animations = makeAnimations();
    auto events = moveHog(hogs, 0, jump, true, kStep, map, animations);
    CHECK(events.tookOff);
    CHECK(hogs[0].motion.state == MotionState::Airborne);

    float highest = 0.0f;
    bool landed = false;
    for (int i = 0; i < 100 && !landed; ++i) {
        events = moveHog(hogs, 0, forward(), true, kStep, map, animations);
        highest = std::max(highest, hogs[0].position.y);
        landed = events.landed;
    }
    REQUIRE(landed);
    const float expectedApex = kJumpUpSpeed * kJumpUpSpeed / (2.0f * kGravity);  // 169 units
    CHECK(highest == Approx(expectedApex).margin(20.0f));
    const float expectedDistance = kJumpForwardSpeed * 2.0f * kJumpUpSpeed / kGravity;  // 360 units
    CHECK(hogs[0].position.x == Approx(expectedDistance).margin(25.0f));
    CHECK(hogs[0].position.y == 0.0f);
    CHECK(hogs[0].motion.state == MotionState::Landing);
    CHECK(hogs[0].animation.animation() == kJumpEndAnimation);

    runSteps(hogs, map, MovementInput{}, 50);
    CHECK(hogs[0].motion.state == MotionState::Standing);
}

// Walking off the edge of a bridge or a cliff makes the hog fall; the
// landing reports how far it fell (for fall damage in a later block).
TEST_CASE("Movement: walking off a ledge falls and reports the fall height", "[game][movement]") {
    // A platform 400 high for x < 200; the ground at 0 after it.
    const auto map = flatMap({makeBox({-800, 200, 0}, {1000, 200, 1000})});
    std::vector<Hog> hogs{makeHog(map, -100, 0)};
    hogs[0].position.y = 400.0f;

    const auto events = runSteps(hogs, map, forward(), 75);

    CHECK(events.landed);
    CHECK(events.fallHeight == Approx(400.0f).margin(5.0f));
    CHECK(hogs[0].position.y == 0.0f);
    CHECK(hogs[0].position.x > 200.0f);
}

// Water tiles make the hog swim: slower, with the swimming animation, a
// splash when it goes in, and no jumping out of the water.
TEST_CASE("Movement: swimming in water is slower and has its own animation", "[game][movement]") {
    auto terrain = makeTerrain([](int, int) { return 0; });
    for (auto& block : terrain.blocks) {
        for (auto& tile : block.tiles) {
            if (block.xOffset >= 0) tile.isWater = true;  // water for x >= 0
        }
    }
    const MapCollision map{TerrainHeightField(terrain), {}};
    std::vector<Hog> hogs{makeHog(map, -100, 0)};

    const auto events = runSteps(hogs, map, forward(), 100);  // two seconds

    CHECK(events.enteredWater);
    CHECK(hogs[0].motion.state == MotionState::Swimming);
    CHECK(hogs[0].animation.animation() == kSwimAnimation);
    // 100 units on land, then at half speed for the rest of the time.
    const float onLand = 100.0f / kRunSpeed;
    CHECK(hogs[0].position.x == Approx((2.0f - onLand) * kRunSpeed * kSwimSpeedFactor).margin(15.0f));

    MovementInput jump;
    jump.jump = true;
    CHECK_FALSE(runSteps(hogs, map, jump, 1).tookOff);
}

// The map has an edge: hogs must not walk off the terrain into the void.
TEST_CASE("Movement: hogs stop at the edge of the map", "[game][movement]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 1900, 0)};
    runSteps(hogs, map, forward(), 100);
    CHECK(hogs[0].position.x <= 2048.0f);
}

// Each surface has its own footstep sound in the game's bank; a wrong name
// would leave the hog silent on that surface.
TEST_CASE("Movement: one footstep sound per surface", "[game][movement]") {
    std::set<std::string_view> names;
    for (int surface = 0; surface <= static_cast<int>(formats::SurfaceType::Lava); ++surface) {
        names.insert(footstepSoundName(static_cast<formats::SurfaceType>(surface)));
    }
    CHECK(names.size() == 12);
    CHECK(footstepSoundName(formats::SurfaceType::Quagmire) == "FT_QUAG");
    CHECK(footstepSoundName(formats::SurfaceType::Snow) == "FT_SNOW");
}

// The first turn goes to the player's team, teams then follow in order,
// and each team uses its hogs in rotation.
TEST_CASE("Turns: teams in order, hogs of each team in rotation", "[game][turns]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 200, 0, 1), makeHog(map, 400, 0, 0),
                          makeHog(map, 600, 0, 1)};
    hogs[3].playerControlled = true;
    TurnSystem turns(hogs);

    CHECK(turns.teams() == std::vector<int>{0, 1});
    CHECK(turns.activeTeam() == 1);
    CHECK(turns.activeHog() == 1u);
    turns.endTurn(hogs);
    CHECK(turns.activeHog() == 0u);
    turns.endTurn(hogs);
    CHECK(turns.activeHog() == 3u);  // team 1's next hog
    turns.endTurn(hogs);
    CHECK(turns.activeHog() == 2u);
    turns.endTurn(hogs);
    CHECK(turns.activeHog() == 1u);  // back to the first
    CHECK(turns.turnNumber() == 5);
}

// A turn ends when its time runs out; walking uses up the movement
// allowance, after which the hog can no longer walk.
TEST_CASE("Turns: time limit and movement allowance", "[game][turns]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 200, 0, 1)};
    TurnSystem turns(hogs, TurnRules{10.0, 1000.0f});

    CHECK_FALSE(turns.update(4.0, 600.0f, hogs));
    CHECK(turns.movementLeft() == Approx(400.0f));
    CHECK(turns.canMove());
    turns.update(1.0, 500.0f, hogs);
    CHECK_FALSE(turns.canMove());
    CHECK(turns.secondsLeft() == Approx(5.0));

    CHECK(turns.update(5.0, 0.0f, hogs));  // time out
    CHECK(turns.activeTeam() == 1);
    CHECK(turns.canMove());                // the new turn has a full allowance
    CHECK(turns.secondsLeft() == Approx(10.0));
}

// The player may pick another hog of the same team, but only before the
// active hog has moved, or a turn could move several hogs.
TEST_CASE("Turns: another hog of the team only before moving", "[game][turns]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 200, 0, 1), makeHog(map, 400, 0, 0)};
    TurnSystem turns(hogs);

    CHECK(turns.switchHog(1, hogs));
    CHECK(turns.activeHog() == 2u);
    CHECK(turns.switchHog(1, hogs));
    CHECK(turns.activeHog() == 0u);  // wraps within the team, skipping team 1

    turns.update(0.1, 10.0f, hogs);
    CHECK_FALSE(turns.switchHog(1, hogs));
    CHECK(turns.activeHog() == 0u);
}

// The world moves only the active hog with the player's input; the others
// keep waiting where they are. Ending the turn hands control over.
TEST_CASE("World: only the active hog follows the input", "[game][world]") {
    const auto map = flatMap();
    std::vector<Hog> hogs{makeHog(map, 0, 0, 0), makeHog(map, 0, 1000, 1)};
    World world(map, hogs);
    const auto animations = makeAnimations();

    for (int i = 0; i < 25; ++i) {
        world.update(kStep, forward(), animations);
    }
    CHECK(world.hogs()[0].position.x == Approx(kRunSpeed / 2.0f).margin(1.0));
    CHECK(world.hogs()[1].position.x == 0.0f);
    CHECK(world.turns()->movementLeft() < world.turns()->rules().movementAllowance);
    CHECK(world.timeSeconds() == Approx(0.5));

    world.endTurn();
    CHECK(world.activeHog() == 1u);
    world.update(kStep, forward(), animations);
    CHECK(world.hogs()[1].position.x > 0.0f);
}

// A map without hogs is only scenery: no turns, nothing to control, and
// updating it must not fail.
TEST_CASE("World: a map without hogs has no turns", "[game][world]") {
    World world(flatMap(), {});
    CHECK_FALSE(world.turns().has_value());
    CHECK_FALSE(world.activeHog().has_value());
    world.update(kStep, forward(), makeAnimations());
    CHECK_FALSE(world.switchHog(1));
    world.endTurn();
}
