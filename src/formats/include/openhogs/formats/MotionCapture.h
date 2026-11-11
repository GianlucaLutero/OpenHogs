// MotionCapture.h
//
// Reader for Chars/mcap.mad, the animations of the hogs (see
// docs/formats/characters.md). Despite its extension the file is not a
// .MAD archive: it is a table of animations, each a list of frames, and
// each frame gives the position of the skeleton's root and a rotation for
// every bone.
//
// The game has 93 animations, identified by their number. The executable
// names the first 59 (see scene::animationName).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

// Rotation of a bone relative to its parent, as Euler angles in radians.
// How they combine into a rotation (the game uses the transpose of
// Rz * Ry * Rx) is explained in docs/formats/characters.md and implemented
// by scene::boneRotation.
struct BoneRotation {
    float x = 0;
    float y = 0;
    float z = 0;
};

struct AnimationFrame {
    // Position of the root bone (the pelvis), in model units, Y down.
    std::array<std::int32_t, 3> rootPosition{};
    std::vector<BoneRotation> bones;  // one per skeleton bone
};

struct Animation {
    std::vector<AnimationFrame> frames;
};

struct MotionCapture {
    std::vector<Animation> animations;

    // Parses an mcap file for a skeleton of `boneCount` bones (15 for the
    // hogs). Throws core::FormatError if the table points outside the file
    // or an animation is not a whole number of frames.
    static MotionCapture parse(std::span<const std::byte> data, std::size_t boneCount);
};

} // namespace openhogs::formats
