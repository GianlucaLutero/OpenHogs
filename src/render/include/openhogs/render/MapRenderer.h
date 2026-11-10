// MapRenderer.h
//
// Draws a scene::MapScene with OpenGL: the terrain, the placed objects
// and markers for logical objects (hog start positions).
//
// Construction uploads every texture and mesh to the GPU once; draw() then
// only binds and draws. Create it after the OpenGL context and destroy it
// before the context goes away.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/GlObjects.h"
#include "openhogs/scene/MapScene.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace openhogs::render {

struct DrawOptions {
    bool showObjects = true;
    bool showMarkers = true;
};

class MapRenderer {
public:
    explicit MapRenderer(const scene::MapScene& scene);

    // Draws the map into the current framebuffer of `width` x `height` pixels.
    void draw(const Camera& camera, int width, int height, const DrawOptions& options) const;

private:
    // One textured group of triangles on the GPU.
    struct GpuPart {
        std::uint32_t textureIndex;
        GlMesh mesh;
    };

    std::vector<GpuPart> uploadMesh(const scene::Mesh& mesh) const;
    const GlTexture& textureOrFallback(const std::vector<GlTexture>& textures, std::uint32_t index) const;
    void drawParts(const std::vector<GpuPart>& parts, const std::vector<GlTexture>& textures) const;

    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;  // for markers and missing textures

    std::vector<GlTexture> terrainTextures_;
    std::vector<GpuPart> terrain_;

    std::vector<GlTexture> modelTextures_;
    std::map<std::string, std::vector<GpuPart>> models_;
    std::vector<scene::ModelInstance> objects_;

    std::unique_ptr<GlMesh> markerMesh_;
    std::vector<glm::vec3> markers_;
};

// Reads back the current framebuffer as an image (top row first), e.g. to
// save a screenshot.
formats::RgbaImage captureFramebuffer(int width, int height);

} // namespace openhogs::render
