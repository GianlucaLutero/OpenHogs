// MapViewer.h
//
// The map viewer application: one window, one map at a time, a free
// camera, and keys to switch between maps.
//
// Controls:
//   W A S D        move            Q / E      down / up
//   Shift          move faster     Right mouse drag or arrow keys: look around
//   Page Up/Down   previous / next map
//   O              show / hide objects
//   M              show / hide markers (hog start positions)
//   F              frame the whole map
//   F12            save a screenshot (openhogs-<map>.png)
//   Esc            quit

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/MapRenderer.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct SDL_Window;
union SDL_Event;

namespace openhogs::mapviewer {

class MapViewer {
public:
    // `maps` must not be empty; `startIndex` selects the first map shown.
    MapViewer(const core::VirtualFileSystem& vfs, std::vector<std::string> maps, std::size_t startIndex,
              int width, int height, bool hiddenWindow);
    ~MapViewer();

    MapViewer(const MapViewer&) = delete;
    MapViewer& operator=(const MapViewer&) = delete;

    // Runs until the user closes the window.
    void run();

    // Moves the camera, e.g. to reproduce a view from the command line.
    void setCamera(const glm::vec3& position, float yawDegrees, float pitchDegrees);

    // Renders a single frame off-screen and writes it as a PNG file.
    void saveScreenshot(const std::filesystem::path& path);

private:
    void loadMap(std::size_t index);
    void handleEvent(const SDL_Event& event, bool& running);
    void updateCamera(float seconds);

    const core::VirtualFileSystem& vfs_;
    std::vector<std::string> maps_;
    std::size_t currentMap_ = 0;

    SDL_Window* window_ = nullptr;
    void* glContext_ = nullptr;  // SDL_GLContext
    int width_;
    int height_;

    render::Camera camera_;
    render::DrawOptions options_;
    std::unique_ptr<render::MapRenderer> renderer_;
    glm::vec3 mapBoundsMin_{0.0f};  // used by "frame the whole map"
    glm::vec3 mapBoundsMax_{0.0f};
};

} // namespace openhogs::mapviewer
