// CharacterRenderer.cpp - see CharacterRenderer.h for an overview.

#include "openhogs/render/CharacterRenderer.h"

#include "BasicShader.h"

#include "openhogs/formats/Model.h"

#include <glm/gtc/matrix_transform.hpp>

namespace openhogs::render {

namespace {

constexpr glm::vec4 kBackgroundColour{0.45f, 0.62f, 0.85f, 1.0f};
constexpr glm::vec4 kNoTint{1.0f};
constexpr glm::vec4 kGridColour{0.35f, 0.45f, 0.3f, 1.0f};

// A square grid of thin quads on the ground (world units), so the hog's
// movement and its contact with the ground are easy to see.
std::vector<scene::Vertex> makeGridVertices() {
    constexpr int kHalfLines = 6;
    constexpr float kSpacing = 64.0f;  // world units between lines
    constexpr float kHalfWidth = 0.8f;
    constexpr float kExtent = kHalfLines * kSpacing;

    std::vector<scene::Vertex> vertices;
    const auto addQuad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
        for (const glm::vec3& p : {a, b, c, a, c, d}) {
            vertices.push_back({p, {0.5f, 0.5f}, 1.0f});
        }
    };
    for (int i = -kHalfLines; i <= kHalfLines; ++i) {
        const float t = static_cast<float>(i) * kSpacing;
        addQuad({t - kHalfWidth, 0, -kExtent}, {t + kHalfWidth, 0, -kExtent}, {t + kHalfWidth, 0, kExtent},
                {t - kHalfWidth, 0, kExtent});
        addQuad({-kExtent, 0, t - kHalfWidth}, {kExtent, 0, t - kHalfWidth}, {kExtent, 0, t + kHalfWidth},
                {-kExtent, 0, t + kHalfWidth});
    }
    return vertices;
}

} // namespace

CharacterRenderer::CharacterRenderer() {
    shader_ = std::make_unique<GlShaderProgram>(kBasicVertexShader, kBasicFragmentShader);
    whiteTexture_ = std::make_unique<GlTexture>(formats::RgbaImage{1, 1, {255, 255, 255, 255}});
    grid_ = std::make_unique<GlMesh>(makeGridVertices());
}

void CharacterRenderer::setTextures(const std::vector<formats::RgbaImage>& textures) {
    textures_.clear();
    for (const auto& image : textures) {
        textures_.emplace_back(image);
    }
}

void CharacterRenderer::draw(const Camera& camera, int width, int height, const scene::Mesh& posedMesh,
                             float feetDepth) const {
    glViewport(0, 0, width, height);
    glClearColor(kBackgroundColour.r, kBackgroundColour.g, kBackgroundColour.b, kBackgroundColour.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    shader_->use();
    shader_->setMatrix("uViewProjection", camera.projectionMatrix(aspect) * camera.viewMatrix());
    shader_->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);

    shader_->setMatrix("uModel", glm::mat4(1.0f));
    shader_->setVector("uTint", kGridColour);
    whiteTexture_->bind();
    grid_->draw();

    // Model space -> world: lift the origin so the feet touch the ground,
    // then halve the size and mirror Y (model Y points down).
    const float scale = formats::kModelToWorldScale;
    const glm::mat4 modelToWorld = glm::translate(glm::mat4(1.0f), {0.0f, feetDepth * scale, 0.0f}) *
                                   glm::scale(glm::mat4(1.0f), {scale, -scale, scale});
    shader_->setMatrix("uModel", modelToWorld);
    shader_->setVector("uTint", kNoTint);
    for (const auto& part : posedMesh) {
        const GlTexture& texture = part.textureIndex < textures_.size() ? textures_[part.textureIndex] : *whiteTexture_;
        texture.bind();
        GlMesh(part.vertices).draw();
    }
}

} // namespace openhogs::render
