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
// Controls (the keyboard's can be changed on the CONTROLS screen and are
// saved in [Controls] of openhogs.ini; these are the defaults):
//   W / Up, S / Down    run forward / walk back
//   A / Left, D / Right turn
//   Space               jump (with W: jump forward)
//   Q / E               previous / next weapon
//   R / F               aim up / down
//   Left Ctrl or X      fire (hold to throw further, release to throw)
//   V or right mouse    look through the rifle sight (guns); the mouse
//                       then turns the hog and raises or lowers its aim
//   Tab / Shift+Tab     another hog of the team (before moving)
//   Return              end the turn (after the match: back to the menu)
//   P, and always Esc   pause menu (continue / abort the match)
//   Mouse wheel         camera nearer or farther
//   (gamepads: see Gamepads.h)
//
// The camera stays behind the active hog, as the owner chose (decision 15).
//
// During the turns of the computer's teams (GameOptions::computerTeams)
// the keys that control the hog do nothing.
//   F12                 save a screenshot (openhogs-<map>.png)

#pragma once

#include "Gamepads.h"
#include "Interface.h"
#include "KeyboardControls.h"
#include "MatchAudio.h"
#include "OriginalHud.h"
#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/ChaseCamera.h"
#include "openhogs/game/ComputerPlayer.h"
#include "openhogs/game/FixedTimestep.h"
#include "openhogs/game/GameVersion.h"
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
    bool autoSight = false;               // hold the rifle sight (for screenshots of it)
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

    // The player's keys (the CONTROLS screen, [Controls]).
    ui::KeyBindings keyBindings = ui::KeyBindings::defaults();

    // PC or PlayStation rules (game/GameVersion.h): whether the computer's
    // hogs walk and the turn tunes loop. Set by the application.
    game::VersionRules versionRules;
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
    // Promotion points picked up on the map (campaign missions).
    int promotionPoints() const { return promotionPoints_; }

private:
    // What a team's hogs are drawn with.
    struct TeamLook {
        std::unique_ptr<render::CharacterTextureSet> textures;
        std::vector<glm::uvec2> textureSizes;
        scene::FaceTextureSlots faces;
    };

    // What a key or pad button asks the match to do.
    enum class Command { None, Pause, NextHog, PreviousHog, EndTurn, Jump, PreviousWeapon, NextWeapon };
    Command commandForKey(const SDL_Event& event) const;
    static Command commandForPad(Uint8 button);
    GameRequest handleCommand(Command command);
    GameRequest handlePauseInput(std::optional<ui::MenuInput> input);
    game::PlayerInput readInput();
    void step(double seconds);
    void handleEvents(const std::vector<game::GameEvent>& events);
    void pickedUp(const game::GameEvent& event);
    void objectiveDone(const game::GameEvent& event);
    void drawWeapons(const render::Camera& camera, int width, int height);
    void drawEffects(const render::Camera& camera, int width, int height);
    void drawModel(const render::Camera& camera, int width, int height, const formats::Model& model,
                   const glm::mat4& modelToWorld);
    const formats::Model* weaponModel(std::string_view name);
    render::Camera sceneCamera() const;
    void drawScene(int width, int height);
    void drawHud(int width, int height);
    void showMessage(const std::string& message, double seconds, bool turnMessage);
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
    glm::vec4 bannerColour_{1.0f};  // turn messages in the team's colour, others white
    OriginalHud hud_;
    std::set<int> computerTeams_;
    std::vector<std::size_t> squadDeaths_;
    int promotionPoints_ = 0;
    game::ComputerPlayer computer_;
    bool autoWalk_ = false;
    bool autoSight_ = false;
    std::optional<double> autoFireAt_;
    KeyboardControls keyboard_;
    bool jumpPressed_ = false;  // jump was pressed since the last step
    int weaponStep_ = 0;        // previous / next weapon pressed since the last step
    // Mouse movement through the rifle sight since the last step, in
    // degrees: turning (left positive) and raising the aim.
    float sightLookYaw_ = 0.0f;
    float sightLookPitch_ = 0.0f;
    void lockMouseForSight(bool sighting);

    Interface& interface_;
    SoundPlayer& sounds_;
    const Gamepads& gamepads_;
    MatchAudio audio_;  // voices, music and background of the match

    // GPU resources (the application keeps the OpenGL context alive).
    std::unique_ptr<render::MapRenderer> mapRenderer_;
    // Where the map first placed each building or vehicle hogs can get
    // into, to place the driven ones from (map object -> transform).
    std::map<std::size_t, glm::mat4> mountTransforms_;
    void placeMounts();
    std::unique_ptr<render::CharacterRenderer> hogRenderer_;
    std::unique_ptr<render::EffectsRenderer> effects_;
    std::unique_ptr<render::CharacterTextureSet> weaponTextures_;
    std::vector<glm::uvec2> weaponTextureSizes_;
    std::map<std::string, std::optional<formats::Model>, std::less<>> weaponModels_;  // empty: missing
    std::map<std::string, formats::Model, std::less<>> models_;  // by class id
    std::map<int, TeamLook> teamLooks_;
};

} // namespace openhogs::gameapp
