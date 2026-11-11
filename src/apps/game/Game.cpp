// Game.cpp - see Game.h for an overview.

#include "Game.h"

#include "openhogs/formats/TextTable.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/scene/CharacterExpressions.h"
#include "openhogs/scene/MapScene.h"
#include "openhogs/scene/ModelMesh.h"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
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

// Sounds of the same bank, by file name, for what happens in a match. The
// choice is OpenHogs' own (docs/open-decisions.md).
constexpr std::string_view kLandingSound = "P_LAND1";
constexpr std::string_view kSplashSound = "I_SPLASH";
constexpr std::string_view kExplosionSound = "E_1";
constexpr std::string_view kBulletHitSound = "I_BULIT1";
constexpr std::string_view kBounceSound = "P_BOUNCE";
constexpr std::string_view kHurtSound = "P_OWW";
constexpr std::string_view kDeathSound = "P_MOAN";
constexpr std::string_view kTurnSound = "S_SELECT";
constexpr std::string_view kVictorySound = "P_WHOHO";

// Effect colours.
constexpr glm::vec4 kAimColour{1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 kTracerColour{1.0f, 0.95f, 0.5f, 1.0f};
constexpr glm::vec4 kShellColour{0.25f, 0.25f, 0.2f, 1.0f};
constexpr glm::vec4 kHealthColour{0.2f, 0.85f, 0.2f, 1.0f};
constexpr glm::vec4 kActiveHealthColour{1.0f, 0.85f, 0.1f, 1.0f};
constexpr float kHealthBarHeight = 380.0f;  // above the feet

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
    effects_ = std::make_unique<render::EffectsRenderer>();
    try {
        const auto textures = library_.loadWeaponTextures();
        weaponTextures_ = std::make_unique<render::CharacterTextureSet>(textures);
        weaponTextureSizes_ = scene::textureSizes(textures);
    } catch (const std::exception& error) {
        std::cerr << "Warning: no weapon models (" << error.what() << ")\n";
    }
    try {
        const auto table = formats::TextTable::parse(vfs.readFile("Language/Text/gtext.ofs"),
                                                     vfs.readFile("Language/Text/gtext.bin"));
        gameText_ = table.entries;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no game text, weapon names in English (" << error.what() << ")\n";
    }

    autoWalk_ = options.autoWalk;
    autoFireAt_ = options.autoFireAt;
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

    if (options.autoWeaponSlot) {
        // Cycle to the requested weapon through the normal controls.
        for (std::size_t i = 0; i < *options.autoWeaponSlot; ++i) {
            game::PlayerInput select;
            select.weaponStep = 1;
            world_.update(0.0, select, library_.animations());
        }
    }

    if (options.sound) {
        startSound(vfs, options.volumes);
    }
    updateTitle();
}

Game::~Game() {
    audioDevice_.reset();  // stops the sound thread before the mixer goes
    teamLooks_.clear();    // GPU resources must go before the window's context
    weaponTextures_.reset();
    effects_.reset();
    hogRenderer_.reset();
    mapRenderer_.reset();
}

