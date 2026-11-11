// Tests for posing characters: bone transforms from the skeleton and an
// animation frame, blending between frames, and posed model meshes.
//
// The rotation order and the blending are the two places where a small
// mistake gives hogs with twisted limbs or arms that spin for a frame, and
// both are hard to spot by eye in a moving animation. These tests fix the
// conventions found on the game data (docs/formats/characters.md) with
// tiny skeletons whose results can be worked out on paper.

#include "openhogs/scene/CharacterPose.h"
#include "openhogs/scene/ModelMesh.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/gtc/constants.hpp>

using namespace openhogs;
using namespace openhogs::scene;
using Catch::Approx;

namespace {

constexpr float kHalfPi = glm::half_pi<float>();

// A two-bone arm: root at the origin, child 10 units along +X.
formats::Skeleton makeArm() {
    formats::Skeleton skeleton;
    skeleton.bones.push_back({0, 0, 0, 0});
    skeleton.bones.push_back({0, 10, 0, 0});
    return skeleton;
}

formats::AnimationFrame makeFrame(formats::BoneRotation root, formats::BoneRotation child,
                                  std::array<std::int32_t, 3> rootPosition = {0, 0, 0}) {
    formats::AnimationFrame frame;
    frame.rootPosition = rootPosition;
    frame.bones = {root, child};
    return frame;
}

glm::vec3 transformPoint(const glm::mat4& m, glm::vec3 p) {
    return glm::vec3(m * glm::vec4(p, 1.0f));
}

void checkClose(glm::vec3 actual, glm::vec3 expected) {
    CHECK(actual.x == Approx(expected.x).margin(1e-4));
    CHECK(actual.y == Approx(expected.y).margin(1e-4));
    CHECK(actual.z == Approx(expected.z).margin(1e-4));
}

} // namespace

// The game's rotation is the transpose of Rz * Ry * Rx (its engine
// multiplies row vectors). This pins both the order and the direction:
// with 90 degrees about X and about Z, +Y goes to +X and +X goes to +Z;
// any other order or the untransposed matrix gives different results.
TEST_CASE("Pose: bone angles follow the game's rotation convention", "[scene][pose]") {
    const glm::mat4 rotation = boneRotation({kHalfPi, 0.0f, kHalfPi});

    checkClose(transformPoint(rotation, {0, 1, 0}), {1, 0, 0});
    checkClose(transformPoint(rotation, {1, 0, 0}), {0, 0, 1});
}

// What the convention means for a hog: the left arm points along +Z when
// all angles are zero, and the idle animations store a positive X angle
// for the shoulder. That angle must lower the arm (towards +Y, which is
// down in model space); the opposite convention makes idle hogs stand with
// their hands up, which is how the convention was found.
TEST_CASE("Pose: a positive X angle lowers the left arm", "[scene][pose]") {
    const glm::mat4 shoulder = boneRotation({0.9f, 0.0f, 0.0f});
    const glm::vec3 hand = transformPoint(shoulder, {0, 0, 100});

    CHECK(hand.y > 70.0f);  // well below the shoulder
    CHECK(hand.z > 0.0f);   // still on the left side
}

// With no rotation, every bone sits at its parent plus its offset, and the
// root follows the frame's root position: this is the skeleton's rest pose.
TEST_CASE("Pose: without rotations bones sit at their offsets", "[scene][pose]") {
    const auto pose = computePose(makeArm(), makeFrame({}, {}, {1, -100, 2}));

    checkClose(transformPoint(pose[0], {0, 0, 0}), {1, -100, 2});
    checkClose(transformPoint(pose[1], {0, 0, 0}), {11, -100, 2});
}

// Turning a parent must carry its children along: an angle of 90 degrees
// about Z on the root swings the child (10 units along X) to 10 units
// along -Y, and the child's own vertices turn with it.
TEST_CASE("Pose: children follow their parent's rotation", "[scene][pose]") {
    const auto pose = computePose(makeArm(), makeFrame({0, 0, kHalfPi}, {}));

    checkClose(transformPoint(pose[1], {0, 0, 0}), {0, -10, 0});
    checkClose(transformPoint(pose[1], {1, 0, 0}), {0, -11, 0});
}

// Halfway between two frames, a bone must be halfway between their
// rotations, and the root halfway between their positions.
TEST_CASE("Pose: sampling between frames blends rotation and position", "[scene][pose]") {
    formats::Animation animation;
    animation.frames = {makeFrame({}, {}, {0, 0, 0}), makeFrame({0, 0, kHalfPi}, {}, {0, -10, 0})};

    const auto pose = samplePose(makeArm(), animation, 0.5f);

    const float s = std::sqrt(0.5f) * 10.0f;  // 10 units at 45 degrees
    checkClose(transformPoint(pose[1], {0, 0, 0}), {s, -s - 5.0f, 0});
}

