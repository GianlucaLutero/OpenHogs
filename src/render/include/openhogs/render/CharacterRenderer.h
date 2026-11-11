// CharacterRenderer.h
//
// Draws posed hogs: one on a ground grid for the character viewer
// (draw), or any number on top of a map for the game (drawHog).
//
// The posed mesh changes every frame (the animation moves it), so it is
// uploaded again on each draw. A hog has under 1,000 triangles, which is
// far less work than the time a frame takes, so no GPU skinning is needed.
//
// The mesh comes in model space (model units, Y down). draw() turns it
// into world space with the same convention as map objects: scale by
// formats::kModelToWorldScale and mirror Y so the hog stands up. drawHog()
// takes the model-to-world matrix from the caller.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/GlObjects.h"
#include "openhogs/scene/Geometry.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace openhogs::render {

// The textures of one team on the GPU, in the order mesh texture indices
// count. Needs a current OpenGL context.
class CharacterTextureSet {
public:
    explicit CharacterTextureSet(const std::vector<formats::RgbaImage>& images);

    // nullptr if the set has no such texture.
    const GlTexture* find(std::uint32_t index) const;

private:
    std::vector<GlTexture> textures_;
};

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

    // Draws one hog into the frame as it is (no clearing, no grid), placed
    // by `modelToWorld`. Used after the map has been drawn.
    void drawHog(const Camera& camera, int width, int height, const scene::Mesh& posedMesh,
                 const CharacterTextureSet& textures, const glm::mat4& modelToWorld) const;

private:
    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;  // for the grid and missing textures
    std::unique_ptr<GlMesh> grid_;
    std::unique_ptr<CharacterTextureSet> textures_;
};

} // namespace openhogs::render
