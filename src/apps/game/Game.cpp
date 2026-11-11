// Game.cpp - see Game.h for an overview.

#include "Game.h"

#include "openhogs/scene/CharacterExpressions.h"
#include "openhogs/scene/MapScene.h"
#include "openhogs/scene/ModelMesh.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr float kOrbitDegreesPerPixel = 0.3f;
constexpr float kZoomStep = 0.9f;  // distance factor per wheel notch

// The camera looks at the hog's chest, this far above its feet.
constexpr float kCameraTargetHeight = 180.0f;

constexpr std::uint32_t kAudioOutputRate = 48000;

// The background sound of a match: the first sound of the daytime bank
// (Audio/AMB_1D.wav), played in a loop.
const char* const kAmbientBank = "Audio/sfxday.srl";
constexpr std::size_t kAmbientSound = 0;
constexpr float kAmbientVolume = 0.6f;  // under the footsteps

// Sounds of the same bank for landing and splashing, found by file name.
constexpr std::string_view kLandingSoundName = "P_LAND1";
constexpr std::string_view kSplashSoundName = "I_SPLASH";

// How long the camera swings round behind a newly active hog.
constexpr double kCameraSwingSeconds = 1.5;

// File name without folder and extension: "AUDIO\\FT_GRASS.wav" -> "ft_grass".
std::string soundStem(const std::string& path) {
    const auto slash = path.find_last_of("\\/");
    std::string stem = path.substr(slash == std::string::npos ? 0 : slash + 1);
    const auto dot = stem.rfind('.');
    if (dot != std::string::npos) {
        stem.resize(dot);
    }
    for (char& c : stem) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return stem;
}

} // namespace

Game::Game(const core::VirtualFileSystem& vfs, const GameOptions& options)
    : mapName_(options.mapName), library_(vfs), world_(game::loadWorld(vfs, options.mapName)),
      window_("OpenHogs", options.width, options.height, options.hiddenWindow), width_(options.width),
      height_(options.height) {
    const auto scene = scene::loadMapScene(vfs, options.mapName);
    for (const auto& warning : scene.warnings) {
        std::cerr << "Warning: " << warning << "\n";
    }
    mapRenderer_ = std::make_unique<render::MapRenderer>(scene);
    hogRenderer_ = std::make_unique<render::CharacterRenderer>();

    autoWalk_ = options.autoWalk;
    if (options.startHog) {
        if (*options.startHog >= world_.hogs().size()) {
            throw std::runtime_error("Map " + mapName_ + " has only " + std::to_string(world_.hogs().size()) +
                                     " hogs");
        }
        // Only hogs of the team that starts can be picked.
        for (std::size_t tries = 0; tries < world_.hogs().size() && world_.activeHog() != options.startHog; ++tries) {
            world_.switchHog(1);
        }
        if (world_.activeHog() != options.startHog) {
            std::cerr << "Warning: hog " << *options.startHog + 1 << " is not in the team that plays first\n";
        }
    }

    // Start behind the active hog, looking the way it faces.
    if (const auto active = world_.activeHog()) {
        camera_.lookAlong(game::hogFacing(world_.hogs()[*active]));
        cameraHog_ = active;
    }
    camera_.snapTo(cameraTarget());

    if (options.sound) {
        startSound(vfs, options.volumes);
    }
    updateTitle();
}

Game::~Game() {
    audioDevice_.reset();  // stops the sound thread before the mixer goes
    teamLooks_.clear();    // GPU resources must go before the window's context
    hogRenderer_.reset();
    mapRenderer_.reset();
}

void Game::startSound(const core::VirtualFileSystem& vfs, const audio::Volumes& volumes) {
    try {
        audio::SoundLibrary sounds(vfs);
        const auto ambience = sounds.loadFromBank(sounds.loadBank(kAmbientBank), kAmbientSound);

        // Footsteps, landing and splash sounds, found by file name in the
        // same bank. A missing one is simply not played.
        const auto bank = sounds.loadBank(kAmbientBank);
        const auto findSound = [&](std::string_view name) -> std::shared_ptr<const formats::PcmSound> {
            const std::string wanted = soundStem(std::string(name));
            for (std::size_t i = 0; i < bank.files.size(); ++i) {
                if (soundStem(bank.files[i]) == wanted) {
                    return sounds.loadFromBank(bank, i);
                }
            }
            std::cerr << "Warning: sound " << name << " not found in " << kAmbientBank << "\n";
            return nullptr;
        };
        for (int surface = 0; surface <= static_cast<int>(formats::SurfaceType::Lava); ++surface) {
            const auto type = static_cast<formats::SurfaceType>(surface);
            footstepSounds_[type] = findSound(game::footstepSoundName(type));
        }
        landingSound_ = findSound(kLandingSoundName);
        splashSound_ = findSound(kSplashSoundName);

        mixer_ = std::make_unique<audio::Mixer>(kAudioOutputRate);
        mixer_->setVolumes(volumes);
        mixer_->play(ambience, audio::SoundCategory::Effects, kAmbientVolume, true);
        audioDevice_ = std::make_unique<audio::AudioDevice>(*mixer_);
    } catch (const std::exception& error) {
        // No sound card, or a missing sound file: play without sound.
        std::cerr << "Warning: no sound (" << error.what() << ")\n";
        audioDevice_.reset();
        mixer_.reset();
    }
}

