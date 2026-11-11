// CharacterViewer.cpp - see CharacterViewer.h for an overview.

#include "CharacterViewer.h"

#include "openhogs/scene/CharacterPose.h"
#include "openhogs/scene/ModelMesh.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace openhogs::characterviewer {

namespace {

constexpr float kOrbitDegreesPerPixel = 0.4f;
constexpr float kZoomStep = 0.9f;  // distance factor per wheel notch
constexpr float kMinDistance = 120.0f;
constexpr float kMaxDistance = 3000.0f;
constexpr float kSpeedStep = 1.25f;  // playback speed factor per key press

// The camera looks at the middle of a standing hog (world units).
constexpr float kLookAtHeight = 90.0f;

const char* detailName(scene::CharacterDetail detail) {
    switch (detail) {
    case scene::CharacterDetail::High: return "high detail";
    case scene::CharacterDetail::Medium: return "medium detail";
    case scene::CharacterDetail::Low: return "low detail";
    }
    return "";
}

scene::CharacterDetail nextDetail(scene::CharacterDetail detail) {
    switch (detail) {
    case scene::CharacterDetail::High: return scene::CharacterDetail::Medium;
    case scene::CharacterDetail::Medium: return scene::CharacterDetail::Low;
    case scene::CharacterDetail::Low: break;
    }
    return scene::CharacterDetail::High;
}

std::size_t wrapIndex(std::size_t index, int step, std::size_t count) {
    const auto signedCount = static_cast<long long>(count);
    const long long next = (static_cast<long long>(index) + step) % signedCount;
    return static_cast<std::size_t>(next < 0 ? next + signedCount : next);
}

} // namespace

CharacterViewer::CharacterViewer(const scene::CharacterLibrary& library, const ViewerState& start, int width,
                                 int height, bool hiddenWindow)
    : library_(library), state_(start), window_("OpenHogs character viewer", width, height, hiddenWindow),
      width_(width), height_(height) {
    renderer_ = std::make_unique<render::CharacterRenderer>();
    loadModel();
    loadTeam();
    updateCamera();
    updateTitle();
}

CharacterViewer::~CharacterViewer() {
    renderer_.reset();  // GPU resources must go before the window's context
}

void CharacterViewer::loadModel() {
    const auto& characterClass = scene::characterClasses()[state_.classIndex];
    model_ = library_.loadModel(characterClass, state_.detail);
}

void CharacterViewer::loadTeam() {
    const auto textures = library_.loadTeamTextures(scene::characterTeams()[state_.teamIndex]);
    textureSizes_ = scene::textureSizes(textures.images);
    faceSlots_ = textures.faces;
    renderer_->setTextures(textures.images);
}

void CharacterViewer::changeAnimation(int step) {
    state_.animation = wrapIndex(state_.animation, step, library_.animations().size());
    state_.frame = 0.0f;
}

void CharacterViewer::stepFrame(int step) {
    state_.playing = false;
    const auto frameCount = library_.animations()[state_.animation].frames.size();
    state_.frame = static_cast<float>(wrapIndex(static_cast<std::size_t>(state_.frame), step, frameCount));
}

scene::Expression CharacterViewer::currentExpression() const {
    return state_.expression.value_or(library_.expressionRules().forAnimation(state_.animation));
}

void CharacterViewer::nextExpression() {
    // Automatic, then every expression in order, then automatic again.
    constexpr int kLast = static_cast<int>(scene::Expression::Gormless);
    if (!state_.expression) {
        state_.expression = scene::Expression::Neutral;
    } else if (static_cast<int>(*state_.expression) == kLast) {
        state_.expression.reset();
    } else {
        state_.expression = static_cast<scene::Expression>(static_cast<int>(*state_.expression) + 1);
    }
}

void CharacterViewer::updateCamera() {
    camera_.yawDegrees = state_.orbitYawDegrees;
    camera_.pitchDegrees = state_.orbitPitchDegrees;
    camera_.position = glm::vec3(0.0f, kLookAtHeight, 0.0f) - camera_.forward() * state_.orbitDistance;
}

void CharacterViewer::updateTitle() {
    const auto& characterClass = scene::characterClasses()[state_.classIndex];
    const auto& animation = library_.animations()[state_.animation];
    const auto frame = static_cast<std::size_t>(state_.frame) % animation.frames.size();

    const std::string name(scene::animationName(state_.animation));
    const std::string expression(scene::expressionName(currentExpression()));
    char title[384];
    std::snprintf(title, sizeof(title),
                  "OpenHogs character viewer - %s (%s, %s) - animation %zu/%zu%s%s, frame %zu/%zu%s - face: %s%s",
                  std::string(characterClass.name).c_str(),
                  std::string(scene::characterTeams()[state_.teamIndex]).c_str(), detailName(state_.detail),
                  state_.animation, library_.animations().size() - 1, name.empty() ? "" : ": ", name.c_str(),
                  frame, animation.frames.size() - 1, state_.playing ? "" : " (paused)", expression.c_str(),
                  state_.expression ? "" : " (auto)");
    window_.setTitle(title);
}

