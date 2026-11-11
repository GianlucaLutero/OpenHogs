// HudLayout.cpp - see HudLayout.h for an overview, docs/formats/hud.md for
// where the numbers come from.

#include "openhogs/ui/HudLayout.h"

#include <algorithm>
#include <cmath>

namespace openhogs::ui {

namespace {

// warhogs_.exe, table at VA 0x4CF468: the dial (ang1 .. ang5), tied to the
// right of the screen.
constexpr std::array<HudPiece, 5> kAngleGaugeFrame{{
    {"ang1", 480.0f, 11.0f, HudAnchor::Right},
    {"ang2", 544.0f, 33.0f, HudAnchor::Right},
    {"ang3", 480.0f, 75.0f, HudAnchor::Right},
    {"ang4", 544.0f, 75.0f, HudAnchor::Right},
    {"ang5", 608.0f, 41.0f, HudAnchor::Right},
}};

// VA 0x4CF580: the fan, two wedges above the horizon and the same two
// upside down below it.
constexpr std::array<HudPiece, 4> kAngleGaugeFan{{
    {"wedge1", 499.0f, 16.0f, HudAnchor::Right, false},
    {"wedge2", 482.0f, 34.0f, HudAnchor::Right, false},
    {"wedge1", 499.0f, 71.0f, HudAnchor::Right, true},
    {"wedge2", 482.0f, 71.0f, HudAnchor::Right, true},
}};

// VA 0x4CF300: the tube of the power gauge, centred.
constexpr std::array<HudPiece, 7> kPowerGaugeFrame{{
    {"newpow3", 160.0f, 419.0f, HudAnchor::Centre},
    {"newpow4", 224.0f, 419.0f, HudAnchor::Centre},
    {"newpow5", 288.0f, 419.0f, HudAnchor::Centre},
    {"newpow6", 352.0f, 419.0f, HudAnchor::Centre},
    {"newpow7", 416.0f, 419.0f, HudAnchor::Centre},
    {"newpow1", 160.0f, 399.0f, HudAnchor::Centre},
    {"newpow2", 224.0f, 399.0f, HudAnchor::Centre},
}};

// VA 0x4CF648: the clock, in the bottom-right corner.
constexpr std::array<HudPiece, 4> kClockFrame{{
    {"clock01", 503.0f, 377.0f, HudAnchor::Right},
    {"clock02", 567.0f, 377.0f, HudAnchor::Right},
    {"clock03", 503.0f, 405.0f, HudAnchor::Right},
    {"clock04", 567.0f, 405.0f, HudAnchor::Right},
}};

// VA 0x4CF2D0: how far a digit has rolled (pixels) during the last 12
// hundredths of a second before it changes: it speeds up, overshoots the
// next digit and settles.
constexpr std::array<float, 12> kDigitRoll{0, 2, 4, 6, 8, 10, 22, 24, 23, 23, 24, 24};

// The needle's scale: from 95 pixels right of the tube's left end, 151
// pixels long (power 0 .. 4095 in the executable).
constexpr float kPowerScaleStart = 160.0f + 95.0f;
constexpr float kPowerScaleLength = 151.0f;

// Seconds left under which the clock lamp turns amber, then red.
constexpr double kAmberSeconds = 10.0;
constexpr double kRedSeconds = 5.0;

} // namespace

float anchoredX(float x, HudAnchor anchor, float canvasWidth) {
    switch (anchor) {
    case HudAnchor::Centre:
        return x + (canvasWidth - kOriginalScreenWidth) / 2.0f;
    case HudAnchor::Right:
        return x + (canvasWidth - kOriginalScreenWidth);
    case HudAnchor::Left:
        break;
    }
    return x;
}

std::span<const HudPiece> angleGaugeFrame() { return kAngleGaugeFrame; }
std::span<const HudPiece> angleGaugeFan() { return kAngleGaugeFan; }
std::span<const HudPiece> powerGaugeFrame() { return kPowerGaugeFrame; }
std::span<const HudPiece> clockFrame() { return kClockFrame; }

std::array<glm::vec2, 4> aimPointerCorners(float aimDegrees, glm::vec2 pivot) {
    // The executable turns the picture with the head 50 pixels out, the
    // tail 14 pixels behind the pivot, 7 pixels either side of the line.
    constexpr float kHead = 50.0f;
    constexpr float kTail = 14.0f;
    constexpr float kHalfThickness = 7.0f;
    const float angle = glm::radians(aimDegrees);
    // Screen y points down: "up" is negative y.
    const glm::vec2 forward{-std::cos(angle), -std::sin(angle)};
    const glm::vec2 side{std::sin(angle), -std::cos(angle)};  // the picture's top edge side
    const glm::vec2 head = pivot + forward * kHead;
    const glm::vec2 tail = pivot - forward * kTail;
    return {head + side * kHalfThickness, tail + side * kHalfThickness, tail - side * kHalfThickness,
            head - side * kHalfThickness};
}

float powerNeedleX(float power) {
    return kPowerScaleStart + std::clamp(power, 0.0f, 1.0f) * kPowerScaleLength;
}

ClockDigits clockDigits(double secondsLeft) {
    // The executable counts in hundredths of a second and shows
    // (hundredths + 100) / 100: the seconds rounded up, "01" at the end.
    const long hundredths = std::max(0L, static_cast<long>(std::floor(secondsLeft * 100.0)));
    const long shown = std::min(99L, hundredths / 100 + 1);
    ClockDigits digits;
    digits.tens = static_cast<int>(shown / 10);
    digits.units = static_cast<int>(shown % 10);

    // In the last 12 hundredths before the value drops, the units digit
    // rolls; the tens digit rolls with it when the units go from 0 to 9.
    const long fraction = hundredths % 100;
    if (hundredths >= 100 && fraction < static_cast<long>(kDigitRoll.size())) {
        const float roll = kDigitRoll[kDigitRoll.size() - 1 - static_cast<std::size_t>(fraction)];
        digits.unitsRoll = roll;
        if (digits.units == 0) {
            digits.tensRoll = roll;
        }
    }
    return digits;
}

ClockLamp nextClockLamp(ClockLamp current, double secondsLeft, bool turnJustStarted) {
    if (secondsLeft < kRedSeconds) return ClockLamp::Red;
    if (secondsLeft < kAmberSeconds) return ClockLamp::Amber;
    return turnJustStarted ? ClockLamp::Green : current;
}

glm::vec4 lampColour(ClockLamp lamp) {
    // The executable's tints (VA 0x4CF250) are dark, (0, 90, 0), (158,
    // 107, 14) and (90, 0, 0), and it adds a glow it computes each frame;
    // OpenHogs adds a steady 97 to each channel instead.
    constexpr float kGlow = 97.0f;
    glm::vec3 base{0.0f, 90.0f, 0.0f};
    if (lamp == ClockLamp::Amber) base = {158.0f, 107.0f, 14.0f};
    if (lamp == ClockLamp::Red) base = {90.0f, 0.0f, 0.0f};
    return {glm::min((base + kGlow) / 255.0f, glm::vec3(1.0f)), 1.0f};
}

void HudSlide::tick(bool shown) {
    ticks_ = std::clamp(ticks_ + (shown ? 1 : -1), 0, kTicks);
}

float HudSlide::offset() const {
    // The executable eases the instrument in from 200 pixels below; the
    // exact curve is not known, so it slows down as it arrives.
    const float remaining = 1.0f - static_cast<float>(ticks_) / static_cast<float>(kTicks);
    return kHiddenOffset * remaining * remaining;
}

void HudFade::tick(bool shown) {
    level_ = std::clamp(level_ + (shown ? kStep : -kStep), 0, kMaximum);
}

float HudFade::fanAlpha() const { return static_cast<float>(level_) / 255.0f; }

float HudFade::pointerAlpha() const { return std::min(1.0f, static_cast<float>(level_ * 5) / 255.0f); }

int HudTicker::advance(double elapsedSeconds) {
    carried_ += std::max(0.0, elapsedSeconds);
    // The small allowance keeps rounding errors from losing a tick
    // (sixty frames of 1/60 s are exactly 25 ticks).
    const int ticks = static_cast<int>(carried_ / kHudTickSeconds + 1e-6);
    carried_ = std::max(0.0, carried_ - ticks * kHudTickSeconds);
    // After a long pause (a loading screen) the HUD simply catches up at
    // once instead of replaying every missed tick.
    return std::min(ticks, HudSlide::kTicks);
}

glm::vec4 teamMessageColour(int nation) {
    // VA 0x4CF1E0, in the executable's team order: British, American,
    // French, German, Russian, Japanese, Team Lard.
    constexpr std::array<glm::vec3, 7> kColours{{
        {70, 240, 70},
        {70, 240, 240},
        {70, 70, 240},
        {255, 255, 255},
        {240, 70, 70},
        {240, 240, 70},
        {255, 32, 255},
    }};
    // scene::characterTeams() lists Japanese before Russian.
    constexpr std::array<std::size_t, 7> kFromCharacterTeam{0, 1, 2, 3, 5, 4, 6};
    const int count = static_cast<int>(kColours.size());
    const auto team = static_cast<std::size_t>(((nation % count) + count) % count);
    return {kColours[kFromCharacterTeam[team]] / 255.0f, 1.0f};
}

} // namespace openhogs::ui
