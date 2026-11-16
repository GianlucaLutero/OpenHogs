// Tests for the rifle sight's layout (ui/RifleSight.h). They protect what
// the player relies on when aiming through it: the view stays open inside
// the round hole and is black everywhere else, on any screen width, and
// the crosshair marks the middle of the screen, where the shot goes.

#include "openhogs/ui/RifleSight.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace openhogs::ui;
using Catch::Approx;

namespace {

bool insideTriangle(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
    const auto side = [](glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };
    const float d1 = side(p, a, b), d2 = side(p, b, c), d3 = side(p, c, a);
    const bool negative = d1 < 0 || d2 < 0 || d3 < 0;
    const bool positive = d1 > 0 || d2 > 0 || d3 > 0;
    return !(negative && positive);
}

bool covered(const std::vector<glm::vec2>& triangles, glm::vec2 point) {
    for (std::size_t i = 0; i + 2 < triangles.size(); i += 3) {
        if (insideTriangle(point, triangles[i], triangles[i + 1], triangles[i + 2])) return true;
    }
    return false;
}

} // namespace

TEST_CASE("The rifle sight is black outside its hole and open inside", "[ui][sight]") {
    for (const float width : {640.0f, 853.0f}) {
        INFO(width);
        const auto mask = sightMaskTriangles(width);
        const glm::vec2 centre = sightCentre(width);
        CHECK_FALSE(covered(mask, centre));
        CHECK_FALSE(covered(mask, centre + glm::vec2(kSightRadius * 0.9f, 0.0f)));
        CHECK(covered(mask, centre + glm::vec2(kSightRadius * 1.1f, 0.0f)));
        // Every corner of the screen is black.
        for (const glm::vec2 corner : {glm::vec2(1, 1), glm::vec2(width - 1, 1), glm::vec2(1, 479),
                                       glm::vec2(width - 1, 479)}) {
            CHECK(covered(mask, corner));
        }
    }
}

TEST_CASE("The crosshair sits on the middle of the screen", "[ui][sight]") {
    const glm::vec2 corner = crosshairCorner(853.0f);
    CHECK(corner.x + kCrosshairSize / 2.0f == Approx(853.0f / 2.0f));
    CHECK(corner.y + kCrosshairSize / 2.0f == Approx(240.0f));
    CHECK(kSightRadius == Approx(191.2f));  // the hole model's 239 units at 0.8 scale
}
