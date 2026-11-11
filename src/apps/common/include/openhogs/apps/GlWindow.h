// GlWindow.h
//
// A window with an OpenGL 3.3 context, shared by the SDL applications (map
// viewer, character viewer). It hides the SDL and glad setup so each
// application only deals with drawing and input.
//
// Also provides savePng(), which writes an image file: the viewers use it
// for screenshots.

#pragma once

#include "openhogs/formats/TimImage.h"

#include <filesystem>
#include <string>

struct SDL_Window;

namespace openhogs::apps {

class GlWindow {
public:
    // Creates the window and makes its OpenGL context current. A hidden
    // window is used to render screenshots without showing anything.
    // Throws std::runtime_error if SDL or OpenGL 3.3 is not available.
    GlWindow(const std::string& title, int width, int height, bool hidden);
    ~GlWindow();

    GlWindow(const GlWindow&) = delete;
    GlWindow& operator=(const GlWindow&) = delete;

    void setTitle(const std::string& title);

    // Current drawable size in pixels (the window can be resized).
    void sizeInPixels(int& width, int& height) const;

    // Shows what was drawn since the last call.
    void swap();

private:
    SDL_Window* window_ = nullptr;
    void* glContext_ = nullptr;  // SDL_GLContext
};

// Encodes `image` as PNG and writes it. Throws std::runtime_error if the
// file cannot be written.
void savePng(const formats::RgbaImage& image, const std::filesystem::path& path);

} // namespace openhogs::apps
