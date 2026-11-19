// Game.cpp - see Game.h for an overview.

#include "Game.h"

#include "MenuControls.h"

#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/scene/CharacterExpressions.h"
#include "openhogs/scene/MapScene.h"
#include "openhogs/scene/ModelMesh.h"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/gl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>

namespace openhogs::gameapp {

namespace {

constexpr float kZoomStep = 0.9f;  // distance factor per wheel notch
// Through the rifle sight the mouse turns the hog and raises its aim, this
// many degrees per pixel (the 25-degree view makes coarser steps jumpy).
constexpr float kSightLookDegreesPerPixel = 0.08f;

// The camera looks at the hog's chest, this far above its feet.
constexpr float kCameraTargetHeight = 180.0f;
// The rifle sight magnifies: its view is this narrow (the ordinary
// camera's is 60 degrees; decision 131).
constexpr float kSightFieldOfViewDegrees = 25.0f;
constexpr float kBinocularFieldOfViewDegrees = 12.0f;

// Sounds of the match bank (Audio/sfxday.srl), by file name, for what happens in a match. The
// choice is OpenHogs' own (docs/open-decisions.md).
constexpr std::string_view kLandingSound = "P_LAND1";
constexpr std::string_view kSplashSound = "I_SPLASH";
constexpr std::string_view kExplosionSound = "E_1";
constexpr std::string_view kBulletHitSound = "I_BULIT1";
constexpr std::string_view kBounceSound = "P_BOUNCE";
constexpr std::string_view kDeathSound = "P_MOAN";
constexpr std::string_view kSuddenDeathSound = "E_1000P";  // the big explosion
constexpr std::string_view kPickupSound = "I_PICKUP";

// Effect colours.
constexpr glm::vec4 kAimColour{1.0f, 1.0f, 1.0f, 1.0f};
constexpr glm::vec4 kTracerColour{1.0f, 0.95f, 0.5f, 1.0f};
constexpr glm::vec4 kShellColour{0.25f, 0.25f, 0.2f, 1.0f};
// The label over the hog in play (its name and health) starts this far
// above its feet, just over its head.
constexpr float kHogLabelHeight = 330.0f;

// HUD layout, in canvas units (the screen is render::kCanvasHeight tall).
constexpr float kDamageNumberHeight = 450.0f;  // above the feet, by the label
constexpr float kBannerY = 140.0f;  // the executable writes match messages at y = 140
constexpr glm::vec4 kPauseShade{0.0f, 0.0f, 0.0f, 0.4f};

// The computer opponent's settings. Its random errors are seeded from the
// clock, so it does not play every match the same way.
game::AiSettings computerSettings(const game::VersionRules& rules) {
    game::AiSettings settings;
    settings.seed = static_cast<std::uint32_t>(SDL_GetTicksNS());
    settings.walks = rules.computerWalks;  // the PC's computer hogs never walk
    return settings;
}

game::TurnRules turnRules(const GameOptions& options) {
    game::TurnRules rules;
    if (options.turnSeconds) {
        rules.turnSeconds = *options.turnSeconds;
    }
    rules.suddenDeathRound = options.suddenDeathRound;
    rules.deathmatchLimit = options.deathmatchLimit;
    rules.selectPig = options.selectPig;
    return rules;
}

// How long the match messages stay on screen.
constexpr double kTurnMessageSeconds = 2.5;
constexpr double kEndMessageSeconds = -1.0;  // until the players leave
// Hiding hogs' disguises are drawn as extra objects with these numbers.
constexpr std::size_t kDisguiseObject = 1'000'000;
// The boots a hog leaves when it blows up, and the parachutes of falling
// crates, drawn as map objects with numbers of their own.
constexpr std::size_t kBootsObject = 2'000'000;
constexpr std::size_t kParachuteObject = 3'000'000;
// Where a falling crate's parachute starts, above the crate's middle.
constexpr float kParachuteHeight = 96.0f;  // the crate's half height: WE_CHUT2 hangs from its origin
// Hog shadows: the blob's width on the ground and how dark it is.
constexpr float kShadowWidth = 150.0f;
constexpr float kShadowOpacity = 0.45f;
// The effect pictures (scene/Particles.h).
const char* const kEffectPictures = "Language/Tims/expltims.mad";

render::EffectPictures loadEffectPictures(const core::VirtualFileSystem& vfs) {
    const auto archive = formats::PackageArchive::parse(vfs.readFile(kEffectPictures));
    const auto picture = [&](const std::string& name) {
        const auto* entry = archive.find(name);
        if (entry == nullptr) throw std::runtime_error(name + " missing from " + kEffectPictures);
        return formats::decodeTim(archive.entryData(*entry));
    };
    render::EffectPictures pictures;
    pictures.fireball = picture("ptp0.tim");
    pictures.smoke = picture("ptp1.tim");
    pictures.foam = picture("ptp2.tim");
    pictures.scorch = picture("pcfx013.tim");
    pictures.shadow = picture("Shad_org.tim");
    for (int frame = 3; frame <= 11; ++frame) {
        pictures.clod.push_back(picture("pcfx0" + std::string(frame < 10 ? "0" : "") + std::to_string(frame) + ".tim"));
    }
    return pictures;
}

// Half the height of the BOOTS model, so it stands on the ground.
constexpr float kBootsHalfHeight = 27.0f;
constexpr float kDisguiseHalfHeight = 96.0f;
constexpr glm::vec4 kHealColour{0.3f, 1.0f, 0.3f, 1.0f};

} // namespace

Game::Game(const core::VirtualFileSystem& vfs, const GameOptions& options, Interface& userInterface,
           SoundPlayer& sounds, const Gamepads& gamepads)
    : mapName_(options.mapName), library_(vfs),
      world_(game::loadWorld(vfs, options.mapName, turnRules(options), options.prepareHogs)),
      pauseMenu_(userInterface.strings()), hud_(vfs),
      computerTeams_(options.computerTeams.begin(), options.computerTeams.end()), computer_(computerSettings(options.versionRules)),
      keyboard_(options.keyBindings), interface_(userInterface), sounds_(sounds), gamepads_(gamepads),
      audio_(sounds, world_, [this](int team) { return isComputerTeam(team); }, options.versionRules.turnTuneLoops) {
    const auto scene = scene::loadMapScene(vfs, options.mapName);
    for (const auto& warning : scene.warnings) {
        std::cerr << "Warning: " << warning << "\n";
    }
    mapRenderer_ = std::make_unique<render::MapRenderer>(scene);
    for (const auto& object : mapRenderer_->objects()) {
        if (world_.isHidden(object.object)) mapRenderer_->hideObject(object.object);  // until revealed
    }
    for (const auto& pickup : world_.pickups()) {
        for (const auto& object : mapRenderer_->objects()) {
            if (object.object == pickup.object) crateTransforms_[pickup.object] = object.transform;
        }
    }
    for (const auto& mount : world_.mounts()) {
        for (const auto& object : mapRenderer_->objects()) {
            if (object.object == mount.object) mountTransforms_[mount.object] = object.transform;
        }
    }
    hogRenderer_ = std::make_unique<render::CharacterRenderer>();
    effects_ = std::make_unique<render::EffectsRenderer>();
    try {
        effects_->setPictures(loadEffectPictures(vfs));
    } catch (const std::exception& error) {
        std::cerr << "Warning: no effect pictures (" << error.what() << "): explosions drawn as balls\n";
    }
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
    autoSight_ = options.autoSight;
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
    hud_.setRadarMap(ui::radarMapPicture(world_.terrain()));
    showTeamStrength();
    audio_.matchStarted();
    if (options.openWeaponMenu) openWeaponMenu();
}

Game::~Game() {
    lockMouseForSight(false);  // back to the menus with a free pointer
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
        // Through the rifle sight the mouse looks around: right turns
        // right, up raises the aim.
        if (!paused_ && world_.sightView()) {
            sightLookYaw_ -= event.motion.xrel * kSightLookDegreesPerPixel;
            sightLookPitch_ -= event.motion.yrel * kSightLookDegreesPerPixel;
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
        if (paused_) return handlePauseInput(menuInputForKey(event.key.key));
        if (weaponMenu_.isOpen()) return handleWeaponMenuKey(event);
        return handleCommand(commandForKey(event));

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
    case Command::WeaponMenu: openWeaponMenu(); break;
    case Command::None:
    case Command::Pause: break;
    }
    return GameRequest::None;
}

Game::Command Game::commandForKey(const SDL_Event& event) const {
    if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
        return Command::Pause;  // always, whatever the bindings (ui::kAlwaysPauseKey)
    }
    const auto control = keyboard_.controlFor(event.key.scancode);
    if (!control) return Command::None;
    switch (*control) {
    case ui::Control::Pause: return Command::Pause;
    case ui::Control::NextHog: return (event.key.mod & SDL_KMOD_SHIFT) ? Command::PreviousHog : Command::NextHog;
    case ui::Control::EndTurn: return Command::EndTurn;
    case ui::Control::Jump: return Command::Jump;
    case ui::Control::PreviousWeapon: return Command::PreviousWeapon;
    case ui::Control::NextWeapon: return Command::NextWeapon;
    case ui::Control::WeaponMenu: return Command::WeaponMenu;
    default: return Command::None;  // held controls are read in readInput()
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

void Game::openWeaponMenu() {
    // Only for the hog in play, standing ready (not charging a throw).
    const auto active = world_.activeHog();
    if (!active || world_.phase() != game::MatchPhase::Playing) return;
    const auto& hog = world_.hogs()[*active];
    if (!hog.alive || hog.charging || hog.weapons.empty()) return;
    // Its own weapons count their uses left, the team's stock from crates
    // how many it has; the rest never run out.
    std::vector<ui::WeaponMenuEntry> entries;
    const std::size_t footerStart = hog.weapons.size() - std::min(hog.footerActions, hog.weapons.size());
    for (std::size_t i = 0; i < hog.weapons.size(); ++i) {
        ui::WeaponMenuEntry entry{i, hog.weapons[i], std::nullopt};
        if (i < hog.ownCount()) {
            if (const auto uses = hog.ownAmmo.find(hog.weapons[i]); uses != hog.ownAmmo.end()) entry.uses = uses->second;
        } else if (i < footerStart) {
            entry.uses = world_.stock(hog.team, hog.weapons[i]);
        }
        entries.push_back(entry);
    }
    weaponMenu_.open(entries, hog.weaponSlot % hog.weapons.size());
    weaponMenuTurn_ = world_.turns()->turnNumber();
    playMenuSound(sounds_, ui::MenuSound::Select);
}

GameRequest Game::handleWeaponMenuKey(const SDL_Event& event) {
    // The menu keys (arrows, W/S/A/D, Enter, Space, Esc), the fire key to
    // pick and the menu's own key to close it again.
    std::optional<ui::MenuInput> input = menuInputForKey(event.key.key);
    if (const auto control = keyboard_.controlFor(event.key.scancode)) {
        if (*control == ui::Control::Fire) input = ui::MenuInput::Confirm;
        if (*control == ui::Control::WeaponMenu) input = ui::MenuInput::Back;
    }
    if (!input) return GameRequest::None;
    const auto result = weaponMenu_.handle(*input);
    if (result.chosen) chosenWeapon_ = result.chosen;
    playMenuSound(sounds_, result.chosen ? ui::MenuSound::Select
                           : result.closed ? ui::MenuSound::Back
                           : result.moved  ? ui::MenuSound::Move
                                           : ui::MenuSound::None);
    return GameRequest::None;
}

game::PlayerInput Game::readInput() {
    game::PlayerInput input;
    // Keys held down are read directly; key presses (jump, weapon change)
    // are remembered by handleEvent until the next step.
    const bool* keys = SDL_GetKeyboardState(nullptr);
    const auto held = [&](ui::Control control) { return keyboard_.held(control, keys); };
    input.movement.forward = autoWalk_ || held(ui::Control::Forward);
    input.movement.backward = held(ui::Control::Backward);
    input.movement.turnLeft = held(ui::Control::TurnLeft);
    input.movement.turnRight = held(ui::Control::TurnRight);
    input.movement.jump = jumpPressed_;
    input.aimUp = held(ui::Control::AimUp);
    input.aimDown = held(ui::Control::AimDown);
    input.fire = held(ui::Control::Fire);
    // The rifle sight: held with its key or the right mouse button.
    input.sight = autoSight_ || held(ui::Control::Sight) ||
                  (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
    input.lookYawDegrees = sightLookYaw_;
    input.lookPitchDegrees = sightLookPitch_;
    sightLookYaw_ = sightLookPitch_ = 0.0f;
    if (autoFireAt_) {
        // Screenshot aid: hold fire for a while (half power for throws).
        constexpr double kAutoFireHoldSeconds = 0.75;
        const double t = world_.timeSeconds();
        input.fire = t >= *autoFireAt_ && t < *autoFireAt_ + kAutoFireHoldSeconds;
    }
    gamepads_.addHeldControls(input);
    if (weaponMenu_.isOpen()) {
        // Choosing a weapon: the hog stands and waits.
        input.movement = {};
        input.fire = input.aimUp = input.aimDown = input.sight = false;
    }
    input.weaponStep = weaponStep_;
    input.chooseWeapon = chosenWeapon_;
    jumpPressed_ = false;
    weaponStep_ = 0;
    chosenWeapon_.reset();
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
        case GameEventType::Splash:
            playSound(kSplashSound);
            particles_.splash(event.position);
            break;
        case GameEventType::Fired:
            if (!game::weaponSpec(event.weapon).launchSound.empty()) playSound(game::weaponSpec(event.weapon).launchSound);
            break;
        case GameEventType::Healed:
            // The health given back rises over the hog, in green.
            if (event.hog < world_.hogs().size() && event.amount > 0.0f) {
                hud_.addDamage(world_.hogs()[event.hog].position + glm::vec3(0.0f, kDamageNumberHeight, 0.0f),
                               event.amount, kHealColour);
            }
            break;
        case GameEventType::Bounce: playSound(kBounceSound); break;
        case GameEventType::BulletHit:
            playSound(kBulletHitSound);
            particles_.bulletHit(event.position);
            break;
        case GameEventType::Explosion: playSound(kExplosionSound); break;
        case GameEventType::Died: playSound(kDeathSound); break;
        case GameEventType::SuddenDeath: playSound(kSuddenDeathSound); break;
        case GameEventType::PickedUp:
            playSound(kPickupSound);
            pickedUp(event);
            if (const auto message = ui::pickupMessage(event, world_, interface_.strings(), interface_.gameText())) {
                hud_.postMessage(interface_, *message);
            }
            break;
        case GameEventType::ObjectiveDone: objectiveDone(event); break;
        case GameEventType::HogMounted:
        case GameEventType::HogDismounted: playSound("S_SELECT"); break;
        case GameEventType::HogHidden:
            if (event.hog < world_.hogs().size()) {
                // The disguise: a crate where the hog stood (the wiki: a
                // crate or a nearby prop).
                const auto& hog = world_.hogs()[event.hog];
                formats::MapObject disguise;
                disguise.modelName = "CRATE1";
                disguise.x = static_cast<std::int16_t>(hog.position.x);
                disguise.y = static_cast<std::int16_t>(hog.position.y + kDisguiseHalfHeight);
                disguise.z = static_cast<std::int16_t>(hog.position.z);
                mapRenderer_->addObject({"CRATE1", scene::objectTransform(disguise), kDisguiseObject + event.hog});
            }
            break;
        case GameEventType::HogUnhidden:
        case GameEventType::Respawned:  // back as it began (deathmatch): no disguise left
            mapRenderer_->hideObject(kDisguiseObject + event.hog);
            mapRenderer_->hideObject(kBootsObject + event.hog);
            break;
        case GameEventType::HogBlownUp:
            if (event.amount > 0.0f) {
                // Its boots stay where it stood (FUN_004680e0: object BOOTS),
                // standing on the ground (models are centred on their position).
                formats::MapObject boots;
                boots.modelName = "BOOTS";
                boots.x = static_cast<std::int16_t>(event.position.x);
                boots.y = static_cast<std::int16_t>(event.position.y + kBootsHalfHeight);
                boots.z = static_cast<std::int16_t>(event.position.z);
                glm::mat4 placed = glm::rotate(scene::objectTransform(boots), world_.hogs()[event.hog].yawRadians,
                                               glm::vec3(0.0f, 1.0f, 0.0f));
                mapRenderer_->showObject(kBootsObject + event.hog);
                mapRenderer_->addObject({"BOOTS", placed, kBootsObject + event.hog});
                // Two puffs of smoke beside them (effect 0x3C).
                particles_.puff(event.position + glm::vec3(30.0f, 20.0f, 0.0f));
                particles_.puff(event.position + glm::vec3(-30.0f, 20.0f, 0.0f));
            }
            break;
        case GameEventType::PickedPocket: playSound("I_PICKUP"); break;
        case GameEventType::TurnSlept: break;  // the turn simply passes
        case GameEventType::ObjectRevealed: mapRenderer_->showObject(event.object); break;
        case GameEventType::TrainingTip:
            // The Boot Camp's instruction comes down in the text box (the
            // instructor says it: MatchAudio).
            hud_.postMessage(interface_, interface_.strings().trainingText(event.text), ui::TextBox::kTrainingScroll);
            break;
        case GameEventType::ObjectDestroyed:
            // The object goes from the picture; buildings and vehicles blow up.
            mapRenderer_->hideObject(event.object);
            if (event.amount > 0.0f) playSound(kExplosionSound);
            break;
        case GameEventType::TurnStarted: showTeamStrength(); break;
        case GameEventType::Hurt:         // voices and music: MatchAudio
        case GameEventType::MatchOver: break;
        }
    }
}

void Game::pickedUp(const game::GameEvent& event) {
    // The crate goes; promotion points go to the campaign at the end.
    if (event.pickup >= world_.pickups().size()) return;
    const auto& pickup = world_.pickups()[event.pickup];
    mapRenderer_->hideObject(pickup.object);
    if (pickup.kind == game::PickupKind::PromotionPoints) {
        promotionPoints_ += static_cast<int>(event.amount);
    }
}

void Game::objectiveDone(const game::GameEvent& event) {
    // A reward crate appears where the objective put it (the sergeant's
    // comment is MatchAudio's); lost points come off the campaign's.
    promotionPoints_ -= static_cast<int>(event.amount);
    if (!event.dropped || event.pickup >= world_.pickups().size()) return;
    const auto& crate = world_.pickups()[event.pickup];
    // Placed like a map object, which is centred on its position.
    formats::MapObject placed;
    placed.modelName = crate.model;
    placed.x = static_cast<std::int16_t>(crate.centre.x);
    placed.y = static_cast<std::int16_t>(crate.centre.y);
    placed.z = static_cast<std::int16_t>(crate.centre.z);
    crateTransforms_[crate.object] = scene::objectTransform(placed);
    mapRenderer_->addObject({crate.model, crateTransforms_[crate.object], crate.object});
    placeFallingCrates();  // up in the sky from the first frame
}

void Game::spawnExplosionParticles() {
    // The explosions that happened since the last frame, each once.
    const auto& terrain = world_.terrain();
    for (const auto& explosion : world_.explosions()) {
        if (explosion.serial <= lastExplosion_) continue;
        lastExplosion_ = std::max(lastExplosion_, explosion.serial);
        const glm::vec3& at = explosion.position;
        const bool inWater = terrain.tileAt(at.x, at.z).isWater && at.y - terrain.heightAt(at.x, at.z) < 100.0f;
        particles_.explosion(at, explosion.radius, terrain.heightAt(at.x, at.z), inWater);
    }
}

void Game::drawShadows() {
    // A blob under each hog standing in the open (Shad_org), smaller the
    // higher it is above the ground (decision 160).
    for (std::size_t i = 0; i < world_.hogs().size(); ++i) {
        const auto& hog = world_.hogs()[i];
        if (hog.blownUp || hog.hidden() || hog.sheltered()) continue;
        if (hog.motion.state == game::MotionState::Swimming) continue;
        const float ground = world_.map().groundHeight(hog.position.x, hog.position.z, hog.position.y);
        const float height = std::max(0.0f, hog.position.y - ground);
        const float width = std::max(0.0f, kShadowWidth - height / 3.0f);
        if (width <= 0.0f) continue;
        effects_->drawShadow({hog.position.x, ground, hog.position.z}, width, kShadowOpacity);
    }
}

void Game::placeFallingCrates() {
    // A falling crate is drawn dropHeight above its place, under an open
    // parachute (WE_CHUT2); the parachute goes once it has landed.
    for (std::size_t index = 0; index < world_.pickups().size(); ++index) {
        const auto& pickup = world_.pickups()[index];
        const auto place = crateTransforms_.find(pickup.object);
        if (place == crateTransforms_.end()) continue;
        const std::size_t parachute = kParachuteObject + index;
        const bool falling = pickup.falling() && !pickup.hidden && !pickup.taken;
        if (!falling && !fallingCrates_.contains(index)) continue;
        const glm::mat4 lift = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, pickup.dropHeight, 0.0f));
        mapRenderer_->moveObject(pickup.object, lift * place->second);
        if (!falling) {
            mapRenderer_->hideObject(parachute);
            fallingCrates_.erase(index);
            continue;
        }
        // Models are twice the world's size with Y down (scene::objectTransform).
        constexpr float scale = formats::kModelToWorldScale;
        const glm::vec3 top = glm::vec3(place->second[3]) + glm::vec3(0.0f, pickup.dropHeight + kParachuteHeight, 0.0f);
        const glm::mat4 chute = glm::scale(glm::translate(glm::mat4(1.0f), top), glm::vec3(scale, -scale, scale));
        if (!fallingCrates_.contains(index)) {
            fallingCrates_.insert(index);
            mapRenderer_->addObject({"WE_CHUT2", chute, parachute});
        } else {
            mapRenderer_->moveObject(parachute, chute);
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
    placeMounts();
    placeFallingCrates();
    spawnExplosionParticles();
    particles_.update(seconds);
    banner_.update(seconds);
    // The weapon menu closes when the hog's chance to use it is over.
    const bool newTurn = world_.turns() && world_.turns()->turnNumber() != weaponMenuTurn_;
    if (weaponMenu_.isOpen() && (world_.phase() != game::MatchPhase::Playing || computerIsPlaying() || newTurn)) {
        weaponMenu_.close();
    }
    const auto turn = ui::describeTurn(world_, interface_.strings(), interface_.gameText());
    hud_.update(seconds, turn, weaponMenu_.isOpen(), turn && !world_.sightView() && !world_.binocularView());
    audio_.update(seconds);

    // The camera stays behind the active hog (decision 15), swinging round
    // smoothly as it turns.
    if (const auto active = world_.activeHog()) {
        camera_.turnTowards(game::hogFacing(world_.hogs()[*active]), seconds);
    }
    camera_.update(seconds, cameraTarget());
}

void Game::lockMouseForSight(bool sighting) {
    // Through the sight the mouse moves the view, so the pointer is held in
    // the window (relative mode) instead of wandering off it.
    SDL_Window* window = SDL_GetKeyboardFocus();
    if (window && SDL_GetWindowRelativeMouseMode(window) != sighting) {
        SDL_SetWindowRelativeMouseMode(window, sighting);
    }
}

void Game::simulate(double seconds) {
    const int steps = static_cast<int>(seconds / timestep_.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        step(timestep_.stepSeconds());
    }
    lockMouseForSight(!paused_ && world_.sightView().has_value());
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

    // The weapon in the hog's hand, drawn as the original's renderer does:
    // one rigid piece following the hand (scene::buildCarriedModelMesh).
    // A grenade, a mine or TNT leaves the hand once it is thrown or put
    // down.
    if (hog.drawnWeapon && hog.alive && world_.phase() != game::MatchPhase::Over && weaponTextures_) {
        const auto& spec = game::weaponSpec(*hog.drawnWeapon);
        const bool released = (spec.kind == game::WeaponKind::Thrown || spec.kind == game::WeaponKind::Placed) &&
                              world_.phase() != game::MatchPhase::Playing &&
                              hog.animation.animation() == spec.useAnimation && hog.animation.finished();
        const auto* weapon = released ? nullptr : weaponModel(spec.model);
        if (weapon) {
            const auto weaponMesh = scene::buildCarriedModelMesh(*weapon, weaponTextureSizes_, pose, scene::kHandBone);
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

    if (!world_.sightView()) {
        effects_->drawDots(world_.aimPreview(), 14.0f, kAimColour, true);  // the sight has its crosshair
    }

    for (const auto& tracer : world_.tracers()) {
        std::vector<glm::vec3> dots;
        const float length = glm::length(tracer.to - tracer.from);
        for (float along = 0.0f; along < length; along += 60.0f) {
            dots.push_back(tracer.from + (tracer.to - tracer.from) * (along / length));
        }
        effects_->drawDots(dots, 8.0f, kTracerColour);
    }
    if (effects_->hasPictures()) {
        drawShadows();
        effects_->drawParticles(particles_.particles());
    } else {
        for (const auto& explosion : world_.explosions()) {
            effects_->drawExplosion(explosion.position, explosion.radius,
                                    explosion.ageSeconds / game::kExplosionEffectSeconds);
        }
    }
}

void Game::placeMounts() {
    // A driven vehicle: its first placing, turned round its first centre
    // and moved by as much as the world drove it.
    for (const auto& mount : world_.mounts()) {
        if (mount.moved == glm::vec3(0.0f) && mount.turned == 0.0f) continue;
        const auto first = mountTransforms_.find(mount.object);
        if (first == mountTransforms_.end()) continue;
        const glm::mat4 turn = glm::translate(glm::mat4(1.0f), mount.origin) *
                               glm::rotate(glm::mat4(1.0f), mount.turned, glm::vec3(0.0f, 1.0f, 0.0f)) *
                               glm::translate(glm::mat4(1.0f), -mount.origin);
        mapRenderer_->moveObject(mount.object, glm::translate(glm::mat4(1.0f), mount.moved) * turn * first->second);
    }
}

render::Camera Game::sceneCamera() const {
    render::Camera camera;
    if (world_.mapView()) {
        // MAP VIEW: the whole map from high above.
        const auto& terrain = world_.terrain();
        camera.frame({terrain.minX(), 0.0f, terrain.minZ()}, {terrain.maxX(), 2000.0f, terrain.maxZ()});
        return camera;
    }
    if (const auto look = world_.binocularView()) {
        // Through binoculars: from the hog's eyes, much magnified.
        camera.position = look->eye;
        camera.yawDegrees = glm::degrees(std::atan2(look->direction.x, -look->direction.z));
        camera.pitchDegrees = glm::degrees(std::asin(std::clamp(look->direction.y, -1.0f, 1.0f)));
        camera.fieldOfViewDegrees = kBinocularFieldOfViewDegrees;
        return camera;
    }
    if (const auto sight = world_.sightView()) {
        // Through the rifle sight: from the muzzle along the line of fire,
        // magnified.
        camera.position = sight->eye;
        camera.yawDegrees = glm::degrees(std::atan2(sight->direction.x, -sight->direction.z));
        camera.pitchDegrees = glm::degrees(std::asin(std::clamp(sight->direction.y, -1.0f, 1.0f)));
        camera.fieldOfViewDegrees = kSightFieldOfViewDegrees;
        return camera;
    }
    const auto pose = camera_.pose(world_.terrain());
    camera.position = pose.position;
    camera.yawDegrees = pose.yawDegrees;
    camera.pitchDegrees = pose.pitchDegrees;
    return camera;
}

void Game::drawScene(int width, int height) {
    const render::Camera camera = sceneCamera();
    const bool sighting = world_.sightView().has_value() || world_.binocularView().has_value();

    render::DrawOptions options;
    options.showMarkers = false;  // the hogs themselves stand where the markers were
    mapRenderer_->draw(camera, width, height, options);

    for (std::size_t i = 0; i < world_.hogs().size(); ++i) {
        if (world_.hogs()[i].hidden()) continue;  // its disguise (a crate) is drawn instead
        if (world_.hogs()[i].blownUp) continue;   // nothing left but its boots
        if (sighting && world_.activeHog() == i) continue;  // we look out of its eyes
        if (world_.hogs()[i].sheltered()) continue;         // inside a building or vehicle
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
    const auto turn = ui::describeTurn(world_, interface_.strings(), interface_.gameText());
    const auto active = world_.activeHog();
    if (turn && turn->playing && active && !world_.sightView() && !world_.binocularView() &&
        !world_.hogs()[*active].sheltered()) {
        // No health bars: as on the PlayStation, only the hog in play has a
        // label, its name (or rank) and its health (decision 36).
        hud_.drawHogLabel(interface_, camera.projectionMatrix(aspect) * camera.viewMatrix(),
                          world_.hogs()[*active].position + glm::vec3(0.0f, kHogLabelHeight, 0.0f),
                          ui::hogLabel(*turn, world_.timeSeconds()), turn->health);
    }
    if (world_.sightView()) hud_.drawRifleSight(interface_.renderer());
    if (world_.binocularView()) hud_.drawRifleSight(interface_.renderer(), false);  // the round view, no crosshair
    hud_.draw(interface_, turn);
    hud_.drawRadar(interface_.renderer(), ui::radarMarks(world_, world_.activeHog()), camera.forward());
    hud_.drawTextBox(interface_, teamStrengths());
    hud_.drawWeaponMenu(interface_, weaponMenu_);  // also while it slides away
    if (const auto& message = banner_.current()) {
        // Turn messages fade out over their last quarter.
        const double progress = banner_.progress();
        const float alpha = progress > 0.75 ? static_cast<float>((1.0 - progress) / 0.25) : 1.0f;
        interface_.drawBanner(*message, kBannerY, alpha, bannerColour_);
    }
}

void Game::showTeamStrength() {
    // At the start of the match and of every round after, the big tube
    // shows how strong each team still is (FUN_0045ec50, not in the Boot
    // Camp).
    if (world_.training() || !world_.turns()) return;
    const int round = world_.turns()->round();
    if (round == teamStrengthRound_) return;
    teamStrengthRound_ = round;
    hud_.postTeamStrength();
}

std::vector<OriginalHud::TeamStrength> Game::teamStrengths() const {
    // The health left in each team, in team order, in its message colour.
    std::map<int, float> health;
    for (const auto& hog : world_.hogs()) {
        health[hog.team] += hog.alive ? std::max(hog.health, 0.0f) : 0.0f;
    }
    std::vector<OriginalHud::TeamStrength> strengths;
    for (const auto& [team, points] : health) {
        strengths.push_back({points, ui::teamMessageColour(world_.nationOf(team))});
    }
    return strengths;
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
