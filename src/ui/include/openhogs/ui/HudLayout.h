// HudLayout.h
//
// The layout and the moving parts of the original game's HUD, as the PC
// executable (warhogs_.exe) builds them; see docs/formats/hud.md.
//
// The original HUD is made of brass instruments drawn with the pictures of
// Language/Tims/dashtims.mad:
//
//   - the angle gauge (top right): a half dial with the icon of the
//     selected weapon; an arrow shows the aim above or below the horizon
//     and a pale fan lights up while the hog can aim;
//   - the power gauge (bottom centre): a tube with a scale whose needle
//     follows the power of a throw; it slides up from the bottom of the
//     screen when it is needed;
//   - the clock (bottom right): two rolling digits for the seconds left in
//     the turn and a lamp that turns amber, then red, when time runs out.
//
// Positions are those of the executable's tables, in the pixels of its
// 640 x 480 screen. Each piece is tied to the left, the centre or the
// right of the screen, so on a wider screen the instruments stay at the
// edges (the executable does the same with its screen size). The canvas of
// render::UiRenderer is 480 units high, so y needs no conversion.
//
// Nothing here draws: the game application draws the pieces where this
// says, so the arithmetic is unit tested.

#pragma once

#include <glm/glm.hpp>

#include <array>
#include <span>
#include <string_view>

namespace openhogs::ui {

// The original's screen size, the unit of every position below.
inline constexpr float kOriginalScreenWidth = 640.0f;
inline constexpr float kOriginalScreenHeight = 480.0f;

// Which side of the screen a piece keeps its distance from.
enum class HudAnchor { Left, Centre, Right };

// One picture of the HUD at its place.
struct HudPiece {
    std::string_view picture;  // entry of dashtims.mad
    float x = 0.0f;            // top-left corner, original screen pixels
    float y = 0.0f;
    HudAnchor anchor = HudAnchor::Left;
    bool flippedVertically = false;  // drawn upside down (the lower half of the fan)
};

// x on a canvas `canvasWidth` units wide for a position of the original
// screen tied to `anchor`.
float anchoredX(float x, HudAnchor anchor, float canvasWidth);

// --- Angle gauge (top right) ---------------------------------------------

// The dial's frame, always shown during a turn.
std::span<const HudPiece> angleGaugeFrame();

// The four pale wedges of the fan, shown while the hog can aim.
std::span<const HudPiece> angleGaugeFan();

// The weapon icon (a 56 x 34 picture of MENUTIMS.MAD) sits in the dial's
// window here.
inline constexpr HudPiece kAngleGaugeIcon{"", 547.0f, 55.0f, HudAnchor::Right};

// The arrow (angpoint, 64 x 15) turns about this point, right of the dial.
inline constexpr glm::vec2 kAimPointerPivot{536.0f, 73.5f};
inline constexpr HudAnchor kAimPointerAnchor = HudAnchor::Right;

// The four corners of the arrow (top-left, top-right, bottom-right,
// bottom-left of its picture) for an aim `aimDegrees` above the horizon:
// 0 points left, 90 straight up, -90 straight down. `pivot` is the turning
// point on the canvas. The arrow's head is 50 pixels from the pivot, its
// tail 14 behind it, and it is 14 pixels thick.
std::array<glm::vec2, 4> aimPointerCorners(float aimDegrees, glm::vec2 pivot);

// --- Power gauge (bottom centre) -----------------------------------------

std::span<const HudPiece> powerGaugeFrame();

// The red lamp on the gauge, lit while the power is being charged.
inline constexpr HudPiece kPowerLamp{"pcpie4", 210.0f, 423.0f, HudAnchor::Centre};

// The needle (powg1): its y, and its x for a power of 0 .. 1 (the scale
// is 151 pixels long, 95 pixels into the tube).
inline constexpr float kPowerNeedleY = 426.0f;
float powerNeedleX(float power);

// --- Clock (bottom right) ------------------------------------------------

std::span<const HudPiece> clockFrame();

// The lamp on top of the clock (timlit), tinted by lampColour().
inline constexpr HudPiece kClockLamp{"timlit", 553.0f, 376.0f, HudAnchor::Right};

// The two windows the digits (timer0 .. timer9, 24 x 25) roll in: their
// left edges, and the top and bottom of both.
inline constexpr std::array<float, 2> kClockDigitX{540.0f, 566.0f};
inline constexpr float kClockWindowTop = 415.0f;
inline constexpr float kClockWindowBottom = 443.0f;
inline constexpr float kClockDigitHeight = 25.0f;

// What the two digits show for `secondsLeft`: whole seconds rounded up (so
// "1" until time is out), and how far each digit has rolled towards the
// next one down, in pixels (0 = still). As in the original, a digit rolls
// during the last 0.12 s before it changes, overshooting a little.
struct ClockDigits {
    int tens = 0;
    int units = 0;
    float tensRoll = 0.0f;
    float unitsRoll = 0.0f;
};
ClockDigits clockDigits(double secondsLeft);

// The clock lamp: green, amber under 10 seconds, red under 5. Once amber
// or red it stays so until a new turn starts with a full clock.
enum class ClockLamp { Green, Amber, Red };
ClockLamp nextClockLamp(ClockLamp current, double secondsLeft, bool turnJustStarted);
glm::vec4 lampColour(ClockLamp lamp);

// --- Animation -----------------------------------------------------------

// The original moves its instruments once per frame of its 25 frames per
// second: HudAnimation counts those ticks for any frame rate.
inline constexpr double kHudTickSeconds = 1.0 / 25.0;

// An instrument that slides in from below the screen in 19 ticks and back
// out the same way. offset() is how far below its place it is drawn.
class HudSlide {
public:
    static constexpr int kTicks = 19;
    static constexpr float kHiddenOffset = 200.0f;

    void tick(bool shown);
    void showAtOnce() { ticks_ = kTicks; }
    float offset() const;
    bool visible() const { return ticks_ > 0; }

private:
    int ticks_ = 0;
};

// The fan and the arrow of the angle gauge fade in by 3 steps per tick, up
// to 50, and out the same way. The fan is drawn at alpha level/255 (faint),
// the arrow at 5 x level/255 (opaque when fully in).
class HudFade {
public:
    static constexpr int kMaximum = 50;
    static constexpr int kStep = 3;

    void tick(bool shown);
    int level() const { return level_; }
    float fanAlpha() const;
    float pointerAlpha() const;

private:
    int level_ = 0;
};

// Runs the ticks of the HUD's animations for the time a frame took.
class HudTicker {
public:
    // How many ticks are due after `elapsedSeconds` more.
    int advance(double elapsedSeconds);

private:
    double carried_ = 0.0;
};

// --- Messages ------------------------------------------------------------

// The colour of a team's messages ("GET READY ..."), from the executable's
// table: green British, cyan American, blue French, white German, red
// Russian, yellow Japanese, magenta Team Lard. `nation` is an index into
// scene::characterTeams().
glm::vec4 teamMessageColour(int nation);

} // namespace openhogs::ui
