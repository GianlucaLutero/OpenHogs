// SkyDome.h
//
// The sky around a map: a dome model with the level's sky pictures on it.
//
// The original (FUN_004866b0) loads the two halves of Chars/skydome.mad,
// "skydome" (the upper half) and "skydomeu" (the lower one, seen below the
// horizon past the edge of the map), and textures both with the four
// pictures of the level's sky archive (Chars/sunny.mad, Chars/night1.mad...,
// which hold .TIM pictures despite their extension). The dome is a
// half-sphere of radius 15778 model units, which the original stretches 16
// times across and 8 times up.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/scene/Geometry.h"

#include <string_view>
#include <vector>

namespace openhogs::scene {

struct SkyDome {
    Mesh mesh;  // both halves, in model units (Y down), textured by index
    std::vector<formats::RgbaImage> textures;
};

// How much the original stretches the dome model (FUN_004866b0: 16.16
// fixed-point scales 0x100000, 0x80000, 0x100000).
inline constexpr float kSkyDomeWidthScale = 16.0f;
inline constexpr float kSkyDomeHeightScale = 8.0f;

// Loads the dome with the pictures of `skyArchive` (such as
// "chars/sunny.mad"). Throws std::runtime_error if a file is missing and
// core::FormatError if one is malformed.
SkyDome loadSkyDome(const core::VirtualFileSystem& vfs, std::string_view skyArchive);

} // namespace openhogs::scene
