// Skeleton.h
//
// Reader for Chars/pig.HIR, the skeleton ("hierarchy") shared by every hog
// (see docs/formats/characters.md).
//
// The skeleton is a list of bones. Each bone hangs from a parent bone and
// sits at a fixed offset from it, in model units with Y pointing down:
//
//     0 pelvis ── 1 spine ── 2 head
//          │          ├── 3 left shoulder ── 4 left elbow ── 5 left hand
//          │          └── 6 right shoulder ── 7 right elbow ── 8 right hand
//          ├── 9 left hip ── 10 left knee ── 11 left foot
//          └── 12 right hip ── 13 right knee ── 14 right foot
//
// The vertices of a character model are stored relative to the bone they
// belong to, so the skeleton is what assembles the model into a hog.
// (Left and right were named from the hog's point of view by looking at
// the models; nothing in the file names them.)

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace openhogs::formats {

struct SkeletonBone {
    std::size_t parent = 0;  // index of the parent bone; the root is its own parent
    std::int16_t x = 0;      // offset from the parent bone (model units)
    std::int16_t y = 0;
    std::int16_t z = 0;
};

struct Skeleton {
    std::vector<SkeletonBone> bones;

    // Parses a .HIR file. Throws core::FormatError if the size is wrong or
    // a bone hangs from a bone that comes after it (which would make the
    // hierarchy impossible to compute in one pass).
    static Skeleton parse(std::span<const std::byte> data);
};

} // namespace openhogs::formats
