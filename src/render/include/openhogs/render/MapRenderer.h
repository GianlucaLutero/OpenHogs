// MapRenderer.h
//
// Draws a scene::MapScene with OpenGL: the sky dome, the terrain, the
// placed objects and markers for logical objects (hog start positions),
// in the level's distance fog.
//
// Construction uploads every texture and mesh to the GPU once; draw() then
// only binds and draws. Create it after the OpenGL context and destroy it
// before the context goes away.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/Fog.h"
#include "openhogs/render/GlObjects.h"
#include "openhogs/scene/MapScene.h"
#include "openhogs/scene/SkyDome.h"

#include <map>
#include <memory>
#include <set>
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

    // Draws `dome` around the camera behind everything else. Without a
    // dome the background is a plain colour (the fog's, if there is fog).
    void setSky(const scene::SkyDome& dome);

    // Fades the terrain and the objects into the fog (none by default).
    void setFog(const Fog& fog) { fog_ = fog; }
    const Fog& fog() const { return fog_; }

    // Stops drawing map object `object` (its index in the .POG): a crate
    // that was picked up.
    void hideObject(std::size_t object) { hidden_.insert(object); }
    // Draws a hidden object again (one revealed by an objective).
    void showObject(std::size_t object) { hidden_.erase(object); }
    // The objects the scene places (to hide those the world keeps hidden).
    const std::vector<scene::ModelInstance>& objects() const { return objects_; }

    // Draws one more model, such as a reward crate that appeared during
    // the match. Ignored if the scene did not load that model (MapScene
    // always loads the crate models the map's archive has).
    void addObject(const scene::ModelInstance& instance);

    // Places map object `object` anew (a vehicle being driven).
    void moveObject(std::size_t object, const glm::mat4& transform);

private:
    // One textured group of triangles on the GPU.
    struct GpuPart {
        std::uint32_t textureIndex;
        GlMesh mesh;
    };

    std::vector<GpuPart> uploadMesh(const scene::Mesh& mesh) const;
    const GlTexture& textureOrFallback(const std::vector<GlTexture>& textures, std::uint32_t index) const;
    void drawParts(const std::vector<GpuPart>& parts, const std::vector<GlTexture>& textures) const;
    void drawSky(const Camera& camera) const;

    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;  // for markers and missing textures

    std::vector<GlTexture> terrainTextures_;
    std::vector<GpuPart> terrain_;

    std::vector<GlTexture> modelTextures_;
    std::map<std::string, std::vector<GpuPart>> models_;
    std::vector<scene::ModelInstance> objects_;
    std::set<std::size_t> hidden_;

    std::vector<GlTexture> skyTextures_;
    std::vector<GpuPart> sky_;
    Fog fog_;

    std::unique_ptr<GlMesh> markerMesh_;
    std::vector<glm::vec3> markers_;
};


} // namespace openhogs::render
