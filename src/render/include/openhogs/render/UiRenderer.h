// UiRenderer.h
//
// Draws the 2D interface on top of the 3D scene: text in the game's bitmap
// fonts, coloured panels and bars, and pictures such as weapon icons.
//
// Coordinates are "canvas units": the screen is always kCanvasHeight (480,
// the original's screen height) units tall, and as wide as its aspect
// ratio makes it (canvasWidth(): 853 for 16:9). (0, 0) is the top-left
// corner. So the interface keeps its size relative to the screen at any
// resolution, and layouts can be written in the original's pixels.
//
// Everything is drawn with alpha blending and without depth testing, in
// the order of the calls.

#pragma once

#include "openhogs/formats/BitmapFont.h"
#include "openhogs/render/GlObjects.h"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string_view>
#include <vector>

namespace openhogs::render {

inline constexpr float kCanvasHeight = 480.0f;

// A bitmap font with its atlas on the GPU.
class UiFont {
public:
    explicit UiFont(formats::BitmapFont font);

    const formats::BitmapFont& font() const { return font_; }
    const GlTexture& texture() const { return texture_; }

private:
    formats::BitmapFont font_;
    GlTexture texture_;
};

// A picture on the GPU, with its size in pixels.
class UiImage {
public:
    explicit UiImage(const formats::RgbaImage& image);

    const GlTexture& texture() const { return texture_; }
    GlTexture& texture() { return texture_; }
    float width() const { return width_; }
    float height() const { return height_; }

private:
    GlTexture texture_;
    float width_;
    float height_;
};

enum class TextAlign { Left, Centre, Right };

struct TextStyle {
    float scale = 1.0f;  // 1 = one font pixel per canvas unit
    glm::vec4 colour{1.0f};
    TextAlign align = TextAlign::Left;  // relative to the x given to drawText
    bool shadow = true;                 // a dark copy behind, readable on any background
};

class UiRenderer {
public:
    // Needs a current OpenGL context.
    UiRenderer();
    ~UiRenderer();
    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    // Must be called before the draw functions of a frame; end() restores
    // the 3D drawing state.
    void begin(int widthPixels, int heightPixels);
    void end() const;

    float canvasWidth() const { return canvasWidth_; }

    void drawRect(float x, float y, float width, float height, const glm::vec4& colour) const;

    // The whole picture stretched over the rectangle.
    void drawImage(const UiImage& image, float x, float y, float width, float height,
                   const glm::vec4& tint = glm::vec4(1.0f)) const;

    // Part of the picture: texture coordinates u0..u1, v0..v1 (0..1, from
    // the top-left corner) stretched over the rectangle. Swapping v0 and
    // v1 draws it upside down; a narrower range crops it.
    void drawImageRegion(const UiImage& image, float x, float y, float width, float height, float u0, float v0,
                         float u1, float v1, const glm::vec4& tint = glm::vec4(1.0f)) const;

    // The whole picture on any four-cornered shape (to turn it): the
    // corners of the picture's top-left, top-right, bottom-right and
    // bottom-left, in that order.
    void drawImageQuad(const UiImage& image, const std::array<glm::vec2, 4>& corners,
                       const glm::vec4& tint = glm::vec4(1.0f)) const;

    // One line of text; (x, y) is its top-left corner for left-aligned text
    // (top-centre or top-right for the other alignments).
    void drawText(const UiFont& font, std::string_view text, float x, float y, const TextStyle& style = {}) const;

    // Width of `text` in canvas units at `scale`.
    static float textWidth(const UiFont& font, std::string_view text, float scale = 1.0f);

private:
    struct Vertex2D {
        glm::vec2 position;
        glm::vec2 uv;
    };

    void drawQuads(const std::vector<Vertex2D>& vertices, const GlTexture& texture, const glm::vec4& colour) const;
    void drawGlyphs(const UiFont& font, std::string_view text, float x, float y, float scale,
                    const glm::vec4& colour) const;

    std::unique_ptr<GlShaderProgram> shader_;
    std::unique_ptr<GlTexture> whiteTexture_;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    float canvasWidth_ = kCanvasHeight;
};

} // namespace openhogs::render
