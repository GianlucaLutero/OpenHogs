// UiRenderer.cpp - see UiRenderer.h for an overview.

#include "openhogs/render/UiRenderer.h"

#include "openhogs/ui/TextLayout.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <utility>

namespace openhogs::render {

namespace {

// Unlike the 3D shader, this one blends: panels are see-through and the
// fonts' transparent background (alpha 0) simply does not show.
constexpr const char* kUiVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;

uniform mat4 uProjection;

out vec2 uv;

void main() {
    gl_Position = uProjection * vec4(inPosition, 0.0, 1.0);
    uv = inUv;
}
)";

constexpr const char* kUiFragmentShader = R"(
#version 330 core
in vec2 uv;

uniform sampler2D uTexture;
uniform vec4 uColour;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, uv) * uColour;
}
)";

// How far the text shadow is offset, in font pixels.
constexpr float kShadowOffset = 1.0f;
constexpr glm::vec4 kShadowColour{0.0f, 0.0f, 0.0f, 0.75f};

} // namespace

UiFont::UiFont(formats::BitmapFont font) : font_(std::move(font)), texture_(font_.atlas) {}

UiImage::UiImage(const formats::RgbaImage& image)
    : texture_(image), width_(static_cast<float>(image.width)), height_(static_cast<float>(image.height)) {}

UiRenderer::UiRenderer() {
    shader_ = std::make_unique<GlShaderProgram>(kUiVertexShader, kUiFragmentShader);
    whiteTexture_ = std::make_unique<GlTexture>(formats::RgbaImage{1, 1, {255, 255, 255, 255}});

    // One buffer refilled for every draw call: the interface is a handful
    // of quads, so there is nothing to gain from keeping them on the GPU.
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          reinterpret_cast<const void*>(offsetof(Vertex2D, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D),
                          reinterpret_cast<const void*>(offsetof(Vertex2D, uv)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

UiRenderer::~UiRenderer() {
    glDeleteBuffers(1, &vertexBuffer_);
    glDeleteVertexArrays(1, &vertexArray_);
}

void UiRenderer::begin(int widthPixels, int heightPixels) {
    glViewport(0, 0, widthPixels, heightPixels);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    canvasWidth_ = heightPixels > 0 ? kCanvasHeight * static_cast<float>(widthPixels) / static_cast<float>(heightPixels)
                                    : kCanvasHeight;
    // Canvas units to clip space, y pointing down like on paper.
    const glm::mat4 projection = glm::ortho(0.0f, canvasWidth_, kCanvasHeight, 0.0f, -1.0f, 1.0f);
    shader_->use();
    shader_->setMatrix("uProjection", projection);
    shader_->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
}

void UiRenderer::end() const {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UiRenderer::drawQuads(const std::vector<Vertex2D>& vertices, const GlTexture& texture,
                           const glm::vec4& colour) const {
    if (vertices.empty()) {
        return;
    }
    shader_->setVector("uColour", colour);
    texture.bind();
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex2D)), vertices.data(),
                 GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}

namespace {

// Two triangles covering a rectangle, with texture coordinates u0..u1,
// v0..v1.
template <typename Vertex>
void addRectangle(std::vector<Vertex>& vertices, float x, float y, float width, float height, float u0, float v0,
                  float u1, float v1) {
    const Vertex topLeft{{x, y}, {u0, v0}};
    const Vertex topRight{{x + width, y}, {u1, v0}};
    const Vertex bottomRight{{x + width, y + height}, {u1, v1}};
    const Vertex bottomLeft{{x, y + height}, {u0, v1}};
    for (const Vertex& vertex : {topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft}) {
        vertices.push_back(vertex);
    }
}

} // namespace

void UiRenderer::drawRect(float x, float y, float width, float height, const glm::vec4& colour) const {
    std::vector<Vertex2D> vertices;
    addRectangle(vertices, x, y, width, height, 0.5f, 0.5f, 0.5f, 0.5f);
    drawQuads(vertices, *whiteTexture_, colour);
}

void UiRenderer::drawImage(const UiImage& image, float x, float y, float width, float height,
                           const glm::vec4& tint) const {
    std::vector<Vertex2D> vertices;
    addRectangle(vertices, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuads(vertices, image.texture(), tint);
}

void UiRenderer::drawImageRegion(const UiImage& image, float x, float y, float width, float height, float u0,
                                 float v0, float u1, float v1, const glm::vec4& tint) const {
    std::vector<Vertex2D> vertices;
    addRectangle(vertices, x, y, width, height, u0, v0, u1, v1);
    drawQuads(vertices, image.texture(), tint);
}

void UiRenderer::drawImageQuad(const UiImage& image, const std::array<glm::vec2, 4>& corners,
                               const glm::vec4& tint) const {
    const Vertex2D topLeft{corners[0], {0.0f, 0.0f}};
    const Vertex2D topRight{corners[1], {1.0f, 0.0f}};
    const Vertex2D bottomRight{corners[2], {1.0f, 1.0f}};
    const Vertex2D bottomLeft{corners[3], {0.0f, 1.0f}};
    drawQuads({topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft}, image.texture(), tint);
}

void UiRenderer::drawGlyphs(const UiFont& font, std::string_view text, float x, float y, float scale,
                            const glm::vec4& colour) const {
    const auto& atlas = font.font().atlas;
    const float atlasWidth = static_cast<float>(atlas.width);
    const float atlasHeight = static_cast<float>(atlas.height);

    std::vector<Vertex2D> vertices;
    for (const auto& quad : ui::layoutText(font.font(), text, x, y, scale)) {
        const float u0 = static_cast<float>(quad.source.x) / atlasWidth;
        const float v0 = static_cast<float>(quad.source.y) / atlasHeight;
        const float u1 = static_cast<float>(quad.source.x + quad.source.width) / atlasWidth;
        const float v1 = static_cast<float>(quad.source.y + quad.source.height) / atlasHeight;
        addRectangle(vertices, quad.x, quad.y, quad.width, quad.height, u0, v0, u1, v1);
    }
    drawQuads(vertices, font.texture(), colour);
}

void UiRenderer::drawText(const UiFont& font, std::string_view text, float x, float y, const TextStyle& style) const {
    const float width = textWidth(font, text, style.scale);
    if (style.align == TextAlign::Centre) {
        x -= width / 2.0f;
    } else if (style.align == TextAlign::Right) {
        x -= width;
    }
    if (style.shadow) {
        const float offset = kShadowOffset * style.scale;
        drawGlyphs(font, text, x + offset, y + offset, style.scale,
                   glm::vec4(glm::vec3(kShadowColour), kShadowColour.a * style.colour.a));
    }
    drawGlyphs(font, text, x, y, style.scale, style.colour);
}

float UiRenderer::textWidth(const UiFont& font, std::string_view text, float scale) {
    return ui::measureText(font.font(), text, scale);
}

} // namespace openhogs::render
