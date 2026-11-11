// Application.cpp - see Application.h for an overview.

#include "Application.h"

#include "MenuControls.h"

#include "openhogs/game/FieldConditions.h"
#include "openhogs/game/MatchSounds.h"

#include "openhogs/render/GlObjects.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr glm::vec4 kMenuShade{0.0f, 0.0f, 0.0f, 0.35f};  // darkens the map behind the menus
constexpr float kLoadingY = 220.0f;
constexpr std::size_t kMapListRows = 9;  // the level list scrolls beyond this

} // namespace

ui::VolumeLevels toVolumeLevels(const audio::Volumes& volumes) {
    const auto level = [](float volume) { return static_cast<int>(std::lround(volume * ui::kMaxVolumeLevel)); };
    return {level(volumes.master), level(volumes.effects), level(volumes.speech), level(volumes.music)};
}

audio::Volumes toVolumes(const ui::VolumeLevels& levels) {
    const auto volume = [](int level) { return static_cast<float>(level) / ui::kMaxVolumeLevel; };
    audio::Volumes volumes;
    volumes.master = volume(levels.master);
    volumes.effects = volume(levels.effects);
    volumes.speech = volume(levels.speech);
    volumes.music = volume(levels.music);
    return volumes;
}

Application::Application(const core::VirtualFileSystem& vfs, ApplicationOptions options)
    : vfs_(vfs), maps_(std::move(options.maps)), width_(options.width), height_(options.height),
      window_("OpenHogs", options.width, options.height, options.hiddenWindow), savePath_(options.savePath) {
    sounds_ = options.sound ? std::make_unique<SoundPlayer>(vfs, options.volumes) : std::make_unique<SoundPlayer>();
    gamepads_ = std::make_unique<Gamepads>();
    interface_ = std::make_unique<Interface>(vfs);
    frontEnd_ = std::make_unique<ui::FrontEnd>(interface_->strings(), maps_, toVolumeLevels(options.volumes));
    frontEnd_->setSavedCampaign(CampaignSession::hasSave(savePath_));
    campaign_ = std::make_unique<CampaignSession>(vfs, *interface_, savePath_);

    // The first map circles behind the menus (docs/open-decisions.md).
    if (!maps_.empty()) {
        try {
            background_ = std::make_unique<MenuBackground>(vfs, maps_.front());
        } catch (const std::exception& error) {
            std::cerr << "Warning: no menu background (" << error.what() << ")\n";
        }
    }
}

Application::~Application() {
    // Explicit order: the match and the GPU resources go before the
    // window's OpenGL context (members are destroyed in reverse order, this
    // only makes it obvious).
    game_.reset();
    gamepads_.reset();
    campaign_.reset();
    frontEnd_.reset();
    background_.reset();
    interface_.reset();
}

void Application::drawLoading() {
    // One frame with "LOADING, PLEASE WAIT." while a map loads (a second or
    // two), so the game does not look frozen.
    window_.sizeInPixels(width_, height_);
    glViewport(0, 0, width_, height_);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    auto& ui = interface_->renderer();
    ui.begin(width_, height_);
    interface_->drawBanner(interface_->strings().loading, kLoadingY);
    ui.end();
    window_.swap();
}

void Application::startMatch(const GameOptions& options) {
    drawLoading();
    game_.reset();  // free the previous match first
    game_ = std::make_unique<Game>(vfs_, options, *interface_, *sounds_, *gamepads_);
}

void Application::leaveMatch() {
    if (campaignMatch_) {
        // Back to the campaign's results page.
        campaign_->missionEnded(*game_);
        campaignMatch_ = false;
        game_.reset();
        return;
    }
    game_.reset();
    frontEnd_->showMainMenu();
}

void Application::startCampaignMission() {
    try {
        startMatch(campaign_->missionOptions());
        campaignMatch_ = true;
    } catch (const std::exception& error) {
        // A missing map must not end the game: stay on the campaign pages.
        std::cerr << "Error: cannot start the mission: " << error.what() << "\n";
        game_.reset();
    }
}

bool Application::handleMenuInput(ui::MenuInput input) {
    if (!inCampaign_) {
        return handleFrontEndResult(frontEnd_->handle(input));
    }
    switch (campaign_->handleInput(input, *sounds_)) {
    case CampaignSession::Action::StartMission:
        startCampaignMission();
        break;
    case CampaignSession::Action::Leave:
        inCampaign_ = false;
        frontEnd_->setSavedCampaign(CampaignSession::hasSave(savePath_));
        break;
    case CampaignSession::Action::None:
        break;
    }
    return true;
}

