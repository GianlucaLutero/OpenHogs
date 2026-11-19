// Radar.h
//
// The original's "scanner": a small picture of the whole map in the
// bottom-left corner of the screen, tilted like a little model lying in
// front of the camera and turned with it, with a mark for every hog and
// every crate (see docs/formats/hud.md, "Radar").
//
// The executable only places it (FUN_004582f0: the object made from
// Chars/top.mad, at x 110 and 75 above the bottom of the 640 x 480 screen,
// sliding up from 200 below like the other instruments). The renderer
// (_d3d.dll, afInitScanner and FUN_10009810) makes the picture and the
// marks:
//
// - the picture is 64 x 64 pixels, one per tile: the colour of the tile's
//   surface from a table, brighter the higher the tile's corner stands on
//   the map, and plain red for a tile with the mine flag;
// - every hog is a cross (`iconpig` of Language/Tims/mapicons.mtd) in its
//   nation's colour, weapon crates are squares (`iconpkup`), health crates
//   hearts (`iconhart`) and promotion points diamonds (`iconprop`), each
//   tinted with its own colour.
//
// This file holds what can be worked out without drawing: the picture,
// the marks and where things land on the screen. OriginalHud draws it.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/game/TerrainHeightField.h"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace openhogs::game {
class World;
}

namespace openhogs::ui {

// Where the radar's middle sits on the 640 x 480 screen (left-anchored x,
// y from the top): FUN_004582f0 packs x = 110 and y = height - 75.
inline constexpr glm::vec2 kRadarCentre{110.0f, 405.0f};

// The picture of the map: one pixel per tile, row 0 at the map's lowest Z,
// column 0 at its lowest X.
formats::RgbaImage radarMapPicture(const game::TerrainHeightField& terrain);

enum class RadarMarkKind { Hog, WeaponCrate, HealthCrate, PromotionPoint };

struct RadarMark {
    RadarMarkKind kind = RadarMarkKind::Hog;
    glm::vec2 mapPoint{0.0f};  // 0..1 across the map: x along X, y along Z
    glm::vec4 colour{1.0f};
};

// The marks to draw: the hogs still alive and the crates nobody has taken
// (hidden Boot Camp crates are not shown). `activeHog` is drawn white.
std::vector<RadarMark> radarMarks(const game::World& world, std::optional<std::size_t> activeHog);

// The colour of a nation's hogs on the radar (the renderer's table at
// 0x1002BE70): green British, cyan American, blue French, grey German,
// red Russian, yellow Japanese. Team Lard reads past the table's end,
// into the colour of mud.
glm::vec4 radarHogColour(int nation);

// How the map lies on the screen for a camera looking along
// `cameraForward` (its direction on the ground is what counts: the far
// side of the map is the one the camera faces). `centre` is the radar's
// middle in canvas units.
class RadarView {
public:
    RadarView(const glm::vec3& cameraForward, const glm::vec2& centre);

    // A point of the map (0..1 along X and Z) on the screen.
    glm::vec2 project(const glm::vec2& mapPoint) const;

    // The picture's corners for UiRenderer::drawImageQuad: those of its
    // top-left (row 0, column 0), top-right, bottom-right and bottom-left
    // pixels.
    std::array<glm::vec2, 4> pictureCorners() const;

private:
    glm::vec2 forward_{0.0f, -1.0f};  // on the ground (x, z), unit length
    glm::vec2 centre_{0.0f};
};

} // namespace openhogs::ui
