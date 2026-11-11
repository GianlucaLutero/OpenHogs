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

} // namespace openhogs::scene
