// CharacterViewer.h
//
// The character viewer application: one hog on a ground grid, with keys to
// change its class, team, level of detail and animation, and a camera that
// orbits around it.
//
// Controls:
//   Left / Right     previous / next animation
//   Up / Down        previous / next class
//   T                next team
//   L                next level of detail (high, medium, low)
//   E                next expression (automatic, then each one by hand)
//   B                blinking on / off
//   Space            pause / play
//   , / .            one frame back / forward (pauses)
//   - / +            slower / faster playback
//   Left mouse drag  orbit around the hog      Mouse wheel: zoom
//   F12              save a screenshot (openhogs-<class>-<animation>.png)
//   Esc              quit

#pragma once

#include "openhogs/apps/GlWindow.h"
#include "openhogs/formats/Model.h"
#include "openhogs/render/Camera.h"
#include "openhogs/render/CharacterRenderer.h"
#include "openhogs/scene/CharacterLibrary.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

union SDL_Event;

namespace openhogs::characterviewer {

// What is shown; also the starting state given on the command line.
struct ViewerState {
    std::size_t classIndex = 0;  // into scene::characterClasses()
    std::size_t teamIndex = 0;   // into scene::characterTeams()
    scene::CharacterDetail detail = scene::CharacterDetail::High;
    std::size_t animation = 0;
    float frame = 0.0f;           // fractional frame position
    float framesPerSecond = 25.0f;
    bool playing = true;
    // Empty: the expression follows the animation (scene::ExpressionRules).
    std::optional<scene::Expression> expression;
    bool blinking = true;
    // Hogs face +X. Yaw -90 looks at a hog from the front, 0 from its
    // left side; the default is a three-quarter view from the front.
    float orbitYawDegrees = -60.0f;
    float orbitPitchDegrees = -10.0f;  // negative: looking down at the hog
    float orbitDistance = 650.0f;      // world units
};

class CharacterViewer {
public:
    CharacterViewer(const scene::CharacterLibrary& library, const ViewerState& start, int width, int height,
                    bool hiddenWindow);
    ~CharacterViewer();

    CharacterViewer(const CharacterViewer&) = delete;
    CharacterViewer& operator=(const CharacterViewer&) = delete;

    // Runs until the user closes the window.
    void run();

    // Renders the current state off-screen and writes it as a PNG file.
    void saveScreenshot(const std::filesystem::path& path);

private:
    void loadModel();
    void loadTeam();
    void handleEvent(const SDL_Event& event, bool& running);
    void changeAnimation(int step);
    void stepFrame(int step);
    void updateCamera();
    void updateTitle();
    void nextExpression();
    scene::Expression currentExpression() const;
    void draw(int width, int height);

    const scene::CharacterLibrary& library_;
    ViewerState state_;

    apps::GlWindow window_;
    int width_;
    int height_;

    render::Camera camera_;
    std::unique_ptr<render::CharacterRenderer> renderer_;
    formats::Model model_;
    std::vector<glm::uvec2> textureSizes_;
    scene::FaceTextureSlots faceSlots_;
    double clockSeconds_ = 0.0;  // drives blinking, runs even when paused
};

} // namespace openhogs::characterviewer
