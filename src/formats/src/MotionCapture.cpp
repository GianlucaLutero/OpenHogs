// MotionCapture.cpp - see MotionCapture.h for an overview.

#include "openhogs/formats/MotionCapture.h"

#include "openhogs/core/BinaryReader.h"

#include <string>

namespace openhogs::formats {

namespace {

// Each table entry: uint32 offset, uint32 size (in bytes).
constexpr std::size_t kTableEntrySize = 8;

// Frame header: int32 x, y, z of the root, then five int32 whose meaning
// is unknown (mostly zero; see docs/formats/characters.md).
constexpr std::size_t kFrameHeaderSize = 32;

// Per bone: float x, y, z angles and a fourth float that is always 1.0.
constexpr std::size_t kBoneRecordSize = 16;

std::string animationLabel(std::size_t index) {
    return "mcap animation " + std::to_string(index);
}

AnimationFrame readFrame(core::BinaryReader& reader, std::size_t boneCount) {
    AnimationFrame frame;
    for (auto& coordinate : frame.rootPosition) {
        coordinate = reader.readI32();
    }
    reader.skip(kFrameHeaderSize - 12);

    frame.bones.resize(boneCount);
    for (auto& bone : frame.bones) {
        bone.x = reader.readF32();
        bone.y = reader.readF32();
        bone.z = reader.readF32();
        reader.skip(4);
    }
    return frame;
}

} // namespace

MotionCapture MotionCapture::parse(std::span<const std::byte> data, std::size_t boneCount) {
    core::BinaryReader reader(data);
    const std::size_t frameSize = kFrameHeaderSize + boneCount * kBoneRecordSize;

    // The table has no count: it ends where the first animation starts.
    const std::size_t tableSize = reader.readU32();
    if (tableSize == 0 || tableSize % kTableEntrySize != 0 || tableSize > data.size()) {
        throw core::FormatError("mcap: invalid animation table size " + std::to_string(tableSize));
    }

    MotionCapture result;
    const std::size_t animationCount = tableSize / kTableEntrySize;
    for (std::size_t index = 0; index < animationCount; ++index) {
        reader.seek(index * kTableEntrySize);
        const std::size_t offset = reader.readU32();
        const std::size_t size = reader.readU32();

        if (offset < tableSize || offset > data.size() || size > data.size() - offset) {
            throw core::FormatError(animationLabel(index) + " lies outside the file");
        }
        if (size == 0 || size % frameSize != 0) {
            throw core::FormatError(animationLabel(index) + ": size " + std::to_string(size) +
                                    " is not a whole number of " + std::to_string(frameSize) +
                                    "-byte frames");
        }

        Animation animation;
        reader.seek(offset);
        for (std::size_t frame = 0; frame < size / frameSize; ++frame) {
            animation.frames.push_back(readFrame(reader, boneCount));
        }
        result.animations.push_back(std::move(animation));
    }
    return result;
}

} // namespace openhogs::formats
