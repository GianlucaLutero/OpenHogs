// Tests for the character formats: the .HIR skeleton and the mcap
// animations.
//
// Every hog is assembled from these two files: the skeleton places the
// body parts, the animations move them. A misread field here turns every
// hog into a jumble of limbs, so the tests pin down the record layouts
// verified on the game's files and the checks that keep a damaged file
// from crashing the pose code.

#include "openhogs/core/BinaryReader.h"
#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"

#include "ByteBuilder.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace openhogs::formats;
using openhogs::core::FormatError;
using openhogs::tests::ByteBuilder;
using Catch::Matchers::ContainsSubstring;

namespace {

// One 20-byte HIR record: parent, zero, x, y, z, five zeros.
void addBone(ByteBuilder& file, std::int16_t parent, std::int16_t x, std::int16_t y, std::int16_t z) {
    file.i16(parent).u16(0).i16(x).i16(y).i16(z);
    for (int i = 0; i < 5; ++i) {
        file.u16(0);
    }
}

// One mcap frame for `bones` bones: root position, 5 unknown int32, then
// (x, y, z, 1.0) per bone. Bone b gets angles b/2, b/2 + 0.25, b/2 + 0.125
// (all exact in binary, so the test can compare them exactly).
void addFrame(ByteBuilder& file, std::int32_t rootY, std::size_t bones) {
    file.i32(1).i32(rootY).i32(3);
    for (int i = 0; i < 5; ++i) {
        file.i32(0);
    }
    for (std::size_t b = 0; b < bones; ++b) {
        const float base = static_cast<float>(b) / 2.0f;
        file.f32(base).f32(base + 0.25f).f32(base + 0.125f).f32(1.0f);
    }
}

constexpr std::size_t kFrameSize = 32 + 2 * 16;  // for a 2-bone skeleton

} // namespace

// The parent index and the offset are what put each body part in place;
// the root (bone 0) is its own parent in the game's file.
TEST_CASE("HIR: reads each bone's parent and offset", "[formats][skeleton]") {
    ByteBuilder file;
    addBone(file, 0, 0, 0, 0);
    addBone(file, 0, 0, -21, 1);
    addBone(file, 1, 6, -177, 2);

    const auto skeleton = Skeleton::parse(file.bytes());

    REQUIRE(skeleton.bones.size() == 3);
    CHECK(skeleton.bones[0].parent == 0);
    CHECK(skeleton.bones[2].parent == 1);
    CHECK(skeleton.bones[2].x == 6);
    CHECK(skeleton.bones[2].y == -177);
    CHECK(skeleton.bones[2].z == 2);
}

// The pose code computes bones in file order, so a bone whose parent comes
// later (or a negative index) must be refused instead of reading garbage.
TEST_CASE("HIR: rejects bones whose parent does not come first", "[formats][skeleton]") {
    ByteBuilder forward;
    addBone(forward, 0, 0, 0, 0);
    addBone(forward, 2, 0, 0, 0);
    addBone(forward, 0, 0, 0, 0);
    CHECK_THROWS_WITH(Skeleton::parse(forward.bytes()), ContainsSubstring("bone 1 has parent 2"));

    ByteBuilder truncated;
    addBone(truncated, 0, 0, 0, 0);
    truncated.u16(0);
    CHECK_THROWS_AS(Skeleton::parse(truncated.bytes()), FormatError);
}

// The animation table has no count: it ends where the first animation
// begins. Frames hold the root position and per-bone angles; the fourth
// float of each bone (always 1.0) must be skipped, not read as an angle.
TEST_CASE("mcap: reads animations, root positions and bone angles", "[formats][mcap]") {
    ByteBuilder file;
    const std::uint32_t tableSize = 2 * 8;
    file.u32(tableSize).u32(2 * kFrameSize);
    file.u32(tableSize + 2 * kFrameSize).u32(kFrameSize);
    addFrame(file, -100, 2);
    addFrame(file, -101, 2);
    addFrame(file, -115, 2);

    const auto capture = MotionCapture::parse(file.bytes(), 2);

    REQUIRE(capture.animations.size() == 2);
    REQUIRE(capture.animations[0].frames.size() == 2);
    REQUIRE(capture.animations[1].frames.size() == 1);

    const auto& frame = capture.animations[0].frames[1];
    CHECK(frame.rootPosition == std::array<std::int32_t, 3>{1, -101, 3});
    REQUIRE(frame.bones.size() == 2);
    CHECK(frame.bones[1].x == 0.5f);
    CHECK(frame.bones[1].y == 0.75f);
    CHECK(frame.bones[1].z == 0.625f);
    CHECK(capture.animations[1].frames[0].rootPosition[1] == -115);
}

// A damaged or truncated file must give a clear error, not read past the
// end or produce half frames that would make bones disappear.
TEST_CASE("mcap: rejects animations outside the file or with partial frames", "[formats][mcap]") {
    ByteBuilder outside;
    outside.u32(8).u32(10 * kFrameSize);
    addFrame(outside, -100, 2);
    CHECK_THROWS_WITH(MotionCapture::parse(outside.bytes(), 2), ContainsSubstring("outside the file"));

    ByteBuilder partial;
    partial.u32(8).u32(kFrameSize - 4);
    addFrame(partial, -100, 2);
    CHECK_THROWS_WITH(MotionCapture::parse(partial.bytes(), 2), ContainsSubstring("whole number"));

    ByteBuilder badTable;
    badTable.u32(5).u32(0);
    CHECK_THROWS_AS(MotionCapture::parse(badTable.bytes(), 2), FormatError);
}