glm::vec3 Game::cameraTarget() const {
    if (const auto active = world_.activeHog()) {
        return world_.hogs()[*active].position + glm::vec3(0.0f, kCameraTargetHeight, 0.0f);
    }
    // A map without hogs: look at its centre.
    const auto& terrain = world_.terrain();
    const float x = (terrain.minX() + terrain.maxX()) / 2.0f;
    const float z = (terrain.minZ() + terrain.maxZ()) / 2.0f;
    return {x, terrain.heightAt(x, z), z};
}

const formats::Model& Game::modelFor(const scene::CharacterClass& characterClass) {
    auto found = models_.find(characterClass.id);
    if (found == models_.end()) {
        found = models_.emplace(std::string(characterClass.id),
                                library_.loadModel(characterClass, scene::CharacterDetail::High)).first;
    }
    return found->second;
}

const Game::TeamLook& Game::lookFor(int team) {
    auto found = teamLooks_.find(team);
    if (found == teamLooks_.end()) {
        const auto textures = library_.loadTeamTextures(game::teamNation(team));
        TeamLook look;
        look.textures = std::make_unique<render::CharacterTextureSet>(textures.images);
        look.textureSizes = scene::textureSizes(textures.images);
        look.faces = textures.faces;
        found = teamLooks_.emplace(team, std::move(look)).first;
    }
    return found->second;
}

void Game::updateTitle() {
    std::string title = "OpenHogs - " + mapName_;
    const auto& turns = world_.turns();
    if (turns) {
        const auto& hog = world_.hogs()[turns->activeHog()];
        const auto allowance = turns->rules().movementAllowance;
        const int movementPercent =
            allowance > 0.0f ? static_cast<int>(std::lround(100.0f * turns->movementLeft() / allowance)) : 0;
        char text[200];
        std::snprintf(text, sizeof(text), " - turn %d - team %d (%s), %s - %d s left - movement %d%%%s%s",
                      turns->turnNumber(), turns->activeTeam() + 1, std::string(game::teamNation(hog.team)).c_str(),
                      std::string(hog.characterClass->name).c_str(),
                      static_cast<int>(std::ceil(std::max(0.0, turns->secondsLeft()))), movementPercent,
                      turns->canSwitchHog() ? " - Tab: other hog" : "", paused_ ? " - paused" : "");
        title += text;
    } else {
        title += " - no hogs on this map";
    }
    if (title != lastTitle_) {  // changing the title every frame is wasteful
        window_.setTitle(title);
        lastTitle_ = title;
    }
}

void Game::handleEvent(const SDL_Event& event, bool& running) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        running = false;
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (event.motion.state & SDL_BUTTON_LMASK) {
            camera_.orbit(event.motion.xrel * kOrbitDegreesPerPixel, -event.motion.yrel * kOrbitDegreesPerPixel);
        }
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        camera_.zoom(std::pow(kZoomStep, event.wheel.y));
        break;

    case SDL_EVENT_KEY_DOWN:
        switch (event.key.key) {
        case SDLK_ESCAPE:
            running = false;
            break;
        case SDLK_TAB:
            world_.switchHog((event.key.mod & SDL_KMOD_SHIFT) ? -1 : 1);
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!paused_) world_.endTurn();
            break;
        case SDLK_SPACE:
            jumpPressed_ = true;
            break;
        case SDLK_P:
            paused_ = !paused_;
            break;
        case SDLK_F12: {
            const std::string file = "openhogs-" + mapName_ + ".png";
            saveScreenshot(file);
            std::cout << "Saved " << file << "\n";
            break;
        }
        default:
            break;
        }
        break;

    default:
        break;
    }
}

