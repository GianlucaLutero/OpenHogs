// CharacterRenderer.h
//
// Draws one posed hog on a ground grid, for the character viewer.
//
// The posed mesh changes every frame (the animation moves it), so it is
// uploaded again on each draw. A hog has under 1,000 triangles, which is
// far less work than the time a frame takes, so no GPU skinning is needed.
//
// The mesh comes in model space (model units, Y down). The renderer turns
// it into world space with the same convention as map objects: scale by
// formats::kModelToWorldScale and mirror Y so the hog stands up.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/GlObjects.h"
#include "openhogs/scene/Geometry.h"

#include <memory>
#include <vector>

namespace openhogs::render {

class CharacterRenderer {
public:
    // Needs a current OpenGL context.
    CharacterRenderer();

    // Uploads the textures the mesh's texture indices refer to (a team's
    // .mtd). Can be called again to switch team.
    void setTextures(const std::vector<formats::RgbaImage>& textures);

    // Draws the hog with its feet `feetDepth` model units below its origin
    // standing on the ground grid, which lies at world height 0.
    void draw(const Camera& camera, int width, int height, const scene::Mesh& posedMesh, float feetDepth) const;

private:
    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;  // for the grid and missing textures
    std::unique_ptr<GlMesh> grid_;
    std::vector<GlTexture> textures_;
};

} // namespace openhogs::render
