// Skeleton.cpp - see Skeleton.h for an overview.

#include "openhogs/formats/Skeleton.h"

#include "openhogs/core/BinaryReader.h"

#include <string>

namespace openhogs::formats {

namespace {

// int16 parent, int16 (always 0), int16 x, y, z, then five int16 that are
// always zero in the game's file.
constexpr std::size_t kBoneRecordSize = 20;

} // namespace

Skeleton Skeleton::parse(std::span<const std::byte> data) {
    if (data.empty() || data.size() % kBoneRecordSize != 0) {
        throw core::FormatError("HIR skeleton: size " + std::to_string(data.size()) +
                                " is not a positive multiple of " + std::to_string(kBoneRecordSize));
    }

    core::BinaryReader reader(data);
    Skeleton skeleton;
    const std::size_t boneCount = data.size() / kBoneRecordSize;

    for (std::size_t index = 0; index < boneCount; ++index) {
        const std::int16_t parent = reader.readI16();
        reader.skip(2);

        SkeletonBone bone;
        bone.x = reader.readI16();
        bone.y = reader.readI16();
        bone.z = reader.readI16();
        reader.skip(10);

        // Parents always come first (the root is 0 and is its own parent).
        // Checking it here lets the pose code compute every bone from
        // already-computed parents in a single loop.
        const bool isRoot = index == 0 && parent == 0;
        if (!isRoot && (parent < 0 || static_cast<std::size_t>(parent) >= index)) {
            throw core::FormatError("HIR skeleton: bone " + std::to_string(index) + " has parent " +
                                    std::to_string(parent) + ", which does not come before it");
        }
        bone.parent = static_cast<std::size_t>(parent);
        skeleton.bones.push_back(bone);
    }
    return skeleton;
}

} // namespace openhogs::formats
