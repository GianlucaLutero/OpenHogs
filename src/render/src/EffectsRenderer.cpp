// EffectsRenderer.cpp - see EffectsRenderer.h for an overview.

#include "openhogs/render/EffectsRenderer.h"

#include "BasicShader.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace openhogs::render {

namespace {

void addQuad(std::vector<scene::Vertex>& vertices, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
             float brightness) {
    for (const glm::vec3& p : {a, b, c, a, c, d}) {
        vertices.push_back({p, {0.5f, 0.5f}, brightness});
    }
}

std::vector<scene::Vertex> makeCube() {
    std::vector<scene::Vertex> v;
    const float h = 0.5f;
    // Faces lit differently so the shape reads without real lighting.
    addQuad(v, {-h, h, -h}, {h, h, -h}, {h, h, h}, {-h, h, h}, 1.0f);        // top
    addQuad(v, {-h, -h, -h}, {-h, -h, h}, {h, -h, h}, {h, -h, -h}, 0.5f);    // bottom
    addQuad(v, {-h, -h, h}, {-h, h, h}, {h, h, h}, {h, -h, h}, 0.8f);        // +Z
    addQuad(v, {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h}, 0.7f);    // -Z
    addQuad(v, {h, -h, -h}, {h, -h, h}, {h, h, h}, {h, h, -h}, 0.9f);        // +X
    addQuad(v, {-h, -h, -h}, {-h, h, -h}, {-h, h, h}, {-h, -h, h}, 0.6f);    // -X
    return v;
}

// A UV sphere, brighter at the top.
std::vector<scene::Vertex> makeSphere() {
    constexpr int kRings = 8;
    constexpr int kSegments = 12;
    const auto point = [](int ring, int segment) {
        const float theta = glm::pi<float>() * static_cast<float>(ring) / kRings;
        const float phi = glm::two_pi<float>() * static_cast<float>(segment) / kSegments;
        return glm::vec3(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
    };
    std::vector<scene::Vertex> v;
    for (int ring = 0; ring < kRings; ++ring) {
        for (int segment = 0; segment < kSegments; ++segment) {
            const glm::vec3 a = point(ring, segment), b = point(ring, segment + 1);
            const glm::vec3 c = point(ring + 1, segment + 1), d = point(ring + 1, segment);
            addQuad(v, a, b, c, d, 0.75f + 0.25f * a.y);
        }
    }
    return v;
}

} // namespace

EffectsRenderer::EffectsRenderer() {
    shader_ = std::make_unique<GlShaderProgram>(kBasicVertexShader, kBasicFragmentShader);
    whiteTexture_ = std::make_unique<GlTexture>(formats::RgbaImage{1, 1, {255, 255, 255, 255}});
    cube_ = std::make_unique<GlMesh>(makeCube());
    sphere_ = std::make_unique<GlMesh>(makeSphere());
}

void EffectsRenderer::begin(const Camera& camera, int width, int height) const {
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    shader_->use();
    shader_->setMatrix("uViewProjection", camera.projectionMatrix(aspect) * camera.viewMatrix());
    shader_->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    whiteTexture_->bind();
}

void EffectsRenderer::drawDots(const std::vector<glm::vec3>& points, float size, const glm::vec4& colour,
                               bool alwaysVisible) const {
    if (alwaysVisible) glDisable(GL_DEPTH_TEST);
    shader_->setVector("uTint", colour);
    for (const auto& point : points) {
        shader_->setMatrix("uModel", glm::scale(glm::translate(glm::mat4(1.0f), point), glm::vec3(size)));
        cube_->draw();
    }
    if (alwaysVisible) glEnable(GL_DEPTH_TEST);
}

void EffectsRenderer::drawExplosion(const glm::vec3& centre, float radius, float progress) const {
    // Grows quickly to the blast radius, then fades from yellow to red.
    const float size = radius * std::min(1.0f, 0.3f + progress * 1.5f);
    const glm::vec4 hot{1.0f, 0.9f, 0.3f, 1.0f};
    const glm::vec4 cool{0.5f, 0.12f, 0.05f, 1.0f};
    shader_->setVector("uTint", glm::mix(hot, cool, progress));
    shader_->setMatrix("uModel", glm::scale(glm::translate(glm::mat4(1.0f), centre), glm::vec3(size)));
    sphere_->draw();
}

} // namespace openhogs::render
