// OriginalHud.cpp - see OriginalHud.h for an overview.

#include "OriginalHud.h"

#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TimImage.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// The instruments, and the damage digits with the explosion pictures.
const char* const kDashboardPictures = "Language/Tims/dashtims.mad";
const char* const kEffectPictures = "Language/Tims/expltims.mad";

// TIM entry names end in ".tim"; the layout uses the bare names.
std::string bareName(std::string_view entryName) {
    std::string name(entryName);
    if (const auto dot = name.rfind('.'); dot != std::string::npos) {
        name.erase(dot);
    }
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

} // namespace

OriginalHud::OriginalHud(const core::VirtualFileSystem& vfs) {
    for (const char* path : {kDashboardPictures, kEffectPictures}) {
        try {
            const auto archive = formats::PackageArchive::parse(vfs.readFile(path));
            for (const auto& entry : archive.entries()) {
                pictures_.emplace(bareName(entry.name), render::UiImage(formats::decodeTim(archive.entryData(entry))));
            }
        } catch (const std::exception& error) {
            std::cerr << "Warning: no HUD pictures from " << path << " (" << error.what() << ")\n";
        }
    }
}

void OriginalHud::addDamage(const glm::vec3& head, float amount, const glm::vec4& colour) {
    damageNumbers_.add(head, amount, colour);
}

void OriginalHud::drawDamageNumbers(render::UiRenderer& ui, const glm::mat4& viewProjection,
                                    const glm::vec3& cameraPosition) const {
    for (const auto& number : damageNumbers_.numbers()) {
        const glm::vec4 clip = viewProjection * glm::vec4(number.position, 1.0f);
        if (clip.w <= 0.0f) continue;  // behind the camera
        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        const float centreX = (ndc.x * 0.5f + 0.5f) * ui.canvasWidth();
        const float centreY = (0.5f - ndc.y * 0.5f) * render::kCanvasHeight;
        const float size = ui::floatingDigitSize(glm::length(number.position - cameraPosition));

        // The digits side by side, centred on the point.
        float x = centreX - size * static_cast<float>(number.text.size()) / 2.0f;
        for (const char digit : number.text) {
            if (const auto* image = picture(std::string("num") + digit)) {
                ui.drawImage(*image, x, centreY - size / 2.0f, size, size, number.colour);
            }
            x += size;
        }
    }
}

const render::UiImage* OriginalHud::picture(std::string_view name) const {
    const auto found = pictures_.find(name);
    return found == pictures_.end() ? nullptr : &found->second;
}

void OriginalHud::update(double elapsedSeconds, const std::optional<ui::HudInfo>& turn) {
    // Which instruments are wanted now.
    const bool playing = turn && turn->playing;
    const bool wantClock = turn.has_value();
    const bool wantPower = playing && turn->usesPower;
    const bool wantAim = playing && turn->canAim;

    const bool newTurn = turn && turn->turnNumber != lastTurn_;
    if (turn) {
        lamp_ = ui::nextClockLamp(lamp_, turn->exactSecondsLeft, newTurn);
        lastTurn_ = turn->turnNumber;
        if (wantAim) lastAimDegrees_ = turn->aimDegrees;
    }

    damageNumbers_.update(elapsedSeconds);
    for (int tick = ticker_.advance(elapsedSeconds); tick > 0; --tick) {
        powerSlide_.tick(wantPower);
        clockSlide_.tick(wantClock);
        aimFade_.tick(wantAim);
    }
}

void OriginalHud::drawPiece(render::UiRenderer& ui, const ui::HudPiece& piece, float yOffset,
                            const glm::vec4& tint) const {
    const auto* image = picture(piece.picture);
    if (!image) return;
    const float x = ui::anchoredX(piece.x, piece.anchor, ui.canvasWidth());
    const float y = piece.y + yOffset;
    if (piece.flippedVertically) {
        ui.drawImageRegion(*image, x, y, image->width(), image->height(), 0.0f, 1.0f, 1.0f, 0.0f, tint);
    } else {
        ui.drawImage(*image, x, y, image->width(), image->height(), tint);
    }
}

void OriginalHud::draw(Interface& userInterface, const std::optional<ui::HudInfo>& turn) const {
    if (!turn) return;
    auto& ui = userInterface.renderer();
    drawAngleGauge(userInterface, *turn);
    drawPowerGauge(ui, *turn);
    drawClock(ui, *turn);
}

