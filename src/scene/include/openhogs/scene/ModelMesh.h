// ModelMesh.h
//
// Builds the triangles of a 3D model (.VTX/.FAC) ready to draw.
//
// Face texture coordinates are stored in texels, so they are divided by the
// size of the texture each face uses. Quads are split into two triangles.
//
// Character models are made of rigid body parts: every vertex belongs to
// one skeleton bone and is stored relative to it. buildPosedModelMesh()
// moves each vertex (and its normal) with its bone's transform, and shades
// the triangles with a fixed light so the shape of the hog reads well.

#pragma once

#include "openhogs/formats/Model.h"
#include "openhogs/scene/CharacterPose.h"
#include "openhogs/scene/Geometry.h"

#include <vector>

namespace openhogs::scene {

// `textureSizes[i]` is the width and height of texture i of the model's
// .MTD archive. Faces using a texture index outside the list get a 1 x 1
// size, i.e. their UVs are not scaled (the renderer shows a fallback).
// Positions stay in model units; the caller applies the object transform.
Mesh buildModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes);

// Same as buildModelMesh, with every vertex moved by the transform of its
// bone in `pose`. Positions are in model space (model units, Y down).
// Throws std::invalid_argument if a vertex refers to a bone the pose does
// not have.
Mesh buildPosedModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes, const Pose& pose);

// The bones the original renderer (Data/_d3d.dll, afDrawAnimModel) hangs
// separate objects from: the weapon in hand, a hat, a pack on the back.
inline constexpr std::size_t kHandBone = 5;  // what the hog holds (weapons)
inline constexpr std::size_t kHeadBone = 2;  // hats
inline constexpr std::size_t kBackBone = 1;  // the jetpack's rocket

// An object carried by a hog (a weapon, a hat), moved as one rigid piece
// with `bone` of `pose`, as the original renderer draws them: the bones
// stored in the object's vertices are ignored, and the object is turned
// half a turn around the bone's vertical axis (the renderer's objects face
// the other way from the hogs' body parts). Throws std::invalid_argument
// if the pose has no such bone.
Mesh buildCarriedModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes, const Pose& pose,
                           std::size_t bone);

} // namespace openhogs::scene
