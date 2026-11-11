// Game.h
//
// The game application: a map with its hogs, a camera that follows the
// selected hog, ambient sound, and the main loop that ties them together.
//
// Each frame the loop:
//   1. reads the input (camera, hog selection),
//   2. advances the world in fixed steps (game::FixedTimestep),
//   3. draws the map and the hogs.
//
// Controls:
//   Tab / Shift+Tab   next / previous hog
//   Left mouse drag   turn the camera around the hog     Mouse wheel: zoom
//   P                 pause
//   F12               save a screenshot (openhogs-<map>.png)
//   Esc               quit

#pragma once

#include "openhogs/apps/GlWindow.h"
#include "openhogs/audio/AudioDevice.h"
#include "openhogs/audio/Mixer.h"
#include "openhogs/audio/SoundLibrary.h"
#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/FixedTimestep.h"
#include "openhogs/game/World.h"
#include "openhogs/render/CharacterRenderer.h"
#include "openhogs/render/MapRenderer.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>

union SDL_Event;

namespace openhogs::gameapp {

struct GameOptions {
    std::string mapName;
    int width = 1280;
    int height = 720;
    bool hiddenWindow = false;            // for screenshots without a window
    bool sound = true;
    audio::Volumes volumes;
    std::optional<std::size_t> startHog;  // hog to select first (0-based)
};

class Game {
public:
    Game(const core::VirtualFileSystem& vfs, const GameOptions& options);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Runs until the player quits.
    void run();

    // Advances the match by `seconds` without drawing (used to take
    // screenshots of a match that has been running for a while).
    void simulate(double seconds);

    // Renders the current state off-screen and writes it as a PNG file.
    void saveScreenshot(const std::filesystem::path& path);

private:
    // What a team's hogs are drawn with.
    struct TeamLook {
        std::unique_ptr<render::CharacterTextureSet> textures;
        std::vector<glm::uvec2> textureSizes;
        scene::FaceTextureSlots faces;
    };

    void startSound(const core::VirtualFileSystem& vfs, const audio::Volumes& volumes);
    void handleEvent(const SDL_Event& event, bool& running);
    void step(double seconds);
    void draw(int width, int height);
    void drawHog(const render::Camera& camera, int width, int height, const game::Hog& hog, std::size_t index);
    const formats::Model& modelFor(const scene::CharacterClass& characterClass);
    const TeamLook& lookFor(int team);
    glm::vec3 cameraTarget() const;
    void updateTitle();

    std::string mapName_;
    scene::CharacterLibrary library_;
    game::World world_;
    game::ChaseCamera camera_;
    game::FixedTimestep timestep_;
    bool paused_ = false;

    // The window owns the OpenGL context: everything below needs it and is
    // destroyed first (see the destructor).
    apps::GlWindow window_;
    int width_;
    int height_;
    std::unique_ptr<render::MapRenderer> mapRenderer_;
    std::unique_ptr<render::CharacterRenderer> hogRenderer_;
    std::map<std::string, formats::Model, std::less<>> models_;  // by class id
    std::map<int, TeamLook> teamLooks_;

    // Sound is optional: the game still runs on a computer without it.
    std::unique_ptr<audio::Mixer> mixer_;
    std::unique_ptr<audio::AudioDevice> audioDevice_;
};

} // namespace openhogs::gameapp