game::MovementInput Game::readInput() {
    game::MovementInput input;
    // Keys held down are read directly; a jump is a key press, remembered
    // by handleEvent until the next step.
    const bool* keys = SDL_GetKeyboardState(nullptr);
    input.forward = autoWalk_ || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.backward = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.turnLeft = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.turnRight = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.jump = jumpPressed_;
    jumpPressed_ = false;
    return input;
}

void Game::playSounds(const game::MovementEvents& events) {
    if (!mixer_) {
        return;
    }
    const auto play = [&](const std::shared_ptr<const formats::PcmSound>& sound) {
        if (sound) mixer_->play(sound, audio::SoundCategory::Effects);
    };
    for (int i = 0; i < events.footsteps; ++i) {
        const auto found = footstepSounds_.find(events.surface);
        if (found != footstepSounds_.end()) play(found->second);
    }
    if (events.landed) play(landingSound_);
    if (events.enteredWater) play(splashSound_);
}

void Game::step(double seconds) {
    if (!paused_) {
        const auto events = world_.update(seconds, readInput(), library_.animations());
        playSounds(events);
    }

    // The camera works while paused too. When the active hog changes, or
    // while it moves, it swings round behind the hog.
    const auto active = world_.activeHog();
    if (active != cameraHog_) {
        cameraHog_ = active;
        cameraSwingSecondsLeft_ = kCameraSwingSeconds;
    }
    if (active) {
        const auto& hog = world_.hogs()[*active];
        const bool moving = hog.motion.state == game::MotionState::Running ||
                            hog.motion.state == game::MotionState::Swimming ||
                            hog.motion.state == game::MotionState::Airborne;
        if (moving || cameraSwingSecondsLeft_ > 0.0) {
            camera_.turnTowards(game::hogFacing(hog), seconds);
        }
    }
    cameraSwingSecondsLeft_ = std::max(0.0, cameraSwingSecondsLeft_ - seconds);
    camera_.update(seconds, cameraTarget());
    updateTitle();
}

void Game::simulate(double seconds) {
    const int steps = static_cast<int>(seconds / timestep_.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        step(timestep_.stepSeconds());
    }
}

void Game::drawHog(const render::Camera& camera, int width, int height, const game::Hog& hog, std::size_t index) {
    const auto& model = modelFor(*hog.characterClass);
    const auto& look = lookFor(hog.team);

    const auto pose = hog.animation.pose(library_.skeleton(), library_.animations());
    auto mesh = scene::buildPosedModelMesh(model, look.textureSizes, pose);

    const auto expression = library_.expressionRules().forAnimation(hog.animation.animation());
    const bool blinking = scene::isBlinking(world_.timeSeconds(), static_cast<std::uint32_t>(index));
    scene::applyExpression(mesh, look.faces, expression, blinking, hog.characterClass->masked);

    hogRenderer_->drawHog(camera, width, height, mesh, *look.textures, game::hogModelToWorld(hog));
}

void Game::draw(int width, int height) {
    const auto pose = camera_.pose(world_.terrain());
    render::Camera camera;
    camera.position = pose.position;
    camera.yawDegrees = pose.yawDegrees;
    camera.pitchDegrees = pose.pitchDegrees;

    render::DrawOptions options;
    options.showMarkers = false;  // the hogs themselves stand where the markers were
    mapRenderer_->draw(camera, width, height, options);

    for (std::size_t i = 0; i < world_.hogs().size(); ++i) {
        drawHog(camera, width, height, world_.hogs()[i], i);
    }
}

void Game::run() {
    bool running = true;
    Uint64 previous = SDL_GetTicksNS();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event, running);
        }

        const Uint64 now = SDL_GetTicksNS();
        const double elapsed = static_cast<double>(now - previous) / 1e9;
        previous = now;
        const int steps = timestep_.advance(elapsed);
        for (int i = 0; i < steps; ++i) {
            step(timestep_.stepSeconds());
        }

        window_.sizeInPixels(width_, height_);
        draw(width_, height_);
        window_.swap();
    }
}

void Game::saveScreenshot(const std::filesystem::path& path) {
    // Off-screen, so it works even with a hidden or covered window.
    render::GlFramebuffer target(width_, height_);
    target.bind();
    draw(width_, height_);
    const auto image = render::captureFramebuffer(width_, height_);
    render::GlFramebuffer::unbind();
    apps::savePng(image, path);
}

} // namespace openhogs::gameapp
