// Game.h
//
// The game application: a map with its hogs taking turns, a camera that
// follows the active hog, sound, and the main loop that ties them together.
//
// Each frame the loop:
//   1. reads the input (movement, camera, turn commands),
//   2. advances the world in fixed steps (game::FixedTimestep) and plays
//      the sounds of what happened,
//   3. draws the map and the hogs.
//
// The state of the turn (team, hog, time and movement left) is shown in the
// window title until the HUD exists (menus-and-hud block).
//
// Controls:
//   W / Up, S / Down    run forward / walk back
//   A / Left, D / Right turn
//   Space               jump (with W: jump forward)
//   Tab / Shift+Tab     another hog of the team (before moving)
//   Enter               end the turn
//   Left mouse drag     turn the camera around the hog     Mouse wheel: zoom
//   P                   pause
//   F12                 save a screenshot (openhogs-<map>.png)
//   Esc                 quit

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
    std::optional<std::size_t> startHog;  // hog to start with (0-based), if of the first team
    bool autoWalk = false;                // hold "forward" (for screenshots of a walking hog)
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
    game::MovementInput readInput();
    void step(double seconds);
    void playSounds(const game::MovementEvents& events);
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
    bool autoWalk_ = false;
    bool jumpPressed_ = false;  // Space was pressed since the last step

    // After a change of hog the camera swings round behind the new hog
    // for this long, even if it does not move.
    std::optional<std::size_t> cameraHog_;
    double cameraSwingSecondsLeft_ = 0.0;
    std::string lastTitle_;

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
    std::map<formats::SurfaceType, std::shared_ptr<const formats::PcmSound>> footstepSounds_;
    std::shared_ptr<const formats::PcmSound> landingSound_;
    std::shared_ptr<const formats::PcmSound> splashSound_;
};

} // namespace openhogs::gameapp
