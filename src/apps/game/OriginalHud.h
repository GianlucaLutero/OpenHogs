// OriginalHud.h
//
// Draws the HUD of a match the way the original game does: the angle gauge
// in the top-right corner (with the weapon's icon), the power gauge that
// slides up at the bottom of the screen for weapons thrown or fired with a
// chosen power, and the clock in the bottom-right corner. The pictures are
// those of Language/Tims/dashtims.mad; where they go and how they move is
// worked out by ui::HudLayout (see docs/formats/hud.md).
//
// It also shows the damage numbers that rise above hurt hogs (digits of
// Language/Tims/expltims.mad).
//
// update() runs the instruments' animations (sliding in and out, fading)
// at the original's 25 ticks per second; draw() draws them. Needs a
// current OpenGL context.

#pragma once

#include "Interface.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/FloatingNumbers.h"
#include "openhogs/ui/Hud.h"
#include "openhogs/ui/HudLayout.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace openhogs::gameapp {

class OriginalHud {
public:
    // Missing pictures only cause a warning: the instruments are then
    // drawn without them.
    explicit OriginalHud(const core::VirtualFileSystem& vfs);

    // `turn` is the active hog's HUD information, empty between matches
    // or when the match is over.
    void update(double elapsedSeconds, const std::optional<ui::HudInfo>& turn);

    void draw(Interface& userInterface, const std::optional<ui::HudInfo>& turn) const;

    // A hog lost `amount` health points; `head` is the point above it.
    void addDamage(const glm::vec3& head, float amount, const glm::vec4& colour);

    // Draws the damage numbers, seen through `viewProjection` from
    // `cameraPosition`.
    void drawDamageNumbers(render::UiRenderer& ui, const glm::mat4& viewProjection,
                           const glm::vec3& cameraPosition) const;

private:
    const render::UiImage* picture(std::string_view name) const;
    void drawPiece(render::UiRenderer& ui, const ui::HudPiece& piece, float yOffset = 0.0f,
                   const glm::vec4& tint = glm::vec4(1.0f)) const;

    void drawAngleGauge(Interface& userInterface, const ui::HudInfo& turn) const;
    void drawPowerGauge(render::UiRenderer& ui, const ui::HudInfo& turn) const;
    void drawClock(render::UiRenderer& ui, const ui::HudInfo& turn) const;
    void drawClockDigit(render::UiRenderer& ui, float x, float offset, int digit, float roll) const;

    std::map<std::string, render::UiImage, std::less<>> pictures_;
    ui::FloatingNumbers damageNumbers_;

    ui::HudTicker ticker_;
    ui::HudSlide powerSlide_;
    ui::HudSlide clockSlide_;
    ui::HudFade aimFade_;
    ui::ClockLamp lamp_ = ui::ClockLamp::Green;
    int lastTurn_ = -1;
    float lastAimDegrees_ = 0.0f;  // the arrow stays where it was while fading out
};

} // namespace openhogs::gameapp
