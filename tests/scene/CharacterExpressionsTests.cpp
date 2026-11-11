// Tests for facial expressions: the rules per animation, their override
// file, blinking and the texture swap on the posed mesh.
//
// Expressions only swap two textures, so a mistake shows up as the wrong
// face (or a body part drawn with an eye texture). The tests protect the
// swap itself, the balaclava variants of the saboteur, the user's ability
// to correct the rules from a content pack, and a blink that is brief,
// regular and not shared by every hog at the same moment.

#include "openhogs/core/BinaryReader.h"
#include "openhogs/scene/CharacterExpressions.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs;
using namespace openhogs::scene;

namespace {

// Texture list of a hog: 0 = uniform, 1 = default eyes, 2 = default mouth,
// then eyes001..007 at 10..16 and gobs001..007 at 20..26.
FaceTextureSlots makeSlots() {
    FaceTextureSlots slots;
    slots.defaultEyes = 1;
    slots.defaultMouth = 2;
    for (std::uint32_t n = 1; n <= 7; ++n) {
        slots.eyes[n] = 9 + n;
        slots.mouths[n] = 19 + n;
    }
    return slots;
}

Mesh makeHeadMesh() {
    Mesh mesh;
    mesh.push_back({0, {}});  // uniform
    mesh.push_back({1, {}});  // eyes
    mesh.push_back({2, {}});  // mouth
    return mesh;
}

std::vector<std::uint32_t> texturesOf(const Mesh& mesh) {
    std::vector<std::uint32_t> textures;
    for (const auto& part : mesh) {
        textures.push_back(part.textureIndex);
    }
    return textures;
}

} // namespace

// An expression must change exactly the eye and mouth textures and leave
// the rest of the hog alone. Angry uses eyes005 and gobs005.
TEST_CASE("Expressions: only the eyes and mouth change texture", "[scene][expressions]") {
    auto mesh = makeHeadMesh();
    applyExpression(mesh, makeSlots(), Expression::Angry, false, false);
    CHECK(texturesOf(mesh) == std::vector<std::uint32_t>{0, 14, 24});
}

// Blinking closes the eyes (eyes002) whatever the expression, and keeps
// the expression's mouth.
TEST_CASE("Expressions: blinking closes the eyes and keeps the mouth", "[scene][expressions]") {
    auto mesh = makeHeadMesh();
    applyExpression(mesh, makeSlots(), Expression::Happy, true, false);
    CHECK(texturesOf(mesh) == std::vector<std::uint32_t>{0, 11, 20});
}

// The saboteur's face is a black balaclava: it must use the variants drawn
// on black (eyes006/007, gobs006/007), never the skin-coloured ones.
TEST_CASE("Expressions: the saboteur uses the balaclava faces", "[scene][expressions]") {
    auto open = makeHeadMesh();
    applyExpression(open, makeSlots(), Expression::Angry, false, true);
    CHECK(texturesOf(open) == std::vector<std::uint32_t>{0, 15, 26});

    auto blinking = makeHeadMesh();
    applyExpression(blinking, makeSlots(), Expression::Happy, true, true);
    CHECK(texturesOf(blinking) == std::vector<std::uint32_t>{0, 16, 25});
}

// "Gormless" is the team archive's own default face, and a missing face
// texture (e.g. a pack without FACES.MTD) falls back to the default face
// instead of drawing the uniform texture on the eyes.
TEST_CASE("Expressions: default face for gormless and for missing textures", "[scene][expressions]") {
    auto gormless = makeHeadMesh();
    applyExpression(gormless, makeSlots(), Expression::Gormless, false, false);
    CHECK(texturesOf(gormless) == std::vector<std::uint32_t>{0, 1, 2});

    FaceTextureSlots noFaces;
    noFaces.defaultEyes = 1;
    noFaces.defaultMouth = 2;
    auto missing = makeHeadMesh();
    applyExpression(missing, noFaces, Expression::Shocked, true, false);
    CHECK(texturesOf(missing) == std::vector<std::uint32_t>{0, 1, 2});

    auto noDefaults = makeHeadMesh();
    applyExpression(noDefaults, FaceTextureSlots{}, Expression::Shocked, false, false);
    CHECK(texturesOf(noDefaults) == std::vector<std::uint32_t>{0, 1, 2});
}

// The built-in rules follow the animation names: celebrating is happy,
// aiming angry, dying has closed eyes; animations past the list are neutral.
TEST_CASE("Expressions: built-in rules per animation", "[scene][expressions]") {
    const auto rules = ExpressionRules::defaults();
    CHECK(rules.forAnimation(0) == Expression::Neutral);   // run cycle
    CHECK(rules.forAnimation(24) == Expression::Angry);    // aiming rifle
    CHECK(rules.forAnimation(41) == Expression::Happy);    // celebration
    CHECK(rules.forAnimation(47) == Expression::Dying);    // dying
    CHECK(rules.forAnimation(32) == Expression::Gormless); // looking gormless
    CHECK(rules.forAnimation(500) == Expression::Neutral);
}

// The rules are a guess until the game's own logic is known, so users
// must be able to correct them; mistakes in the file must be reported.
TEST_CASE("Expressions: rules can be overridden, errors are reported", "[scene][expressions]") {
    auto rules = ExpressionRules::defaults();
    rules.applyOverrides(core::IniFile::parse("[Animations]\n0 = Happy\n70 = sad\n"));
    CHECK(rules.forAnimation(0) == Expression::Happy);
    CHECK(rules.forAnimation(70) == Expression::Sad);
    CHECK(rules.forAnimation(24) == Expression::Angry);  // untouched

    CHECK_THROWS_AS(rules.applyOverrides(core::IniFile::parse("[Animations]\n3 = grumpy\n")), core::FormatError);
    CHECK_THROWS_AS(rules.applyOverrides(core::IniFile::parse("[Animations]\nrun = happy\n")), core::FormatError);
    CHECK(parseExpression("eyes-closed") == Expression::EyesClosed);
}

// A blink must be short (eyes mostly open), happen regularly, and differ
// between hogs so a team does not blink in unison.
TEST_CASE("Expressions: blinking is brief, regular and differs between hogs", "[scene][expressions]") {
    constexpr double kStep = 0.01;
    constexpr double kMinutes = 2.0;
    int closedSteps = 0;
    int blinks = 0;
    int sameAsOtherHog = 0;
    bool wasClosed = false;
    for (double t = 0.0; t < kMinutes * 60.0; t += kStep) {
        const bool closed = isBlinking(t, 1);
        closedSteps += closed ? 1 : 0;
        blinks += (closed && !wasClosed) ? 1 : 0;
        wasClosed = closed;
        sameAsOtherHog += (closed && isBlinking(t, 2)) ? 1 : 0;
    }
    // About one blink every 5.5 seconds: about 22 in two minutes.
    CHECK(blinks >= 18);
    CHECK(blinks <= 23);
    CHECK(closedSteps * kStep < 0.05 * kMinutes * 60.0);  // eyes closed under 5% of the time
    CHECK(sameAsOtherHog < closedSteps / 2);
    CHECK_FALSE(isBlinking(-1.0, 1));
}
