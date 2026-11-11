// GlWindow.cpp - see GlWindow.h for an overview.

#include "openhogs/apps/GlWindow.h"

#include "openhogs/extractor/PngWriter.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <fstream>
#include <stdexcept>

namespace openhogs::apps {

namespace {

std::string sdlError(const std::string& what) {
    return what + ": " + SDL_GetError();
}

} // namespace

GlWindow::GlWindow(const std::string& title, int width, int height, bool hidden) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(sdlError("Cannot initialise SDL"));
    }

    // OpenGL 3.3 core is supported by every GPU of the last 15 years.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }
    window_ = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (window_ == nullptr) {
        throw std::runtime_error(sdlError("Cannot create the window"));
    }
    glContext_ = SDL_GL_CreateContext(window_);
    if (glContext_ == nullptr) {
        throw std::runtime_error(sdlError("Cannot create an OpenGL 3.3 context"));
    }
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) == 0) {
        throw std::runtime_error("Cannot load the OpenGL functions");
    }
    SDL_GL_SetSwapInterval(1);  // vsync: no need to draw faster than the screen
}

GlWindow::~GlWindow() {
    if (glContext_ != nullptr) SDL_GL_DestroyContext(static_cast<SDL_GLContext>(glContext_));
    if (window_ != nullptr) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void GlWindow::setTitle(const std::string& title) {
    SDL_SetWindowTitle(window_, title.c_str());
}

void GlWindow::sizeInPixels(int& width, int& height) const {
    SDL_GetWindowSizeInPixels(window_, &width, &height);
}

void GlWindow::swap() {
    SDL_GL_SwapWindow(window_);
}

void savePng(const formats::RgbaImage& image, const std::filesystem::path& path) {
    const auto data = extractor::encodePng(image);
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) {
        throw std::runtime_error("Cannot write " + path.string());
    }
}

} // namespace openhogs::apps
