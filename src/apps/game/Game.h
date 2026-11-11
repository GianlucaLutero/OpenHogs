// Game.h
//
// One match: a map with its hogs taking turns and fighting, a camera that
// follows the action, the HUD and the pause menu. The application
// (Application.h) owns the window, the sound and the menus, and creates a
// Game for each match.
//
// Each frame the application:
//   1. passes the input events to handleEvent(),
//   2. calls advance(), which moves the world on in fixed steps
//      (game::FixedTimestep) and plays the sounds of what happened,
//   3. calls draw(): the map, the hogs, the effects, then the HUD.
//
// The HUD (ui::describeTurn) shows the team, class, health and weapon of
// the active hog, the time and movement left and the power of a throw.
// Texts come from the game's text tables, so a language pack translates
// them.
//
// Controls:
//   W / Up, S / Down    run forward / walk back
//   A / Left, D / Right turn
//   Space               jump (with W: jump forward)
//   Q / E               previous / next weapon
//   R / F               aim up / down
//   Ctrl or X           fire (hold to throw further, release to throw)
//   Tab / Shift+Tab     another hog of the team (before moving)
//   Enter               end the turn (after the match: back to the menu)
//   Left mouse drag     turn the camera around the hog     Mouse wheel: zoom
//   Esc or P            pause menu (continue / abort the match)
//   (gamepads: see Gamepads.h)
//
// During the turns of the computer's teams (GameOptions::computerTeams)
// the keys that control the hog do nothing; the camera keys still work.
//   F12                 save a screenshot (openhogs-<map>.png)

#pragma once

#include "Gamepads.h"
#include "Interface.h"
#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/ComputerPlayer.h"
#include "openhogs/game/FixedTimestep.h"
#include "openhogs/game/World.h"
#include "openhogs/render/CharacterRenderer.h"
#include "openhogs/render/EffectsRenderer.h"
#include "openhogs/render/MapRenderer.h"
#include "openhogs/scene/CharacterLibrary.h"
#include "openhogs/ui/Hud.h"
#include "openhogs/ui/MessageBanner.h"
#include "openhogs/ui/PauseMenu.h"

#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <set>
#include <string>

union SDL_Event;

namespace openhogs::gameapp {

struct GameOptions {
    std::string mapName;
    std::optional<std::size_t> startHog;  // hog to start with (0-based), if of the first team
    bool autoWalk = false;                // hold "forward" (for screenshots of a walking hog)
    // For screenshots of a shot: select this weapon slot of the first hog
    // and hold "fire" from this time (seconds) for kAutoFireHoldSeconds.
    std::optional<std::size_t> autoWeaponSlot;
    std::optional<double> autoFireAt;
    std::vector<int> computerTeams;  // teams played by the computer (game::ComputerPlayer)

    // Campaign missions: the turn time, a change to the map's hogs (the
    // player's squad takes their place) and the nation each team fights for.
    std::optional<double> turnSeconds;
    int suddenDeathRound = 0;  // 0 = none (multiplayer field conditions)
    std::function<void(std::vector<game::Hog>&)> prepareHogs;
    std::map<int, int> teamNations;
};

// What the match asks of the application after an input event.
enum class GameRequest {
    None,
    LeaveMatch,  // back to the menus (match aborted or over)
    Screenshot,  // F12: save a screenshot of the current frame
};

class Game {
public:
    // Needs a current OpenGL context. `userInterface`, `sounds` and `gamepads` must outlive
    // the game.
    Game(const core::VirtualFileSystem& vfs, const GameOptions& options, Interface& userInterface, SoundPlayer& sounds,
         const Gamepads& gamepads);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    GameRequest handleEvent(const SDL_Event& event);

    // Moves the match on by `elapsedSeconds` of real time.
    void advance(double elapsedSeconds);

    // Advances the match by `seconds` at once (used to take screenshots of
    // a match that has been running for a while).
    void simulate(double seconds);

    void draw(int width, int height);

    // Opens the pause menu (also used when the window loses focus).
    void pause();

    const std::string& mapName() const { return mapName_; }
    const game::World& world() const { return world_; }

    // The campaign squad's hogs (Hog::squadIndex) in the order they died.
    const std::vector<std::size_t>& squadDeaths() const { return squadDeaths_; }

private:
    // What a team's hogs are drawn with.
    struct TeamLook {
        std::unique_ptr<render::CharacterTextureSet> textures;
        std::vector<glm::uvec2> textureSizes;
        scene::FaceTextureSlots faces;
    };

    // What a key or pad button asks the match to do.
    enum class Command { None, Pause, NextHog, PreviousHog, EndTurn, Jump, PreviousWeapon, NextWeapon };
    static Command commandForKey(const SDL_Event& event);
    static Command commandForPad(Uint8 button);
    GameRequest handleCommand(Command command);
    GameRequest handlePauseInput(std::optional<ui::MenuInput> input);
    game::PlayerInput readInput();
    void step(double seconds);
    void handleEvents(const std::vector<game::GameEvent>& events);
    void drawWeapons(const render::Camera& camera, int width, int height);
    void drawEffects(const render::Camera& camera, int width, int height);
    void drawModel(const render::Camera& camera, int width, int height, const formats::Model& model,
                   const glm::mat4& modelToWorld);
    const formats::Model* weaponModel(std::string_view name);
    void drawScene(int width, int height);
    void drawHud();
    void drawTurnPanel(const ui::HudInfo& info);
    void drawWeaponPanel(const ui::HudInfo& info);
    void drawHog(const render::Camera& camera, int width, int height, const game::Hog& hog, std::size_t index);
    const formats::Model& modelFor(const scene::CharacterClass& characterClass);
    const TeamLook& lookFor(int team);
    glm::vec3 cameraTarget() const;
    bool isComputerTeam(int team) const;
    bool computerIsPlaying() const;  // the active team is the computer's

    std::string mapName_;
    scene::CharacterLibrary library_;
    game::World world_;
    game::ChaseCamera camera_;
    game::FixedTimestep timestep_;
    bool paused_ = false;
    ui::PauseMenu pauseMenu_;
    ui::MessageBanner banner_;
    std::set<int> computerTeams_;
    std::vector<std::size_t> squadDeaths_;
    game::ComputerPlayer computer_;
    bool autoWalk_ = false;
    std::optional<double> autoFireAt_;
    bool jumpPressed_ = false;  // Space was pressed since the last step
    int weaponStep_ = 0;        // Q / E pressed since the last step

    // After a change of hog the camera swings round behind the new hog
    // for this long, even if it does not move.
    std::optional<std::size_t> cameraHog_;
    double cameraSwingSecondsLeft_ = 0.0;

    Interface& interface_;
    SoundPlayer& sounds_;
    const Gamepads& gamepads_;

    // GPU resources (the application keeps the OpenGL context alive).
    std::unique_ptr<render::MapRenderer> mapRenderer_;
    std::unique_ptr<render::CharacterRenderer> hogRenderer_;
    std::unique_ptr<render::EffectsRenderer> effects_;
    std::unique_ptr<render::CharacterTextureSet> weaponTextures_;
    std::vector<glm::uvec2> weaponTextureSizes_;
    std::map<std::string, std::optional<formats::Model>, std::less<>> weaponModels_;  // empty: missing
    std::map<std::string, formats::Model, std::less<>> models_;  // by class id
    std::map<int, TeamLook> teamLooks_;
};

} // namespace openhogs::gameapp
