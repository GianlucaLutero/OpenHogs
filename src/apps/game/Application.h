// Application.h
//
// The game application as a whole: the window, the sound, the 2D
// interface and the switch between the menus, the campaign pages and a
// match.
//
//     menus (ui::FrontEnd over a MenuBackground)
//        │ a level is picked                     ▲ the match is aborted or over
//        ▼                                       │
//     match (Game) ──────────────────────────────┘
//
// A campaign (CampaignSession) has its own pages between missions; its
// matches go back to those pages instead of the main menu.
//
// The window and its OpenGL context, the sound and the interface live as
// long as the application; a Game exists only during a match.

#pragma once

#include "CampaignSession.h"
#include "CutscenePlayer.h"
#include "Game.h"
#include "Interface.h"
#include "MenuBackground.h"
#include "SoundPlayer.h"

#include "openhogs/apps/GlWindow.h"
#include "openhogs/audio/Mixer.h"
#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/ui/FrontEnd.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

union SDL_Event;

namespace openhogs::gameapp {

struct ApplicationOptions {
    int width = 1280;
    int height = 720;
    bool hiddenWindow = false;  // for screenshots without a window
    bool sound = true;
    audio::Volumes volumes;
    std::vector<std::string> maps;  // the levels offered by the menu
    std::filesystem::path savePath = "saves/campaign.ini";
    bool intro = true;  // play the original's start-up videos first
};

class Application {
public:
    Application(const core::VirtualFileSystem& vfs, ApplicationOptions options);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Runs until the player quits. Starts on the menus, or straight in a
    // match if startMatch() was called first.
    void run();

    // Starts a match (shows "LOADING" while the map loads).
    void startMatch(const GameOptions& options);

    // Plays videos (Language/Streams/...) before anything else; for the
    // screenshot tool, advance them with advanceCutscenes().
    void playCutscenes(std::vector<std::string> paths);
    void advanceCutscenes(double seconds);

    // The current match, if any (for the screenshot tools).
    Game* game() { return game_.get(); }

    // Passes inputs to the menus as if keys had been pressed (for
    // screenshots of a menu page). Returns false if one asked to quit.
    bool sendMenuInputs(const std::vector<ui::MenuInput>& inputs);

    // Renders the current frame off-screen and writes it as a PNG file.
    void saveScreenshot(const std::filesystem::path& path);

private:
    void handleEvent(const SDL_Event& event, bool& running);
    bool handleMenuInput(ui::MenuInput input);  // false: quit
    void startCampaignMission();
    bool handleFrontEndResult(const ui::FrontEndResult& result);
    void update(double seconds);
    void draw(int width, int height);
    void drawMenus(int width, int height);
    void drawLoading();
    void leaveMatch();

    const core::VirtualFileSystem& vfs_;
    std::vector<std::string> maps_;
    int width_;
    int height_;

    // Declared first, destroyed last: the window owns the OpenGL context
    // every member below needs.
    apps::GlWindow window_;
    std::unique_ptr<SoundPlayer> sounds_;  // never null (silent without sound)
    std::unique_ptr<Gamepads> gamepads_;
    std::unique_ptr<Interface> interface_;
    std::unique_ptr<MenuBackground> background_;  // empty if the map failed to load
    std::unique_ptr<ui::FrontEnd> frontEnd_;
    std::unique_ptr<CampaignSession> campaign_;
    bool inCampaign_ = false;      // the campaign pages are shown (or its match played)
    bool campaignMatch_ = false;   // the match being played is a campaign mission
    std::filesystem::path savePath_;
    std::unique_ptr<Game> game_;
    std::unique_ptr<CutscenePlayer> cutscenes_;  // videos being played over everything
};

// Volume levels of the menu sliders (0..10) <-> mixer volumes (0..1).
ui::VolumeLevels toVolumeLevels(const audio::Volumes& volumes);
audio::Volumes toVolumes(const ui::VolumeLevels& levels);

} // namespace openhogs::gameapp