// The stored angles jump from +179 to -179 degrees when a bone turns past
// half a turn. Blending must go the short way (2 degrees through 180), not
// back through 0, otherwise the bone spins round for one frame.
TEST_CASE("Pose: blending across the +180/-180 wrap takes the short way", "[scene][pose]") {
    const float almostHalfTurn = glm::radians(179.0f);
    formats::Animation animation;
    animation.frames = {makeFrame({0, 0, almostHalfTurn}, {}), makeFrame({0, 0, -almostHalfTurn}, {})};

    const auto pose = samplePose(makeArm(), animation, 0.5f);

    // Halfway is exactly 180 degrees: the child points along -X.
    checkClose(transformPoint(pose[1], {0, 0, 0}), {-10, 0, 0});
}

// Looping animations play by counting frames past the end; sampling must
// wrap around to the start instead of failing or freezing.
TEST_CASE("Pose: frame positions wrap around the animation", "[scene][pose]") {
    formats::Animation animation;
    animation.frames = {makeFrame({}, {}, {0, 0, 0}), makeFrame({}, {}, {0, -10, 0})};

    checkClose(transformPoint(samplePose(makeArm(), animation, 2.0f)[0], {0, 0, 0}), {0, 0, 0});
    checkClose(transformPoint(samplePose(makeArm(), animation, 3.0f)[0], {0, 0, 0}), {0, -10, 0});
    checkClose(transformPoint(samplePose(makeArm(), animation, -1.0f)[0], {0, 0, 0}), {0, -10, 0});
}

// An animation made for another skeleton would index bones that do not
// exist; it must be refused rather than read out of bounds.
TEST_CASE("Pose: frames must have one rotation per bone", "[scene][pose]") {
    formats::AnimationFrame frame;
    frame.bones.resize(3);
    CHECK_THROWS_AS(computePose(makeArm(), frame), std::invalid_argument);

    CHECK_THROWS_AS(samplePose(makeArm(), formats::Animation{}, 0.0f), std::invalid_argument);
}

// Character vertices are stored relative to their bone: posing must move
// each vertex by its own bone's transform (this is what assembles the
// body parts into a hog) and keep texture coordinates untouched.
TEST_CASE("Posed mesh: each vertex moves with its bone", "[scene][pose]") {
    formats::Model model;
    model.vertices = {{1, 0, 0, 0}, {1, 0, 0, 1}, {0, 1, 0, 1}};
    model.faces.push_back({{{0, 0, 0, 0}, {1, 0, 4, 0}, {2, 0, 0, 4}}, 0});

    const auto pose = computePose(makeArm(), makeFrame({}, {0, 0, kHalfPi}));
    const auto mesh = buildPosedModelMesh(model, {{8, 8}}, pose);

    REQUIRE(mesh.size() == 1);
    REQUIRE(mesh[0].vertices.size() == 3);
    checkClose(mesh[0].vertices[0].position, {1, 0, 0});    // root bone: unchanged
    checkClose(mesh[0].vertices[1].position, {10, -1, 0});  // child: turned 90 degrees, then offset
    checkClose(mesh[0].vertices[2].position, {11, 0, 0});
    CHECK(mesh[0].vertices[1].uv.x == Approx(4.5f / 8.0f));
}

// A model made for a bigger skeleton must be refused with an error rather
// than reading a transform that does not exist.
TEST_CASE("Posed mesh: rejects vertices of bones the pose does not have", "[scene][pose]") {
    formats::Model model;
    model.vertices = {{0, 0, 0, 7}};
    const auto pose = computePose(makeArm(), makeFrame({}, {}));
    CHECK_THROWS_AS(buildPosedModelMesh(model, {}, pose), std::invalid_argument);
}

// Cross-fading between animations blends whole poses: halfway between two
// poses every bone must be halfway in rotation and position, and bones
// must stay rigid (a plain average of the matrices would shrink them).
TEST_CASE("Pose: blending two poses keeps bones rigid", "[scene][pose]") {
    const auto straight = computePose(makeArm(), makeFrame({}, {}, {0, 0, 0}));
    const auto turned = computePose(makeArm(), makeFrame({0, 0, kHalfPi}, {}, {0, -20, 0}));

    const auto halfway = blendPoses(straight, turned, 0.5f);

    checkClose(transformPoint(halfway[0], {0, 0, 0}), {0, -10, 0});
    // The root is turned 45 degrees: a point one unit along its X axis
    // stays one unit away from its origin.
    const glm::vec3 tip = transformPoint(halfway[0], {1, 0, 0}) - transformPoint(halfway[0], {0, 0, 0});
    CHECK(glm::length(tip) == Approx(1.0f).margin(1e-4));
    CHECK(tip.x == Approx(std::sqrt(0.5f)).margin(1e-4));

    checkClose(transformPoint(blendPoses(straight, turned, 0.0f)[1], {0, 0, 0}), {10, 0, 0});
    checkClose(transformPoint(blendPoses(straight, turned, 1.0f)[1], {0, 0, 0}), {0, -30, 0});
    CHECK_THROWS_AS(blendPoses(straight, Pose(1), 0.5f), std::invalid_argument);
}
