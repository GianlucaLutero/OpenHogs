// Tests for the radar (ui/Radar.h). The radar is how a player finds the
// enemy and the crates beyond the camera's view: its picture must show the
// land as the original does (water, mines and high ground recognisable),
// every hog and crate still in play must be marked where it is, and the
// map must lie the way the camera looks, or the player walks the wrong way.

#include "openhogs/ui/Radar.h"

#include "openhogs/game/World.h"

#include "../game/GameTestSupport.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace openhogs;
using namespace openhogs::ui;
using namespace openhogs::tests;
using Catch::Approx;

namespace {

glm::ivec3 pixel(const formats::RgbaImage& picture, int column, int row) {
    const auto index = (static_cast<std::size_t>(row) * picture.width + static_cast<std::size_t>(column)) * 4;
    return {picture.pixels[index], picture.pixels[index + 1], picture.pixels[index + 2]};
}

} // namespace

TEST_CASE("The radar picture has one pixel per tile, coloured like the original", "[ui][radar]") {
    // The terrain rises along X; one tile is water, one has the mine flag.
    auto terrain = makeTerrain([](int x, int) { return (x + 2048) / 4; });
    terrain.blocks[0].tiles[5].surface = formats::SurfaceType::Water;  // column 1, row 1 of the block
    terrain.blocks[0].tiles[6].surface = formats::SurfaceType::Grass;
    terrain.blocks[0].tiles[6].isMine = true;
    const game::TerrainHeightField field(terrain);
    const auto picture = radarMapPicture(field);

    REQUIRE(picture.width == 8);
    REQUIRE(picture.height == 8);
    // Higher ground is brighter (the mud colour scaled by the height).
    CHECK(pixel(picture, 6, 7).r > pixel(picture, 3, 7).r);
    // Mines are plain red, whatever the surface; water is bluish. Pixel
    // (column, row) is the tile at X, Z = (column, row) x 512 from the
    // map's corner.
    int mines = 0, waters = 0;
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            const auto& tile = field.tileAt(field.minX() + 256.0f + 512.0f * static_cast<float>(column),
                                            field.minZ() + 256.0f + 512.0f * static_cast<float>(row));
            const auto colour = pixel(picture, column, row);
            if (tile.isMine) {
                ++mines;
                CHECK(colour == glm::ivec3(255, 0, 0));
            } else if (tile.surface == formats::SurfaceType::Water) {
                ++waters;
                CHECK(colour.b > colour.r);
                CHECK(colour.b > colour.g);
            }
        }
    }
    CHECK(mines == 1);
    CHECK(waters == 1);
}

TEST_CASE("The radar marks living hogs and crates still lying on the map", "[ui][radar]") {
    // A dead hog or a crate already taken must not lure anyone; the hog in
    // play stands out in white, the others wear their nation's colour.
    const auto map = flatMap();
    std::vector<game::Hog> hogs{makeHog(map, -2048, -2048, 0), makeHog(map, 1024, 0, 1), makeHog(map, 0, 0, 1)};
    hogs[2].alive = false;
    std::vector<game::Pickup> pickups(3);
    pickups[0].kind = game::PickupKind::Health;
    pickups[0].centre = {2048.0f, 0.0f, 2048.0f};
    pickups[1].kind = game::PickupKind::Weapon;
    pickups[1].taken = true;
    pickups[2].kind = game::PickupKind::PromotionPoints;
    pickups[2].hidden = true;  // a Boot Camp crate not revealed yet
    const game::World world(map, hogs, {}, pickups);

    const auto marks = radarMarks(world, 0);
    REQUIRE(marks.size() == 3);
    CHECK(marks[0].kind == RadarMarkKind::HealthCrate);
    CHECK(marks[0].mapPoint.x == Approx(1.0f));
    CHECK(marks[0].mapPoint.y == Approx(1.0f));
    CHECK(marks[1].kind == RadarMarkKind::Hog);
    CHECK(marks[1].mapPoint.x == Approx(0.0f));
    CHECK(marks[1].colour == glm::vec4(1.0f));
    CHECK(marks[2].mapPoint.x == Approx(0.75f));
    CHECK(marks[2].mapPoint.y == Approx(0.5f));
    CHECK(marks[2].colour == radarHogColour(world.nationOf(1)));
}

TEST_CASE("The radar's far side is the one the camera faces", "[ui][radar]") {
    // Looking along -Z, points of lower Z are ahead: they must be higher on
    // the screen and drawn smaller (seen further away); turning the camera
    // a quarter to +X brings the high-X side up. The middle of the map stays
    // on the radar's centre.
    const glm::vec2 centre = kRadarCentre;
    const RadarView north({0.0f, -0.3f, -1.0f}, centre);
    CHECK(north.project({0.5f, 0.5f}).x == Approx(centre.x));
    CHECK(north.project({0.5f, 0.5f}).y == Approx(centre.y));
    const glm::vec2 farPoint = north.project({0.5f, 0.0f});
    const glm::vec2 nearPoint = north.project({0.5f, 1.0f});
    CHECK(farPoint.y < centre.y);
    CHECK(nearPoint.y > centre.y);
    const auto corners = north.pictureCorners();
    CHECK(corners[1].x - corners[0].x < corners[2].x - corners[3].x);  // far edge narrower

    const RadarView east({1.0f, 0.0f, 0.0f}, centre);
    CHECK(east.project({1.0f, 0.5f}).y < centre.y);
    CHECK(east.project({0.5f, 0.0f}).x < centre.x);  // low Z is on the left when facing +X
}