void CharacterViewer::handleEvent(const SDL_Event& event, bool& running) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        running = false;
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (event.motion.state & SDL_BUTTON_LMASK) {
            state_.orbitYawDegrees += event.motion.xrel * kOrbitDegreesPerPixel;
            state_.orbitPitchDegrees =
                std::clamp(state_.orbitPitchDegrees - event.motion.yrel * kOrbitDegreesPerPixel, -85.0f, 85.0f);
        }
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        state_.orbitDistance = std::clamp(state_.orbitDistance * std::pow(kZoomStep, event.wheel.y), kMinDistance,
                                          kMaxDistance);
        break;

    case SDL_EVENT_KEY_DOWN:
        switch (event.key.key) {
        case SDLK_ESCAPE: running = false; break;
        case SDLK_LEFT: changeAnimation(-1); break;
        case SDLK_RIGHT: changeAnimation(1); break;
        case SDLK_UP:
            state_.classIndex = wrapIndex(state_.classIndex, -1, scene::characterClasses().size());
            loadModel();
            break;
        case SDLK_DOWN:
            state_.classIndex = wrapIndex(state_.classIndex, 1, scene::characterClasses().size());
            loadModel();
            break;
        case SDLK_T:
            state_.teamIndex = wrapIndex(state_.teamIndex, 1, scene::characterTeams().size());
            loadTeam();
            break;
        case SDLK_L:
            state_.detail = nextDetail(state_.detail);
            loadModel();
            break;
        case SDLK_E: nextExpression(); break;
        case SDLK_B: state_.blinking = !state_.blinking; break;
        case SDLK_SPACE: state_.playing = !state_.playing; break;
        case SDLK_COMMA: stepFrame(-1); break;
        case SDLK_PERIOD: stepFrame(1); break;
        case SDLK_MINUS: state_.framesPerSecond /= kSpeedStep; break;
        case SDLK_EQUALS:
        case SDLK_PLUS: state_.framesPerSecond *= kSpeedStep; break;
        case SDLK_F12: {
            const std::string file = "openhogs-" + std::string(scene::characterClasses()[state_.classIndex].id) +
                                     "-" + std::to_string(state_.animation) + ".png";
            saveScreenshot(file);
            std::cout << "Saved " << file << "\n";
            break;
        }
        default: break;
        }
        break;

    default:
        break;
    }
}

void CharacterViewer::draw(int width, int height) {
    const auto& animation = library_.animations()[state_.animation];
    const auto pose = scene::samplePose(library_.skeleton(), animation, state_.frame);
    auto mesh = scene::buildPosedModelMesh(model_, textureSizes_, pose);
    const bool eyesClosed = state_.blinking && scene::isBlinking(clockSeconds_, 0);
    const bool masked = scene::characterClasses()[state_.classIndex].masked;
    scene::applyExpression(mesh, faceSlots_, currentExpression(), eyesClosed, masked);
    renderer_->draw(camera_, width, height, mesh, scene::kStandingFeetDepth);
}

void CharacterViewer::run() {
    bool running = true;
    Uint64 previous = SDL_GetTicksNS();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event, running);
        }

        const Uint64 now = SDL_GetTicksNS();
        const float seconds = static_cast<float>(now - previous) / 1e9f;
        previous = now;
        clockSeconds_ += seconds;
        if (state_.playing) {
            state_.frame += seconds * state_.framesPerSecond;
            const auto frameCount = static_cast<float>(library_.animations()[state_.animation].frames.size());
            state_.frame = std::fmod(state_.frame, frameCount);  // keep the number small
        }

        updateCamera();
        updateTitle();
        window_.sizeInPixels(width_, height_);
        draw(width_, height_);
        window_.swap();
    }
}

void CharacterViewer::saveScreenshot(const std::filesystem::path& path) {
    // Off-screen, so it works even with a hidden or covered window. The
    // clock stays at 0, where no hog is blinking, so the face is the one
    // asked for.
    updateCamera();
    render::GlFramebuffer target(width_, height_);
    target.bind();
    draw(width_, height_);
    const auto image = render::captureFramebuffer(width_, height_);
    render::GlFramebuffer::unbind();
    apps::savePng(image, path);
}

} // namespace openhogs::characterviewer
