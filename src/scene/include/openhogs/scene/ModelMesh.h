// ModelMesh.h
//
// Builds the triangles of a 3D model (.VTX/.FAC) ready to draw.
//
// Face texture coordinates are stored in texels, so they are divided by the
// size of the texture each face uses. Quads are split into two triangles.

#pragma once

#include "openhogs/formats/Model.h"
#include "openhogs/scene/Geometry.h"

#include <vector>

namespace openhogs::scene {

// `textureSizes[i]` is the width and height of texture i of the model's
// .MTD archive. Faces using a texture index outside the list get a 1 x 1
// size, i.e. their UVs are not scaled (the renderer shows a fallback).
// Positions stay in model units; the caller applies the object transform.
Mesh buildModelMesh(const formats::Model& model, const std::vector<glm::uvec2>& textureSizes);

} // namespace openhogs::scene
