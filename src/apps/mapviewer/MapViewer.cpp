// MapViewer.cpp - see MapViewer.h for an overview.

#include "MapViewer.h"

#include "openhogs/scene/MapScene.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <iostream>
#include <stdexcept>

namespace openhogs::mapviewer {

namespace {

constexpr float kMoveSpeed = 3000.0f;       // world units per second
constexpr float kFastMultiplier = 4.0f;     // while Shift is held
constexpr float kMouseDegreesPerPixel = 0.2f;
constexpr float kKeyTurnDegreesPerSecond = 90.0f;

} // namespace

MapViewer::MapViewer(const core::VirtualFileSystem& vfs, std::vector<std::string> maps, std::size_t startIndex,
                     int width, int height, bool hiddenWindow)
    : vfs_(vfs), maps_(std::move(maps)), window_("OpenHogs map viewer", width, height, hiddenWindow),
      width_(width), height_(height) {
    loadMap(startIndex);
}

MapViewer::~MapViewer() {
    renderer_.reset();  // GPU resources must go before the window's context
}

void MapViewer::loadMap(std::size_t index) {
    currentMap_ = index;
    const std::string& name = maps_[index];

    const auto scene = scene::loadMapScene(vfs_, name);
    for (const auto& warning : scene.warnings) {
        std::cerr << "warning: " << warning << "\n";
    }
    std::cout << "Map " << name << ": " << scene.objects.size() << " objects, " << scene.models.size()
              << " models, " << scene.markers.size() << " markers\n";

    renderer_ = std::make_unique<render::MapRenderer>(scene);
    mapBoundsMin_ = scene.boundsMin;
    mapBoundsMax_ = scene.boundsMax;
    camera_.frame(mapBoundsMin_, mapBoundsMax_);
    std::cout << "Camera: " << camera_.position.x << "," << camera_.position.y << "," << camera_.position.z << ","
              << camera_.yawDegrees << "," << camera_.pitchDegrees << " (see --camera)\n";

    const std::string title = "OpenHogs map viewer - " + name + " (" + std::to_string(index + 1) + "/" +
                              std::to_string(maps_.size()) + ")";
    window_.setTitle(title);
}

void MapViewer::handleEvent(const SDL_Event& event, bool& running) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            running = false;
            break;

        case SDL_EVENT_MOUSE_MOTION:
            // Look around while the right mouse button is held.
            if (event.motion.state & SDL_BUTTON_RMASK) {
                camera_.turn(event.motion.xrel * kMouseDegreesPerPixel, -event.motion.yrel * kMouseDegreesPerPixel);
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            switch (event.key.key) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_PAGEDOWN: loadMap((currentMap_ + 1) % maps_.size()); break;
                case SDLK_PAGEUP: loadMap((currentMap_ + maps_.size() - 1) % maps_.size()); break;
                case SDLK_O: options_.showObjects = !options_.showObjects; break;
                case SDLK_M: options_.showMarkers = !options_.showMarkers; break;
                case SDLK_F: camera_.frame(mapBoundsMin_, mapBoundsMax_); break;
                case SDLK_F12: {
                    const std::string file = "openhogs-" + maps_[currentMap_] + ".png";
                    saveScreenshot(file);
                    std::cout << "Saved " << file << "\n";
                    break;
                }
                default: break;
            }
            break;

        default:
            break;
    }
}

void MapViewer::updateCamera(float seconds) {
    const bool* keys = SDL_GetKeyboardState(nullptr);

    glm::vec3 direction{0.0f};  // x = right, y = up, z = forward
    if (keys[SDL_SCANCODE_W]) direction.z += 1.0f;
    if (keys[SDL_SCANCODE_S]) direction.z -= 1.0f;
    if (keys[SDL_SCANCODE_D]) direction.x += 1.0f;
    if (keys[SDL_SCANCODE_A]) direction.x -= 1.0f;
    if (keys[SDL_SCANCODE_E]) direction.y += 1.0f;
    if (keys[SDL_SCANCODE_Q]) direction.y -= 1.0f;

    float speed = kMoveSpeed * seconds;
    if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) speed *= kFastMultiplier;
    camera_.move(direction * speed);

    const float turn = kKeyTurnDegreesPerSecond * seconds;
    if (keys[SDL_SCANCODE_LEFT]) camera_.turn(-turn, 0.0f);
    if (keys[SDL_SCANCODE_RIGHT]) camera_.turn(turn, 0.0f);
    if (keys[SDL_SCANCODE_UP]) camera_.turn(0.0f, turn);
    if (keys[SDL_SCANCODE_DOWN]) camera_.turn(0.0f, -turn);
}

void MapViewer::run() {
    bool running = true;
    Uint64 previous = SDL_GetTicksNS();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event, running);
        }

        const Uint64 now = SDL_GetTicksNS();
        const float seconds = static_cast<float>(now - previous) / 1e9f;
        previous = now;
        updateCamera(seconds);

        // The window can be resized: always draw at its current pixel size.
        window_.sizeInPixels(width_, height_);
        renderer_->draw(camera_, width_, height_, options_);
        window_.swap();
    }
}

void MapViewer::setCamera(const glm::vec3& position, float yawDegrees, float pitchDegrees) {
    camera_.position = position;
    camera_.yawDegrees = yawDegrees;
    camera_.pitchDegrees = pitchDegrees;
}

void MapViewer::saveScreenshot(const std::filesystem::path& path) {
    // Off-screen, so it works even with a hidden or covered window.
    render::GlFramebuffer target(width_, height_);
    target.bind();
    renderer_->draw(camera_, width_, height_, options_);
    const auto image = render::captureFramebuffer(width_, height_);
    render::GlFramebuffer::unbind();

    apps::savePng(image, path);
}

} // namespace openhogs::mapviewer
