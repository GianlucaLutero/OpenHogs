// Game.cpp - see Game.h for an overview.

#include "Game.h"

#include "MenuControls.h"

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
constexpr float kPadOrbitDegreesPerSecond = 120.0f;

// The camera looks at the hog's chest, this far above its feet.
constexpr float kCameraTargetHeight = 180.0f;

// Sounds of the match bank (Audio/sfxday.srl), by file name, for what happens in a match. The
// choice is OpenHogs' own (docs/open-decisions.md).
constexpr std::string_view kLandingSound = "P_LAND1";
constexpr std::string_view kSplashSound = "I_SPLASH";
constexpr std::string_view kExplosionSound = "E_1";
constexpr std::string_view kBulletHitSound = "I_BULIT1";
constexpr std::string_view kBounceSound = "P_BOUNCE";
constexpr std::string_view kDeathSound = "P_MOAN";
constexpr std::string_view kSuddenDeathSound = "E_1000P";  // the big explosion

// Effect colours.
constexpr glm::vec4 kAimColour{1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 kTracerColour{1.0f, 0.95f, 0.5f, 1.0f};
constexpr glm::vec4 kShellColour{0.25f, 0.25f, 0.2f, 1.0f};
constexpr glm::vec4 kHealthColour{0.2f, 0.85f, 0.2f, 1.0f};
constexpr glm::vec4 kActiveHealthColour{1.0f, 0.85f, 0.1f, 1.0f};
constexpr float kHealthBarHeight = 380.0f;  // above the feet

// How long the camera swings round behind a newly active hog.
constexpr double kCameraSwingSeconds = 1.5;

// HUD layout, in canvas units (the screen is render::kCanvasHeight tall).
constexpr float kDamageNumberHeight = 450.0f;  // above the feet, over the health bar
constexpr float kBannerY = 140.0f;  // the executable writes match messages at y = 140
constexpr glm::vec4 kPauseShade{0.0f, 0.0f, 0.0f, 0.4f};

// The computer opponent's settings. Its random errors are seeded from the
// clock, so it does not play every match the same way.
game::AiSettings computerSettings() {
    game::AiSettings settings;
    settings.seed = static_cast<std::uint32_t>(SDL_GetTicksNS());
    return settings;
}

game::TurnRules turnRules(const GameOptions& options) {
    game::TurnRules rules;
    if (options.turnSeconds) {
        rules.turnSeconds = *options.turnSeconds;
    }
    rules.suddenDeathRound = options.suddenDeathRound;
    return rules;
}

// How long the match messages stay on screen.
constexpr double kTurnMessageSeconds = 2.5;
constexpr double kEndMessageSeconds = -1.0;  // until the players leave

} // namespace

Game::Game(const core::VirtualFileSystem& vfs, const GameOptions& options, Interface& userInterface,
           SoundPlayer& sounds, const Gamepads& gamepads)
    : mapName_(options.mapName), library_(vfs),
      world_(game::loadWorld(vfs, options.mapName, turnRules(options), options.prepareHogs)),
      pauseMenu_(userInterface.strings()), hud_(vfs),
      computerTeams_(options.computerTeams.begin(), options.computerTeams.end()), computer_(computerSettings()),
      interface_(userInterface), sounds_(sounds), gamepads_(gamepads),
      audio_(sounds, world_, [this](int team) { return isComputerTeam(team); }) {
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
    for (const auto& [team, nation] : options.teamNations) {
        world_.setTeamNation(team, nation);
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

    if (const auto message = ui::turnMessage(world_, interface_.strings(), computerIsPlaying())) {
        showMessage(*message, kTurnMessageSeconds, true);
    }
    audio_.matchStarted();
}

Game::~Game() = default;

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
        const auto textures = library_.loadTeamTextures(scene::characterTeams()[static_cast<std::size_t>(world_.nationOf(team))]);
        TeamLook look;
        look.textures = std::make_unique<render::CharacterTextureSet>(textures.images);
        look.textureSizes = scene::textureSizes(textures.images);
        look.faces = textures.faces;
        found = teamLooks_.emplace(team, std::move(look)).first;
    }
    return found->second;
}

void Game::pause() {
    if (!paused_) {
        paused_ = true;
        pauseMenu_.reset();
    }
}

