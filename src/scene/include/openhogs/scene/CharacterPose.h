// CharacterPose.h
//
// Puts a hog's skeleton in the position given by an animation.
//
// A pose is one transform per bone, from bone space to model space. A
// character vertex belongs to one bone and is stored relative to it, so
// its posed position is simply  pose[bone] * vertex.
//
// Bone transforms are built down the hierarchy:
//
//     root:   translate(frame root position) * rotation(root)
//     child:  parent * translate(bone offset) * rotation(child)
//
// where rotation() turns the frame's Euler angles into a matrix with the
// game's convention (see boneRotation).
// Everything stays in the original model space (model units, Y down);
// converting to world space is left to the caller.
//
// Animations are stored at a fixed frame rate. To play them smoothly at
// any display rate, samplePose() blends two neighbouring frames: rotations
// are blended as quaternions (slerp), not as Euler angles, because the
// stored angles jump from +180 to -180 degrees when a bone turns past
// half a turn, and blending those numbers directly would make the bone
// spin the long way round for one frame.

#pragma once

#include "openhogs/formats/MotionCapture.h"
#include "openhogs/formats/Skeleton.h"

#include <glm/glm.hpp>

#include <vector>

namespace openhogs::scene {

using Pose = std::vector<glm::mat4>;  // one bone-to-model transform per bone

// Rotation matrix of one bone for the game's angle convention: the
// transpose of Rz * Ry * Rx, i.e. Rx(-x) * Ry(-y) * Rz(-z). With it a
// positive X angle lowers the left arm (+Z) towards +Y, which is down in
// model space (docs/formats/characters.md explains how this was found).
glm::mat4 boneRotation(const formats::BoneRotation& angles);

// The pose of one stored frame. Throws std::invalid_argument if the frame
// does not have one rotation per bone.
Pose computePose(const formats::Skeleton& skeleton, const formats::AnimationFrame& frame);

// The pose at a fractional frame position, blending the two nearest frames.
// `frame` wraps around (frame N is frame 0 again), so looping animations
// play continuously. Throws std::invalid_argument if the animation is empty.
Pose samplePose(const formats::Skeleton& skeleton, const formats::Animation& animation, float frame);

// Blends two poses bone by bone: `amount` 0 gives `from`, 1 gives `to`.
// Used to cross-fade from one animation into the next over a few frames,
// so a hog does not snap from one stance to another. Throws
// std::invalid_argument if the poses have different bone counts.
Pose blendPoses(const Pose& from, const Pose& to, float amount);

} // namespace openhogs::scene
