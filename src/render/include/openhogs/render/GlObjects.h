// GlObjects.h
//
// Small RAII wrappers around OpenGL objects. Each wrapper creates its
// object in the constructor and deletes it in the destructor, so GPU
// resources can never leak. They can be moved but not copied (two owners
// would delete the same object twice).
//
// All functions require a current OpenGL 3.3 context.

#pragma once

#include "openhogs/formats/TimImage.h"
#include "openhogs/scene/Geometry.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <utility>

namespace openhogs::render {

// A 2D texture created from an RGBA image. Uses nearest-neighbour filtering
// to keep the crisp pixel look of the original PlayStation textures.
class GlTexture {
public:
    explicit GlTexture(const formats::RgbaImage& image);
    ~GlTexture();
    GlTexture(GlTexture&& other) noexcept : id_(std::exchange(other.id_, 0)) {}
    GlTexture& operator=(GlTexture&& other) noexcept;
    GlTexture(const GlTexture&) = delete;
    GlTexture& operator=(const GlTexture&) = delete;

    void bind() const;

private:
    GLuint id_ = 0;
};

// Triangles of one scene::MeshPart, stored in GPU memory.
class GlMesh {
public:
    explicit GlMesh(const std::vector<scene::Vertex>& vertices);
    ~GlMesh();
    GlMesh(GlMesh&& other) noexcept;
    GlMesh& operator=(GlMesh&& other) noexcept;
    GlMesh(const GlMesh&) = delete;
    GlMesh& operator=(const GlMesh&) = delete;

    void draw() const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLsizei vertexCount_ = 0;
};

// An off-screen render target (colour + depth) of a fixed size. Used to
// take screenshots without depending on a visible window.
class GlFramebuffer {
public:
    GlFramebuffer(int width, int height);
    ~GlFramebuffer();
    GlFramebuffer(const GlFramebuffer&) = delete;
    GlFramebuffer& operator=(const GlFramebuffer&) = delete;

    // Following draw calls go to this target until unbind() is called.
    void bind() const;
    static void unbind();

private:
    GLuint framebuffer_ = 0;
    GLuint colour_ = 0;
    GLuint depth_ = 0;
};

// A vertex + fragment shader pair. Throws std::runtime_error with the
// compiler log if a shader does not compile or link.
class GlShaderProgram {
public:
    GlShaderProgram(const char* vertexSource, const char* fragmentSource);
    ~GlShaderProgram();
    GlShaderProgram(const GlShaderProgram&) = delete;
    GlShaderProgram& operator=(const GlShaderProgram&) = delete;

    void use() const;
    void setMatrix(const char* name, const glm::mat4& value) const;
    void setVector(const char* name, const glm::vec4& value) const;
    void setInt(const char* name, int value) const;

private:
    GLuint id_ = 0;
};

// Reads back the current framebuffer as an image (top row first), e.g. to
// save a screenshot.
formats::RgbaImage captureFramebuffer(int width, int height);

} // namespace openhogs::render