void OriginalHud::drawAngleGauge(Interface& userInterface, const ui::HudInfo& turn) const {
    auto& ui = userInterface.renderer();

    // The fan and the arrow go under the dial's brass rim.
    if (aimFade_.level() > 0) {
        const glm::vec4 fanTint{1.0f, 1.0f, 1.0f, aimFade_.fanAlpha()};
        for (const auto& wedge : ui::angleGaugeFan()) {
            drawPiece(ui, wedge, 0.0f, fanTint);
        }
        if (const auto* arrow = picture("angpoint")) {
            const glm::vec2 pivot{ui::anchoredX(ui::kAimPointerPivot.x, ui::kAimPointerAnchor, ui.canvasWidth()),
                                  ui::kAimPointerPivot.y};
            ui.drawImageQuad(*arrow, ui::aimPointerCorners(lastAimDegrees_, pivot),
                             glm::vec4(1.0f, 1.0f, 1.0f, aimFade_.pointerAlpha()));
        }
    }
    for (const auto& piece : ui::angleGaugeFrame()) {
        drawPiece(ui, piece);
    }
    if (const auto* icon = userInterface.weaponIcon(turn.weapon)) {
        const auto& place = ui::kAngleGaugeIcon;
        ui.drawImage(*icon, ui::anchoredX(place.x, place.anchor, ui.canvasWidth()), place.y, icon->width(),
                     icon->height());
    }
}

void OriginalHud::drawPowerGauge(render::UiRenderer& ui, const ui::HudInfo& turn) const {
    if (!powerSlide_.visible()) return;
    const float offset = powerSlide_.offset();
    for (const auto& piece : ui::powerGaugeFrame()) {
        drawPiece(ui, piece, offset);
    }
    if (turn.power) {
        drawPiece(ui, ui::kPowerLamp, offset);
    }
    // The needle stands in the tube's scale window, in front of the scale.
    if (const auto* needle = picture("powg1")) {
        const float x = ui::anchoredX(ui::powerNeedleX(turn.power.value_or(0.0f)), ui::HudAnchor::Centre,
                                      ui.canvasWidth());
        ui.drawImage(*needle, x, ui::kPowerNeedleY + offset, needle->width(), needle->height());
    }
}

void OriginalHud::drawClockDigit(render::UiRenderer& ui, float x, float offset, int digit, float roll) const {
    // The digit rolls upwards and the next one down follows it from below;
    // both are cut to the clock's window. The digits are 25 pixels high in
    // a 28-pixel window: one pixel of the window shows above them.
    const int next = (digit + 9) % 10;
    const float top = ui::kClockWindowTop + offset;
    const float bottom = ui::kClockWindowBottom + offset;
    const float y = top + 1.0f;
    for (const auto& [value, digitTop] : {std::pair{digit, y - roll}, std::pair{next, y - roll + ui::kClockDigitHeight}}) {
        const auto* image = picture("timer" + std::to_string(value));
        if (!image) continue;
        const float visibleTop = std::max(digitTop, top);
        const float visibleBottom = std::min(digitTop + image->height(), bottom);
        if (visibleBottom <= visibleTop) continue;
        const float v0 = (visibleTop - digitTop) / image->height();
        const float v1 = (visibleBottom - digitTop) / image->height();
        ui.drawImageRegion(*image, x, visibleTop, image->width(), visibleBottom - visibleTop, 0.0f, v0, 1.0f, v1);
    }
}

void OriginalHud::drawClock(render::UiRenderer& ui, const ui::HudInfo& turn) const {
    if (!clockSlide_.visible()) return;
    const float offset = clockSlide_.offset();

    // The digits roll in the window, behind the clock's face; the lamp
    // glows on top.
    const auto digits = ui::clockDigits(turn.exactSecondsLeft);
    drawClockDigit(ui, ui::anchoredX(ui::kClockDigitX[0], ui::HudAnchor::Right, ui.canvasWidth()), offset,
                   digits.tens, digits.tensRoll);
    drawClockDigit(ui, ui::anchoredX(ui::kClockDigitX[1], ui::HudAnchor::Right, ui.canvasWidth()), offset,
                   digits.units, digits.unitsRoll);
    for (const auto& piece : ui::clockFrame()) {
        drawPiece(ui, piece, offset);
    }
    drawPiece(ui, ui::kClockLamp, offset, ui::lampColour(lamp_));
}

} // namespace openhogs::gameapp