GameRequest Game::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
        if (!paused_ && (event.motion.state & SDL_BUTTON_LMASK)) {
            camera_.orbit(event.motion.xrel * kOrbitDegreesPerPixel, -event.motion.yrel * kOrbitDegreesPerPixel);
        }
        return GameRequest::None;

    case SDL_EVENT_MOUSE_WHEEL:
        if (!paused_) camera_.zoom(std::pow(kZoomStep, event.wheel.y));
        return GameRequest::None;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        pause();  // nobody is watching: stop the turn clock
        return GameRequest::None;

    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_F12) {
            return GameRequest::Screenshot;
        }
        return paused_ ? handlePauseInput(menuInputForKey(event.key.key)) : handleCommand(commandForKey(event));

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        return paused_ ? handlePauseInput(Gamepads::menuInputForButton(event.gbutton.button))
                       : handleCommand(commandForPad(event.gbutton.button));

    default:
        return GameRequest::None;
    }
}

GameRequest Game::handlePauseInput(std::optional<ui::MenuInput> input) {
    if (!input) {
        return GameRequest::None;
    }
    const auto result = pauseMenu_.handle(*input);
    playMenuSound(sounds_, result.sound);
    switch (result.request) {
    case ui::PauseResult::Request::Resume: paused_ = false; break;
    case ui::PauseResult::Request::Abort: return GameRequest::LeaveMatch;
    case ui::PauseResult::Request::None: break;
    }
    return GameRequest::None;
}

bool Game::isComputerTeam(int team) const {
    return computerTeams_.contains(team);
}

bool Game::computerIsPlaying() const {
    return world_.turns() && isComputerTeam(world_.turns()->activeTeam());
}

GameRequest Game::handleCommand(Command command) {
    if (command == Command::Pause) {
        pause();
        playMenuSound(sounds_, ui::MenuSound::Select);
        return GameRequest::None;
    }
    if (world_.phase() == game::MatchPhase::Over) {
        return command == Command::EndTurn ? GameRequest::LeaveMatch : GameRequest::None;
    }
    if (computerIsPlaying()) {
        return GameRequest::None;  // the computer has the hog
    }
    switch (command) {
    case Command::NextHog: world_.switchHog(1); break;
    case Command::PreviousHog: world_.switchHog(-1); break;
    case Command::EndTurn: world_.endTurn(); break;
    case Command::Jump: jumpPressed_ = true; break;
    case Command::PreviousWeapon: weaponStep_ = -1; break;
    case Command::NextWeapon: weaponStep_ = 1; break;
    case Command::None:
    case Command::Pause: break;
    }
    return GameRequest::None;
}

Game::Command Game::commandForKey(const SDL_Event& event) {
    switch (event.key.key) {
    case SDLK_ESCAPE:
    case SDLK_P: return Command::Pause;
    case SDLK_TAB: return (event.key.mod & SDL_KMOD_SHIFT) ? Command::PreviousHog : Command::NextHog;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return Command::EndTurn;
    case SDLK_SPACE: return Command::Jump;
    case SDLK_Q: return Command::PreviousWeapon;
    case SDLK_E: return Command::NextWeapon;
    default: return Command::None;
    }
}

Game::Command Game::commandForPad(Uint8 button) {
    switch (Gamepads::actionForButton(button)) {
    case PadAction::Pause: return Command::Pause;
    case PadAction::SwitchHog: return Command::NextHog;
    case PadAction::EndTurn: return Command::EndTurn;
    case PadAction::Jump: return Command::Jump;
    case PadAction::PreviousWeapon: return Command::PreviousWeapon;
    case PadAction::NextWeapon: return Command::NextWeapon;
    case PadAction::None: break;
    }
    return Command::None;
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
    gamepads_.addHeldControls(input);
    input.weaponStep = weaponStep_;
    jumpPressed_ = false;
    weaponStep_ = 0;
    return input;
}

void Game::handleEvents(const std::vector<game::GameEvent>& events) {
    using game::GameEventType;
    for (const auto& event : events) {
        audio_.handle(event);
        if (const auto message = ui::messageForEvent(event, world_, interface_.strings(), computerIsPlaying())) {
            showMessage(*message, event.type == GameEventType::MatchOver ? kEndMessageSeconds : kTurnMessageSeconds,
                        event.type == GameEventType::TurnStarted);
        }
        if (event.type == GameEventType::Hurt && event.hog < world_.hogs().size()) {
            const auto& hog = world_.hogs()[event.hog];
            hud_.addDamage(hog.position + glm::vec3(0.0f, kDamageNumberHeight, 0.0f), event.amount,
                           ui::teamMessageColour(world_.nationOf(hog.team)));
        }
        if (event.type == GameEventType::Died) {
            if (const auto squadIndex = world_.hogs()[event.hog].squadIndex) {
                squadDeaths_.push_back(*squadIndex);
            }
        }
        const auto playSound = [this](std::string_view name) { sounds_.play(SoundBankId::Match, name); };
        switch (event.type) {
        case GameEventType::Footstep: playSound(game::footstepSoundName(event.surface)); break;
        case GameEventType::Landed: playSound(kLandingSound); break;
        case GameEventType::Splash: playSound(kSplashSound); break;
        case GameEventType::Fired: playSound(game::weaponSpec(event.weapon).launchSound); break;
        case GameEventType::Bounce: playSound(kBounceSound); break;
        case GameEventType::BulletHit: playSound(kBulletHitSound); break;
        case GameEventType::Explosion: playSound(kExplosionSound); break;
        case GameEventType::Died: playSound(kDeathSound); break;
        case GameEventType::SuddenDeath: playSound(kSuddenDeathSound); break;
        case GameEventType::Hurt:         // voices and music: MatchAudio
        case GameEventType::TurnStarted:
        case GameEventType::MatchOver: break;
        }
    }
}

