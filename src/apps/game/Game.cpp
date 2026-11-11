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

    if (options.startHog) {
        if (*options.startHog >= world_.hogs().size()) {
            throw std::runtime_error("Map " + mapName_ + " has only " + std::to_string(world_.hogs().size()) +
                                     " hogs");
        }
        while (world_.selectedHog() != options.startHog) {
            world_.selectNextHog(1);
        }
    }

    // Start behind the selected hog, looking the way it faces.
    if (const auto selected = world_.selectedHog()) {
        camera_.lookAlong(game::hogFacing(world_.hogs()[*selected]));
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

        mixer_ = std::make_unique<audio::Mixer>(kAudioOutputRate);
        mixer_->setVolumes(volumes);
        mixer_->play(ambience, audio::SoundCategory::Effects, 1.0f, true);
        audioDevice_ = std::make_unique<audio::AudioDevice>(*mixer_);
    } catch (const std::exception& error) {
        // No sound card, or a missing sound file: play without sound.
        std::cerr << "Warning: no sound (" << error.what() << ")\n";
        audioDevice_.reset();
        mixer_.reset();
    }
}

glm::vec3 Game::cameraTarget() const {
    if (const auto selected = world_.selectedHog()) {
        return world_.hogs()[*selected].position + glm::vec3(0.0f, kCameraTargetHeight, 0.0f);
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
    if (const auto selected = world_.selectedHog()) {
        const auto& hog = world_.hogs()[*selected];
        char text[160];
        std::snprintf(text, sizeof(text), " - hog %zu/%zu: %s, team %d (%s)%s", *selected + 1, world_.hogs().size(),
                      std::string(hog.characterClass->name).c_str(), hog.team + 1,
                      std::string(game::teamNation(hog.team)).c_str(), paused_ ? " - paused" : "");
        title += text;
    } else {
        title += " - no hogs on this map";
    }
    window_.setTitle(title);
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
            world_.selectNextHog((event.key.mod & SDL_KMOD_SHIFT) ? -1 : 1);
            updateTitle();
            break;
        case SDLK_P:
            paused_ = !paused_;
            updateTitle();
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

void Game::step(double seconds) {
    if (!paused_) {
        world_.update(seconds, library_.animations());
    }
    camera_.update(seconds, cameraTarget());  // the camera works while paused too
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

    const auto pose = hog.animator.pose(library_.skeleton(), library_.animations());
    auto mesh = scene::buildPosedModelMesh(model, look.textureSizes, pose);

    const auto expression = library_.expressionRules().forAnimation(hog.animator.animation());
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
