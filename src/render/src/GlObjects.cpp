// GlObjects.cpp - see GlObjects.h for an overview.

#include "openhogs/render/GlObjects.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <stdexcept>

namespace openhogs::render {

// --- GlTexture ---------------------------------------------------------------

GlTexture::GlTexture(const formats::RgbaImage& image) {
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    // Rows of the image are tightly packed RGBA.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(image.width),
                 static_cast<GLsizei>(image.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Clamp: the game's UVs stay inside each texture, and repeating would
    // make edge pixels bleed from the opposite side.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

GlTexture::~GlTexture() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
    }
}

GlTexture& GlTexture::operator=(GlTexture&& other) noexcept {
    if (this != &other) {
        if (id_ != 0) glDeleteTextures(1, &id_);
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

void GlTexture::bind() const {
    glBindTexture(GL_TEXTURE_2D, id_);
}

// --- GlMesh ------------------------------------------------------------------

GlMesh::GlMesh(const std::vector<scene::Vertex>& vertices)
    : vertexCount_(static_cast<GLsizei>(vertices.size())) {
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(scene::Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    // Attribute locations match the "layout(location = N)" in the shaders.
    const auto stride = static_cast<GLsizei>(sizeof(scene::Vertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(scene::Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(scene::Vertex, uv)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(offsetof(scene::Vertex, brightness)));

    glBindVertexArray(0);
}

GlMesh::~GlMesh() {
    if (vertexBuffer_ != 0) glDeleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0) glDeleteVertexArrays(1, &vertexArray_);
}

GlMesh::GlMesh(GlMesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0)),
      vertexBuffer_(std::exchange(other.vertexBuffer_, 0)),
      vertexCount_(std::exchange(other.vertexCount_, 0)) {}

GlMesh& GlMesh::operator=(GlMesh&& other) noexcept {
    if (this != &other) {
        if (vertexBuffer_ != 0) glDeleteBuffers(1, &vertexBuffer_);
        if (vertexArray_ != 0) glDeleteVertexArrays(1, &vertexArray_);
        vertexArray_ = std::exchange(other.vertexArray_, 0);
        vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
        vertexCount_ = std::exchange(other.vertexCount_, 0);
    }
    return *this;
}

void GlMesh::draw() const {
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}

// --- GlFramebuffer ----------------------------------------------------------

GlFramebuffer::GlFramebuffer(int width, int height) {
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    glGenRenderbuffers(1, &colour_);
    glBindRenderbuffer(GL_RENDERBUFFER, colour_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colour_);

    glGenRenderbuffers(1, &depth_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        throw std::runtime_error("Cannot create an off-screen framebuffer");
    }
}

GlFramebuffer::~GlFramebuffer() {
    glDeleteRenderbuffers(1, &depth_);
    glDeleteRenderbuffers(1, &colour_);
    glDeleteFramebuffers(1, &framebuffer_);
}

void GlFramebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
}

void GlFramebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// --- GlShaderProgram ---------------------------------------------------------

namespace {

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compilation failed: ") + log);
    }
    return shader;
}

} // namespace

GlShaderProgram::GlShaderProgram(const char* vertexSource, const char* fragmentSource) {
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glLinkProgram(id_);
    glDeleteShader(vertex);  // the program keeps what it needs
    glDeleteShader(fragment);

    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        glDeleteProgram(id_);
        throw std::runtime_error(std::string("Shader linking failed: ") + log);
    }
}

GlShaderProgram::~GlShaderProgram() {
    glDeleteProgram(id_);
}

void GlShaderProgram::use() const {
    glUseProgram(id_);
}

void GlShaderProgram::setMatrix(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(id_, name), 1, GL_FALSE, glm::value_ptr(value));
}

void GlShaderProgram::setVector(const char* name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(id_, name), 1, glm::value_ptr(value));
}

void GlShaderProgram::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(id_, name), value);
}

} // namespace openhogs::render
