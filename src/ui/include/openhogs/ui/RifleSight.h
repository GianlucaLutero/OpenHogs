// RifleSight.h
//
// The layout of the original's rifle sight: while the player aims a gun
// through it, the screen is black but for a round hole in the middle,
// with a crosshair at the centre.
//
// warhogs_.exe (docs/formats/hud.md, "Rifle sight") builds the black part
// from the model "hole" of Chars/sight.mad: a ring whose hole has a radius
// of 239 model units, drawn at 0.8 scale on the 640 x 480 screen, 9 pixels
// above the centre (FUN_00457140); FUN_00457eb0 then draws the crosshair,
// the 48 x 48 picture croshair.tim of Language/Tims/facetims.mad, centred
// on the screen. Here the same is worked out in canvas units (480 high,
// centred across, see render::UiRenderer).

#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace openhogs::ui {

inline constexpr float kSightRadius = 239.0f * 0.8f;    // 191.2
inline constexpr float kSightCentreY = 240.0f - 9.0f;   // the hole sits a little high
inline constexpr float kCrosshairSize = 48.0f;

// The centre of the hole on a canvas `canvasWidth` wide.
glm::vec2 sightCentre(float canvasWidth);

// The black around the hole, as triangles (three points each) covering
// the whole canvas except the hole. `segments` sets how round it is.
std::vector<glm::vec2> sightMaskTriangles(float canvasWidth, int segments = 72);

// Top-left corner of the crosshair picture.
glm::vec2 crosshairCorner(float canvasWidth);

} // namespace openhogs::ui
