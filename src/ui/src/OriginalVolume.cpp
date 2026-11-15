// OriginalVolume.cpp - see OriginalVolume.h for an overview.
//
// The numbers come from the executable's front end (docs/formats/front-end.md):
// the drawing function FUN_0041bef0 (case 7, the panel), the foreground
// FUN_0041e780 (case 7, the knobs), the entry text FUN_00428cf0 (case 7)
// and the update FUN_00423e70 (case 7). Where the executable's text boxes
// do not fit its pictures, the text follows the pictures (see
// docs/open-decisions.md).

#include "openhogs/ui/OriginalVolume.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
constexpr int kPanelStartY = 400;  // 820-high units below its place
// Coming up: the executable's spring (gain 8, damping 30, top speed 30).
constexpr int kPanelGain = 8, kPanelDamping = 30, kPanelMaxSpeed = 30;
// Going away: it speeds up downwards until it is out of sight.
constexpr int kCloseAcceleration = 6, kCloseMaxSpeed = 80;
// The panel unfolds, one picture per tick, once it is this close.
constexpr int kUnfoldStartY = 100;
// The executable's blink counter: the selected value is dark for the
// first 31 of every 101 ticks.
constexpr int kBlinkPeriod = 101, kBlinkDarkTicks = 31;
constexpr int kLitKnob = 2;

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kPanelX = 100.0f, kPanelY = 240.0f;
// The knobs: x = 230 + level * 12.5, y = 320 and 351 (FUN_0041e780).
constexpr float kKnobX = 230.0f, kKnobStepX = 12.5f;
constexpr float kKnobY = 320.0f, kKnobSpacingY = 31.0f;
// The rows of text, centred on the rails of `vol00` (rows 109 and 140 of
// the picture) and one rail spacing above and below them.
constexpr float kRailY = kPanelY + 109.0f;
constexpr float kRowSpacing = 31.0f;
constexpr float kLabelRight = 252.0f;  // the rails start at x 260
constexpr float kValueLeft = 262.0f;

std::string numbered(const char* format, int number) {
    char name[32];
    std::snprintf(name, sizeof name, format, number);
    return name;
}

} // namespace

void OriginalVolumePanel::open(std::size_t selected) {
    panelY_ = kPanelStartY;
    panelVelocity_ = 0;
    closing_ = false;
    frame_ = 0;
    knobFrames_ = {};
    blink_ = 0;
    select(selected);
}

void OriginalVolumePanel::close() {
    closing_ = true;
    panelVelocity_ = 0;
}

void OriginalVolumePanel::select(std::size_t entry) {
    if (entry < kVolumeEntries) selected_ = entry;
}

void OriginalVolumePanel::tick() {
    if (closing_) {
        if (panelY_ < kPanelStartY) {
            panelVelocity_ = std::min(panelVelocity_ + kCloseAcceleration, kCloseMaxSpeed);
            panelY_ = std::min(panelY_ + panelVelocity_, kPanelStartY);
        }
    } else {
        panelY_ = frontEndSpring(panelY_, 0, panelVelocity_, kPanelGain, kPanelDamping, kPanelMaxSpeed);
        if (frame_ < kOpenFrame && panelY_ < kUnfoldStartY) ++frame_;
    }
    // The selected slider's knob lights; the other one fades.
    const std::array<std::size_t, 2> knobEntries{kVolumeMasterEntry, kVolumeEffectsEntry};
    for (std::size_t knob = 0; knob < knobFrames_.size(); ++knob) {
        int& frame = knobFrames_[knob];
        if (selected_ == knobEntries[knob] && !closing_) {
            if (frame < kLitKnob) ++frame;
        } else if (frame > 0) {
            --frame;
        }
    }
    blink_ = (blink_ + 1) % kBlinkPeriod;
}

void OriginalVolumePanel::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalVolumePanel::valueLit() const { return blink_ >= kBlinkDarkTicks; }

bool OriginalVolumePanel::closed() const { return closing_ && panelY_ >= kPanelStartY; }

bool OriginalVolumePanel::settled() const {
    return !closing_ && panelY_ == 0 && panelVelocity_ == 0 && frame_ == kOpenFrame;
}

float OriginalVolumePanel::offsetY() const {
    // Converted to screen pixels like the other screens' offsets.
    return static_cast<float>(panelY_ * 480 / 820);
}

std::vector<MenuPiece> OriginalVolumePanel::pieces(int masterLevel, int effectsLevel) const {
    std::vector<MenuPiece> pieces;
    const float dy = offsetY();
    if (showsText()) {
        // The executable moves the knobs by the plain offset and the panel
        // by twice it; they only differ while the panel moves.
        const std::array<int, 2> levels{masterLevel, effectsLevel};
        for (std::size_t knob = 0; knob < levels.size(); ++knob) {
            MenuPiece piece;
            piece.picture = numbered("vollit%02d", knobFrames_[knob] + 1);
            piece.x = kKnobX + static_cast<float>(levels[knob]) * kKnobStepX;
            piece.y = kKnobY + kKnobSpacingY * static_cast<float>(knob) + dy;
            pieces.push_back(piece);
        }
    }
    MenuPiece panel;
    panel.picture = frame_ == kOpenFrame ? "vol00" : numbered("vol%02d", frame_ + 1);
    panel.x = kPanelX;
    panel.y = kPanelY + 2.0f * dy;
    pieces.push_back(panel);
    return pieces;
}

VolumeTextLine OriginalVolumePanel::textLine(std::size_t entry) const {
    VolumeTextLine line;
    line.labelRight = kLabelRight;
    line.valueLeft = kValueLeft;
    line.middle = kRailY + kRowSpacing * (static_cast<float>(entry) - 1.0f) + 2.0f * offsetY();
    return line;
}

} // namespace openhogs::ui