bool Application::handleFrontEndResult(const ui::FrontEndResult& result) {
    playMenuSound(*sounds_, result.sound);
    switch (result.request) {
    case ui::FrontEndResult::Request::LevelChosen: {
        std::vector<int> teams;
        try {
            teams = game::mapTeams(vfs_, result.map);
        } catch (const std::exception& error) {
            std::cerr << "Error: cannot read " << result.map << ": " << error.what() << "\n";
        }
        return handleFrontEndResult(frontEnd_->setLevelTeams(teams));
    }
    case ui::FrontEndResult::Request::StartMatch: {
        GameOptions options;
        options.mapName = result.map;
        options.computerTeams = result.settings.computerTeams;
        options.teamNations = result.settings.teamNations;
        options.turnSeconds = result.settings.turnSeconds;
        options.suddenDeathRound = result.settings.suddenDeathRound;
        const auto conditions = result.settings.conditions;
        options.prepareHogs = [conditions](std::vector<game::Hog>& hogs) { game::applyFieldConditions(hogs, conditions); };
        try {
            startMatch(options);
        } catch (const std::exception& error) {
            // A broken map must not end the game: stay on the menus.
            std::cerr << "Error: cannot start " << result.map << ": " << error.what() << "\n";
            game_.reset();
        }
        break;
    }
    case ui::FrontEndResult::Request::NewCampaign:
        campaign_->startNew(result.nation);
        inCampaign_ = true;
        break;
    case ui::FrontEndResult::Request::LoadCampaign:
        inCampaign_ = campaign_->loadSaved();
        break;
    case ui::FrontEndResult::Request::VolumesChanged:
        sounds_->setVolumes(toVolumes(frontEnd_->volumes()));
        break;
    case ui::FrontEndResult::Request::Quit:
        return false;
    case ui::FrontEndResult::Request::None:
        break;
    }
    return true;
}

bool Application::sendMenuInputs(const std::vector<ui::MenuInput>& inputs) {
    for (const auto input : inputs) {
        if (game_) {
            break;  // a match started: the remaining inputs are not for the menus
        }
        if (!handleMenuInput(input)) {
            return false;
        }
    }
    return true;
}

void Application::handleEvent(const SDL_Event& event, bool& running) {
    if (event.type == SDL_EVENT_QUIT) {
        running = false;
        return;
    }
    if (gamepads_->handleDeviceEvent(event)) {
        return;
    }
    if (game_) {
        switch (game_->handleEvent(event)) {
        case GameRequest::LeaveMatch:
            leaveMatch();
            break;
        case GameRequest::Screenshot: {
            const std::string file = "openhogs-" + game_->mapName() + ".png";
            saveScreenshot(file);
            std::cout << "Saved " << file << "\n";
            break;
        }
        case GameRequest::None:
            break;
        }
        return;
    }
    std::optional<ui::MenuInput> input;
    if (event.type == SDL_EVENT_KEY_DOWN) {
        input = menuInputForKey(event.key.key);
    } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        input = Gamepads::menuInputForButton(event.gbutton.button);
    }
    if (input) {
        running = handleMenuInput(*input);
    }
}

void Application::update(double seconds) {
    if (game_) {
        game_->advance(seconds);
        return;
    }
    // The menus play the long theme in a loop, as the original's front end.
    sounds_->keepMusic(game::kThemeTrack, 1.0f, true);
    if (background_) {
        background_->update(seconds);
    }
}

void Application::drawMenus(int width, int height) {
    const bool briefing = inCampaign_ && campaign_->showingBriefing();
    if (background_ && !briefing) {
        background_->draw(width, height);
    } else {
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    auto& ui = interface_->renderer();
    ui.begin(width, height);
    if (inCampaign_) {
        if (!briefing) ui.drawRect(0.0f, 0.0f, ui.canvasWidth(), render::kCanvasHeight, kMenuShade);
        campaign_->draw();
    } else {
        ui.drawRect(0.0f, 0.0f, ui.canvasWidth(), render::kCanvasHeight, kMenuShade);
        interface_->drawMenu(frontEnd_->menu(), kMapListRows);
    }
    ui.end();
}

void Application::draw(int width, int height) {
    if (game_) {
        game_->draw(width, height);
    } else {
        drawMenus(width, height);
    }
}

void Application::run() {
    bool running = true;
    Uint64 previous = SDL_GetTicksNS();

    while (running) {
        SDL_Event event;
        while (running && SDL_PollEvent(&event)) {
            handleEvent(event, running);
        }

        const Uint64 now = SDL_GetTicksNS();
        // Loading a map takes a while: do not count that time as play.
        const double elapsed = std::min(static_cast<double>(now - previous) / 1e9, 0.25);
        previous = now;
        update(elapsed);

        window_.sizeInPixels(width_, height_);
        draw(width_, height_);
        window_.swap();
    }
}

void Application::saveScreenshot(const std::filesystem::path& path) {
    // Off-screen, so it works even with a hidden or covered window.
    render::GlFramebuffer target(width_, height_);
    target.bind();
    draw(width_, height_);
    const auto image = render::captureFramebuffer(width_, height_);
    render::GlFramebuffer::unbind();
    apps::savePng(image, path);
}

} // namespace openhogs::gameapp
