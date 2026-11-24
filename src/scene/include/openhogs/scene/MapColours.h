// MapColours.h
//
// How the original recolours a map's textures to give each level its own
// mood: a sunset map is redder, the night map bluer and greyer.
//
// The level's record in the executable (table at 0x4D5210, fields 1 to 4)
// holds four settings, and the renderer (Data/_d3d.dll: afLoadTims for the
// objects' .MTD, afAddToGroundTextures2 for the terrain's .PTG) applies
// them to the textures' palettes as they are loaded, in this order:
//
//   1. desaturation: each channel moves `desaturation` steps (out of 31)
//      towards the colour's grey; 5 means "all grey";
//   2. brightness: `brightness` plus the renderer's own +3 is added to
//      each channel;
//   3. tint: the colour is mixed, `tintPercent` per cent, with its grey
//      tinted by `tint` (each channel (tint + 8) * grey / 16).
//
// Channels are the PlayStation's 5-bit ones (0..31). A colour that would
// end up black (0x0000, the transparent colour) gets red 1 instead.
// The objects' textures only have their first 16 palette colours changed:
// afLoadTims stops after 32 bytes of palette.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace openhogs::scene {

struct PaletteGrading {
    int desaturation = 0;   // field 1
    int brightness = 0;     // field 2, before the renderer's +3
    int tintPercent = 0;    // field 3
    std::array<std::uint8_t, 3> tint{0, 0, 0};  // field 4: red, green, blue
};

// What _d3d.dll adds to every channel besides the level's brightness
// (its global at 0x10022058), and the base of the tint (0x1002205C).
inline constexpr int kRendererBrightness = 3;
inline constexpr int kTintBase = 8;

// The objects' textures (.MTD): only this many palette colours are graded.
inline constexpr std::size_t kGradedObjectColours = 16;

// One 16-bit PlayStation colour graded; bit 15 (semi-transparency) is kept
// and the transparent colour 0x0000 stays as it is.
std::uint16_t gradeColour(std::uint16_t colour, const PaletteGrading& grading);

} // namespace openhogs::scene
