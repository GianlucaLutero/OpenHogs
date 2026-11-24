// MapRenderer.cpp - see MapRenderer.h for an overview.

#include "openhogs/render/MapRenderer.h"

#include "BasicShader.h"

#include <glm/gtc/matrix_transform.hpp>


namespace openhogs::render {

namespace {

// Colour behind the map when there is neither a sky dome nor fog.
constexpr glm::vec4 kSkyColour{0.45f, 0.62f, 0.85f, 1.0f};

// Radius of the drawn sky dome, in world units. The original stretches its
// dome (radius 15778 model units) 16 times across and 8 times up, far
// beyond the camera's far plane here: the dome is shrunk to fit inside it,
// keeping the original's flattening (half as high as wide). Since it is
// centred on the camera and drawn behind everything, only its shape shows.
constexpr float kSkyRadius = 90000.0f;
constexpr float kSkyModelRadius = 15778.0f;
constexpr float kSkyFlattening = scene::kSkyDomeHeightScale / scene::kSkyDomeWidthScale;
constexpr glm::vec4 kNoTint{1.0f};
constexpr glm::vec4 kMarkerColour{1.0f, 0.85f, 0.1f, 1.0f};

// A small diamond (two pyramids) standing on its tip, 300 units tall, used
// to show positions that have no model.
std::vector<scene::Vertex> makeMarkerVertices() {
    const float r = 60.0f;
    const glm::vec3 bottom{0, 0, 0}, top{0, 300, 0};
    const glm::vec3 ring[4] = {{r, 150, 0}, {0, 150, r}, {-r, 150, 0}, {0, 150, -r}};

    std::vector<scene::Vertex> vertices;
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 a = ring[i];
        const glm::vec3 b = ring[(i + 1) % 4];
        for (const glm::vec3& p : {bottom, a, b, top, b, a}) {
            vertices.push_back({p, {0.5f, 0.5f}, 1.0f});
        }
    }
    return vertices;
}

} // namespace

MapRenderer::MapRenderer(const scene::MapScene& scene) {
    shader_ = std::make_unique<GlShaderProgram>(kBasicVertexShader, kBasicFragmentShader);
    whiteTexture_ = std::make_unique<GlTexture>(formats::RgbaImage{1, 1, {255, 255, 255, 255}});

    for (const auto& image : scene.terrainTextures) {
        terrainTextures_.emplace_back(image);
    }
    for (const auto& image : scene.modelTextures) {
        modelTextures_.emplace_back(image);
    }

    terrain_ = uploadMesh(scene.terrain);
    for (const auto& [name, mesh] : scene.models) {
        models_.emplace(name, uploadMesh(mesh));
    }
    objects_ = scene.objects;

    markerMesh_ = std::make_unique<GlMesh>(makeMarkerVertices());
    markers_ = scene.markers;
}

void MapRenderer::setSky(const scene::SkyDome& dome) {
    skyTextures_.clear();
    for (const auto& image : dome.textures) {
        skyTextures_.emplace_back(image);
    }
    sky_ = uploadMesh(dome.mesh);
}

void MapRenderer::drawSky(const Camera& camera) const {
    // Behind everything: drawn first, without writing depth, and outside
    // the fog (the original's dome lies far past the fog's end, yet its
    // pictures show, so it cannot be fogged).
    const float scale = kSkyRadius / kSkyModelRadius;
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), camera.position) *
                            glm::scale(glm::mat4(1.0f), {scale, -scale * kSkyFlattening, scale});  // model Y points down
    applyFog(*shader_, Fog{}, camera.position);
    shader_->setMatrix("uModel", model);
    shader_->setVector("uTint", kNoTint);
    glDepthMask(GL_FALSE);
    drawParts(sky_, skyTextures_);
    glDepthMask(GL_TRUE);
}

void MapRenderer::addObject(const scene::ModelInstance& instance) {
    if (models_.contains(instance.modelName)) {
        objects_.push_back(instance);
    }
}

void MapRenderer::moveObject(std::size_t object, const glm::mat4& transform) {
    for (auto& instance : objects_) {
        if (instance.object == object) instance.transform = transform;
    }
}

std::vector<MapRenderer::GpuPart> MapRenderer::uploadMesh(const scene::Mesh& mesh) const {
    std::vector<GpuPart> parts;
    for (const auto& part : mesh) {
        parts.push_back({part.textureIndex, GlMesh(part.vertices)});
    }
    return parts;
}

const GlTexture& MapRenderer::textureOrFallback(const std::vector<GlTexture>& textures,
                                                std::uint32_t index) const {
    return index < textures.size() ? textures[index] : *whiteTexture_;
}

void MapRenderer::drawParts(const std::vector<GpuPart>& parts, const std::vector<GlTexture>& textures) const {
    for (const auto& part : parts) {
        textureOrFallback(textures, part.textureIndex).bind();
        part.mesh.draw();
    }
}

void MapRenderer::draw(const Camera& camera, int width, int height, const DrawOptions& options) const {
    glViewport(0, 0, width, height);
    const glm::vec4 background = fog_.enabled() ? glm::vec4(fog_.colour, 1.0f) : kSkyColour;
    glClearColor(background.r, background.g, background.b, background.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // Back-face culling stays off: some models are single-sided planes
    // (fences, signs) that must be visible from both sides.
    glDisable(GL_CULL_FACE);

    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    shader_->use();
    shader_->setMatrix("uViewProjection", camera.projectionMatrix(aspect) * camera.viewMatrix());
    shader_->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);

    if (!sky_.empty()) drawSky(camera);
    applyFog(*shader_, fog_, camera.position);

    // Terrain: already in world coordinates.
    shader_->setMatrix("uModel", glm::mat4(1.0f));
    shader_->setVector("uTint", kNoTint);
    drawParts(terrain_, terrainTextures_);

    if (options.showObjects) {
        for (const auto& object : objects_) {
            if (hidden_.contains(object.object)) continue;
            shader_->setMatrix("uModel", object.transform);
            drawParts(models_.at(object.modelName), modelTextures_);
        }
    }

    if (options.showMarkers) {
        applyFog(*shader_, Fog{}, camera.position);  // markers stay visible from afar
        shader_->setVector("uTint", kMarkerColour);
        whiteTexture_->bind();
        for (const auto& position : markers_) {
            shader_->setMatrix("uModel", glm::translate(glm::mat4(1.0f), position));
            markerMesh_->draw();
        }
    }
}

} // namespace openhogs::render