void Game::step(double seconds) {
    if (paused_) {
        return;  // the camera waits too: the pause menu covers the view
    }
    game::PlayerInput input = readInput();  // read anyway: it clears the remembered key presses
    if (computerIsPlaying()) {
        const auto decision = computer_.update(world_, seconds);
        if (decision.endTurn) {
            world_.endTurn();
        }
        input = decision.input;
    }
    handleEvents(world_.update(seconds, input, library_.animations()));
    banner_.update(seconds);
    hud_.update(seconds, ui::describeTurn(world_, interface_.strings(), interface_.gameText()));
    audio_.update(seconds);

    // When the active hog changes, or while it moves, the camera swings
    // round behind the hog.
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
    // The pads' right stick turns the camera, like dragging the mouse.
    camera_.orbit(gamepads_.cameraTurn() * kPadOrbitDegreesPerSecond * static_cast<float>(seconds), 0.0f);
    camera_.update(seconds, cameraTarget());
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
        effects_->drawHealthBar(hog.position + glm::vec3(0.0f, kHealthBarHeight, 0.0f), hog.health / hog.maxHealth,
                                active == i ? kActiveHealthColour : kHealthColour);
    }
}

render::Camera Game::sceneCamera() const {
    const auto pose = camera_.pose(world_.terrain());
    render::Camera camera;
    camera.position = pose.position;
    camera.yawDegrees = pose.yawDegrees;
    camera.pitchDegrees = pose.pitchDegrees;
    return camera;
}

void Game::drawScene(int width, int height) {
    const render::Camera camera = sceneCamera();

    render::DrawOptions options;
    options.showMarkers = false;  // the hogs themselves stand where the markers were
    mapRenderer_->draw(camera, width, height, options);

    for (std::size_t i = 0; i < world_.hogs().size(); ++i) {
        drawHog(camera, width, height, world_.hogs()[i], i);
    }
    drawWeapons(camera, width, height);
    drawEffects(camera, width, height);
}

void Game::drawHud(int width, int height) {
    // The damage numbers float in the scene, under the instruments.
    const render::Camera camera = sceneCamera();
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    hud_.drawDamageNumbers(interface_.renderer(), camera.projectionMatrix(aspect) * camera.viewMatrix(),
                           camera.position);
    hud_.draw(interface_, ui::describeTurn(world_, interface_.strings(), interface_.gameText()));
    if (const auto& message = banner_.current()) {
        // Turn messages fade out over their last quarter.
        const double progress = banner_.progress();
        const float alpha = progress > 0.75 ? static_cast<float>((1.0 - progress) / 0.25) : 1.0f;
        interface_.drawBanner(*message, kBannerY, alpha, bannerColour_);
    }
}

void Game::showMessage(const std::string& message, double seconds, bool turnMessage) {
    // As in the original, "GET READY ..." and "THE ENEMY RETALIATES..." are
    // written in the colour of the team about to play, the other messages
    // in white.
    bannerColour_ = glm::vec4(1.0f);
    if (turnMessage && world_.turns()) {
        bannerColour_ = ui::teamMessageColour(world_.nationOf(world_.turns()->activeTeam()));
    }
    banner_.show(message, seconds);
}

void Game::advance(double elapsedSeconds) {
    const int steps = timestep_.advance(elapsedSeconds);
    for (int i = 0; i < steps; ++i) {
        step(timestep_.stepSeconds());
    }
}

void Game::draw(int width, int height) {
    drawScene(width, height);

    auto& ui = interface_.renderer();
    ui.begin(width, height);
    if (paused_) {
        ui.drawRect(0.0f, 0.0f, ui.canvasWidth(), render::kCanvasHeight, kPauseShade);
        interface_.drawMenu(pauseMenu_.menu());
    } else {
        drawHud(width, height);
    }
    ui.end();
}

} // namespace openhogs::gameapp
