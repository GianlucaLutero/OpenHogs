// HogLabels.h
//
// The labels over the hogs, as the PC executable draws them (FUN_00459b20,
// called by the HUD routine FUN_00457840):
//
//   - every hog on the screen has one (a living hog, not hidden in a crate
//     nor inside a building or vehicle); an espionage hog's (Scout, Sniper,
//     Spy) only shows in its own team's turn, so the others cannot spot it;
//   - the label is a box of 70 x 48 pixels (of 640 x 480) standing on the
//     top of the hog as it appears on the screen: the hog is taken as a
//     square of 182400 / depth pixels round its centre, which with the
//     executable's focal length of 480 pixels is a hog 380 units tall;
//   - labels are drawn from the farthest hog to the nearest, and one that
//     would overlap another label or hog, or leave the screen, moves to a
//     corner of its hog (above right, above left, below left, below
//     right), with a curved arrow (FACETIMS.MAD `arrow000`) pointing at
//     the hog;
//   - the text is the hog's name or its rank (class), in turn every 1.28
//     seconds (bit 7 of the match clock in hundredths), and under it its
//     health, in the colour of its nation (VA 0x4CF1E0); the Boot Camp
//     shows no health; a hog without a name of its own takes one from its
//     nation's list, as the footage's enemies have (decision 175);
//   - the labels fade in while the camera stands still and fade out while
//     it moves (+25 and -40 of 255 per 1/25 s tick).
//
// Everything here is screen arithmetic and rules, without drawing.

#pragma once

#include "openhogs/game/World.h"
#include "openhogs/ui/UiStrings.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace openhogs::ui {

// A rectangle in the 640 x 480 screen of the original (x right, y down).
struct ScreenRect {
    glm::vec2 position{0.0f};  // top left corner
    glm::vec2 size{0.0f};

    bool overlaps(const ScreenRect& other) const;
    bool inside(glm::vec2 screen) const;
};

// A hog as it appears on the screen.
struct HogOnScreen {
    std::size_t hog = 0;
    glm::vec2 centre{0.0f};  // its centre projected, in 640 x 480 pixels
    float depth = 1.0f;      // its distance along the view, world units
};

// Where a label goes: on top of its hog, or moved to one of its corners.
enum class LabelPlace { Above, AboveRight, AboveLeft, BelowLeft, BelowRight };

struct HogLabelBox {
    std::size_t hog = 0;
    LabelPlace place = LabelPlace::Above;
    ScreenRect box;      // where the text goes
    ScreenRect hogRect;  // the hog's square
    // The arrow of a moved label, at the box's corner nearest the hog. Its
    // picture points down and to the left; it is mirrored for the others.
    std::optional<ScreenRect> arrow;
    bool arrowFlipX = false;
    bool arrowFlipY = false;
};

// The executable's numbers (FUN_00459b20).
inline constexpr float kLabelHogScale = 182400.0f;   // the hog's square = this / depth
inline constexpr float kOriginalFocalLength = 480.0f;  // pixels, of the 480 lines
inline constexpr glm::vec2 kLabelBoxSize{70.0f, 48.0f};
inline constexpr glm::vec2 kMovedLabelBoxSize{78.0f, 48.0f};
inline constexpr glm::vec2 kLabelArrowSize{44.0f, 30.0f};
// The point of the hog that is projected: its centre, this far above its
// feet (the map objects' position, see docs/formats/maps.md).
inline constexpr float kHogCentreHeight = 168.0f;

// Lays the labels out, from the farthest hog to the nearest (the order to
// draw them in). `focalLength` is the camera's, in pixels of the 480-line
// screen: the hogs' squares scale with it, so a wider view gives smaller
// hogs and labels nearer their heads. With `moveOverlapping` off (the
// executable's overview cameras, modes 7 and 9) every label stays on top
// of its hog.
std::vector<HogLabelBox> layoutHogLabels(std::vector<HogOnScreen> hogs, glm::vec2 screen, float focalLength,
                                         bool moveOverlapping);

// The hogs that may carry a label now (whether they are on the screen is
// the caller's business).
std::vector<std::size_t> labelledHogs(const game::World& world);

// What one label says.
struct HogLabelText {
    std::string text;        // name or rank (class)
    int health = 0;          // whole points, rounded up
    bool showHealth = true;  // not in the Boot Camp
    int nation = 0;          // scene::characterTeams index, for the colour
};

// A named hog's label switches between its name and its rank this often.
inline constexpr double kHogLabelSwapSeconds = 1.28;
HogLabelText describeHogLabel(const game::World& world, const UiStrings& strings, std::size_t hog);

// The labels' fading, as FUN_00457840: up while the camera stands still,
// down while it moves.
class HogLabelFade {
public:
    void update(double seconds, bool cameraStill);
    float alpha() const { return static_cast<float>(level_) / kFull; }  // 0 .. 1
    // The arrows of moved labels are drawn only when fully shown.
    bool full() const { return level_ >= kFull; }

private:
    static constexpr int kFull = 255;
    static constexpr int kRisePerTick = 25;
    static constexpr int kFallPerTick = 40;
    static constexpr double kTickSeconds = 1.0 / 25.0;
    int level_ = 0;
    double pending_ = 0.0;  // time not yet turned into whole ticks
};

} // namespace openhogs::ui
