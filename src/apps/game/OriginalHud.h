// OriginalHud.h
//
// Draws the HUD of a match the way the original game does: the angle gauge
// in the top-right corner (with the weapon's icon), the power gauge that
// slides up at the bottom of the screen for weapons thrown or fired with a
// chosen power, and the clock in the bottom-right corner. The pictures are
// those of Language/Tims/dashtims.mad; where they go and how they move is
// worked out by ui::HudLayout (see docs/formats/hud.md).
//
// It also draws the radar in the bottom-left corner (ui/Radar.h, marks of
// Language/Tims/mapicons.mtd), the text box that comes down from the top
// with messages (ui/TextBox.h, Language/Tims/tboxtims.mad), the weapon menu
// (Language/Tims/MENUTIMS.MAD), the label
// over the hog in play (its name or rank and its
// health after a heart, as on the PlayStation), the damage numbers that rise above hurt hogs (digits of
// Language/Tims/expltims.mad) and the rifle sight (crosshair of
// Language/Tims/facetims.mad).
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
#include "openhogs/ui/Radar.h"
#include "openhogs/ui/TextBox.h"
#include "openhogs/ui/WeaponMenu.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::gameapp {

class OriginalHud {
public:
    // Missing pictures only cause a warning: the instruments are then
    // drawn without them.
    explicit OriginalHud(const core::VirtualFileSystem& vfs);

    // `turn` is the active hog's HUD information, empty between matches
    // or when the match is over.
    // `radarShown`: the radar is wanted (in a match, not through a sight).
    void update(double elapsedSeconds, const std::optional<ui::HudInfo>& turn, bool weaponMenuOpen = false,
                bool radarShown = false);

    void draw(Interface& userInterface, const std::optional<ui::HudInfo>& turn) const;

    // A hog lost `amount` health points; `head` is the point above it.
    void addDamage(const glm::vec3& head, float amount, const glm::vec4& colour);

    // Draws the damage numbers, seen through `viewProjection` from
    // `cameraPosition`.
    void drawDamageNumbers(render::UiRenderer& ui, const glm::mat4& viewProjection,
                           const glm::vec3& cameraPosition) const;

    // The label over the hog in play, as on the PlayStation: `label` (its
    // name or rank, ui::hogLabel) in the game's yellow letters, and under
    // it a heart with its health. `head` is the point above the hog.
    void drawHogLabel(Interface& userInterface, const glm::mat4& viewProjection, const glm::vec3& head,
                      const std::string& label, int health) const;

    // The weapon menu (ui::WeaponMenu) on its brass panel, sliding in from
    // the right while it is open: the tabs, the icons with their uses
    // left, the selection frame and the chosen weapon's name.
    void drawWeaponMenu(Interface& userInterface, const ui::WeaponMenu& menu) const;

    // The picture of the map the radar shows (ui::radarMapPicture), made
    // when a match starts.
    void setRadarMap(const formats::RgbaImage& picture);

    // The radar, sliding up from below the screen: the map seen along
    // `cameraForward`, with `marks` on it.
    void drawRadar(render::UiRenderer& ui, const std::vector<ui::RadarMark>& marks,
                   const glm::vec3& cameraForward) const;

    // A message for the text box (it waits its turn if one is showing).
    // `scrollPerTick`: how fast it scrolls if it is too long to fit.
    void postMessage(const Interface& userInterface, const std::string& text,
                     float scrollPerTick = ui::TextBox::kPickupScroll);
    // The teams' strength, in the big tube.
    void postTeamStrength() { textBox_.postTeamStrength(); }
    void clearMessages() { textBox_ = {}; }

    // One team's share of the strength bar.
    struct TeamStrength {
        float health = 0.0f;
        glm::vec4 colour{1.0f};
    };
    void drawTextBox(Interface& userInterface, const std::vector<TeamStrength>& teams) const;

    // The rifle sight over the view: black around a round hole and the
    // crosshair (ui/RifleSight.h).
    void drawRifleSight(render::UiRenderer& ui, bool crosshair = true) const;

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
    ui::HudSlide weaponMenuSlide_;
    ui::HudSlide radarSlide_;
    std::optional<render::UiImage> radarMap_;
    ui::TextBox textBox_;
    // The weapon menu's pictures (MENUTIMS.MAD, Language/Tims), if found.
    std::optional<render::UiImage> menuPanel_;
    std::optional<render::UiImage> menuFrame_;
    std::optional<render::UiImage> menuDigits_;
    std::optional<render::UiImage> menuHalftone_;
    ui::HudFade aimFade_;
    ui::ClockLamp lamp_ = ui::ClockLamp::Green;
    int lastTurn_ = -1;
    float lastAimDegrees_ = 0.0f;  // the arrow stays where it was while fading out
    bool lastAimLimited_ = false;  // so does the limit mark
};

} // namespace openhogs::gameapp