void Game::startSound(const core::VirtualFileSystem& vfs, const audio::Volumes& volumes) {
    try {
        sounds_ = std::make_unique<audio::SoundLibrary>(vfs);
        soundBank_ = sounds_->loadBank(kAmbientBank);
        const auto ambience = sounds_->loadFromBank(*soundBank_, kAmbientSound);

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

void Game::playSound(std::string_view name) {
    if (!mixer_ || !soundBank_ || name.empty()) {
        return;
    }
    const std::string wanted = soundStem(std::string(name));
    auto found = loadedSounds_.find(wanted);
    if (found == loadedSounds_.end()) {
        std::shared_ptr<const formats::PcmSound> sound;
        for (std::size_t i = 0; i < soundBank_->files.size(); ++i) {
            if (soundStem(soundBank_->files[i]) == wanted) {
                try {
                    sound = sounds_->loadFromBank(*soundBank_, i);
                } catch (const std::exception& error) {
                    std::cerr << "Warning: " << error.what() << "\n";
                }
                break;
            }
        }
        if (!sound) {
            std::cerr << "Warning: sound " << name << " not found in " << kAmbientBank << "\n";
        }
        found = loadedSounds_.emplace(wanted, sound).first;  // remembered even if missing
    }
    if (found->second) {
        mixer_->play(found->second, audio::SoundCategory::Effects);
    }
}

std::string Game::weaponName(game::WeaponId weapon) const {
    const auto index = static_cast<std::size_t>(weapon);
    if (index < gameText_.size() && !gameText_[index].empty()) {
        return gameText_[index];
    }
    return std::string(game::weaponSpec(weapon).fallbackName);
}

const formats::Model* Game::weaponModel(std::string_view name) {
    if (name.empty() || !weaponTextures_) {
        return nullptr;
    }
    auto found = weaponModels_.find(name);
    if (found == weaponModels_.end()) {
        std::optional<formats::Model> model;
        try {
            model = library_.loadWeaponModel(name);
        } catch (const std::exception& error) {
            std::cerr << "Warning: " << error.what() << "\n";
        }
        found = weaponModels_.emplace(std::string(name), std::move(model)).first;
    }
    return found->second ? &*found->second : nullptr;
}

glm::vec3 Game::cameraTarget() const {
    // While a shot is in the air, follow it.
    if (world_.phase() == game::MatchPhase::Resolving) {
        for (const auto& projectile : world_.projectiles()) {
            const auto kind = game::weaponSpec(projectile.weapon).kind;
            if (kind != game::WeaponKind::Placed && !projectile.resting) {
                return projectile.position;
            }
        }
    }
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
    if (world_.phase() == game::MatchPhase::Over) {
        const auto winner = world_.winner();
        title += winner ? " - VICTORY TO TEAM " + std::to_string(*winner + 1) + " (" +
                              std::string(game::teamNation(*winner)) + ")!"
                        : " - IT'S A DRAW!";
    } else if (turns) {
        const auto& hog = world_.hogs()[turns->activeHog()];
        const auto allowance = turns->rules().movementAllowance;
        const int movementPercent =
            allowance > 0.0f ? static_cast<int>(std::lround(100.0f * turns->movementLeft() / allowance)) : 0;
        char text[320];
        std::snprintf(text, sizeof(text), " - turn %d - team %d (%s), %s, health %d - %s%s - %d s - movement %d%%%s%s",
                      turns->turnNumber(), turns->activeTeam() + 1, std::string(game::teamNation(hog.team)).c_str(),
                      std::string(hog.characterClass->name).c_str(), static_cast<int>(std::ceil(hog.health)),
                      weaponName(hog.selectedWeapon()).c_str(),
                      hog.charging ? (" " + std::to_string(static_cast<int>(hog.power * 100.0f)) + "%").c_str() : "",
                      static_cast<int>(std::ceil(std::max(0.0, turns->secondsLeft()))), movementPercent,
                      turns->canSwitchHog() && world_.phase() == game::MatchPhase::Playing ? " - Tab: other hog" : "",
                      paused_ ? " - paused" : "");
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
        case SDLK_Q:
            weaponStep_ = -1;
            break;
        case SDLK_E:
            weaponStep_ = 1;
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

game::PlayerInput Game::readInput() {
    game::PlayerInput input;
    // Keys held down are read directly; key presses (jump, weapon change)
    // are remembered by handleEvent until the next step.
    const bool* keys = SDL_GetKeyboardState(nullptr);
    input.movement.forward = autoWalk_ || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    input.movement.backward = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    input.movement.turnLeft = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.movement.turnRight = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.movement.jump = jumpPressed_;
    input.aimUp = keys[SDL_SCANCODE_R];
    input.aimDown = keys[SDL_SCANCODE_F];
    input.fire = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] || keys[SDL_SCANCODE_X];
    if (autoFireAt_) {
        // Screenshot aid: hold fire for a while (half power for throws).
        constexpr double kAutoFireHoldSeconds = 0.75;
        const double t = world_.timeSeconds();
        input.fire = t >= *autoFireAt_ && t < *autoFireAt_ + kAutoFireHoldSeconds;
    }
    input.weaponStep = weaponStep_;
    jumpPressed_ = false;
    weaponStep_ = 0;
    return input;
}

void Game::playSounds(const std::vector<game::GameEvent>& events) {
    using game::GameEventType;
    for (const auto& event : events) {
        switch (event.type) {
        case GameEventType::Footstep: playSound(game::footstepSoundName(event.surface)); break;
        case GameEventType::Landed: playSound(kLandingSound); break;
        case GameEventType::Splash: playSound(kSplashSound); break;
        case GameEventType::Fired: playSound(game::weaponSpec(event.weapon).launchSound); break;
        case GameEventType::Bounce: playSound(kBounceSound); break;
        case GameEventType::BulletHit: playSound(kBulletHitSound); break;
        case GameEventType::Explosion: playSound(kExplosionSound); break;
        case GameEventType::Hurt: playSound(kHurtSound); break;
        case GameEventType::Died: playSound(kDeathSound); break;
        case GameEventType::TurnStarted: playSound(kTurnSound); break;
        case GameEventType::MatchOver: playSound(kVictorySound); break;
        }
    }
}

void Game::step(double seconds) {
    if (!paused_) {
        playSounds(world_.update(seconds, readInput(), library_.animations()));
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
        const bool moving = world_.phase() == game::MatchPhase::Playing &&
                            (hog.motion.state == game::MotionState::Running ||
                             hog.motion.state == game::MotionState::Swimming ||
                             hog.motion.state == game::MotionState::Airborne);
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

    // The active hog shows its selected weapon. Like the body, weapon
    // models say which bone each vertex follows: pistols the right hand,
    // rifles the right forearm, grenades and launchers the hips (worn at
    // the belt or on the back). See docs/formats/characters.md.
    if (world_.activeHog() == index && hog.alive && world_.phase() != game::MatchPhase::Over && weaponTextures_) {
        if (const auto* weapon = weaponModel(game::weaponSpec(hog.selectedWeapon()).model)) {
            const auto weaponMesh = scene::buildPosedModelMesh(*weapon, weaponTextureSizes_, pose);
            hogRenderer_->drawHog(camera, width, height, weaponMesh, *weaponTextures_, game::hogModelToWorld(hog));
        }
    }
}

void Game::drawModel(const render::Camera& camera, int width, int height, const formats::Model& model,
                     const glm::mat4& modelToWorld) {
    const auto mesh = scene::buildModelMesh(model, weaponTextureSizes_);
    hogRenderer_->drawHog(camera, width, height, mesh, *weaponTextures_, modelToWorld);
}

void Game::drawWeapons(const render::Camera& camera, int width, int height) {
    // Grenades, TNT and mines are drawn with their models; shells (whose
    // own models are not known) as dark dots.
    std::vector<glm::vec3> shells;
    const float scale = formats::kModelToWorldScale;
    for (const auto& projectile : world_.projectiles()) {
        const auto& spec = game::weaponSpec(projectile.weapon);
        const formats::Model* model =
            spec.kind == game::WeaponKind::Shell ? nullptr : weaponModel(spec.model);
        if (model == nullptr) {
            shells.push_back(projectile.position);
            continue;
        }
        const glm::mat4 toWorld = glm::scale(glm::translate(glm::mat4(1.0f), projectile.position),
                                             glm::vec3(scale, -scale, scale));
        drawModel(camera, width, height, *model, toWorld);
    }
    effects_->begin(camera, width, height);
    effects_->drawDots(shells, 30.0f, kShellColour);
}

void Game::drawEffects(const render::Camera& camera, int width, int height) {
    effects_->begin(camera, width, height);

    effects_->drawDots(world_.aimPreview(), 14.0f, kAimColour, true);

    for (const auto& tracer : world_.tracers()) {
        std::vector<glm::vec3> dots;
        const float length = glm::length(tracer.to - tracer.from);
        for (float along = 0.0f; along < length; along += 60.0f) {
            dots.push_back(tracer.from + (tracer.to - tracer.from) * (along / length));
        }
        effects_->drawDots(dots, 8.0f, kTracerColour);
    }
    for (const auto& explosion : world_.explosions()) {
        effects_->drawExplosion(explosion.position, explosion.radius,
                                explosion.ageSeconds / game::kExplosionEffectSeconds);
    }
    const auto active = world_.activeHog();
    for (std::size_t i = 0; i < world_.hogs().size(); ++i) {
        const auto& hog = world_.hogs()[i];
        if (!hog.alive) continue;
        effects_->drawHealthBar(hog.position + glm::vec3(0.0f, kHealthBarHeight, 0.0f), hog.health / game::kMaxHealth,
                                active == i ? kActiveHealthColour : kHealthColour);
    }
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
    drawWeapons(camera, width, height);
    drawEffects(camera, width, height);
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
