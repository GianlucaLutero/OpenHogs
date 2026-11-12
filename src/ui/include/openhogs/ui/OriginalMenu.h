// OriginalMenu.h
//
// The original game's list menu (the main menu and the other short menus
// of the front end), as the PC executable (warhogs_.exe) draws and animates
// it; see docs/formats/front-end.md.
//
// The screen is a brass machine built from the pictures of
// FEbmps/FEBMP.MAD:
//
//   - a panel (`fullmenu`) with four bars (`chose1`-`chose6`) that hold the
//     entries, and a plate (`title1`-`title6`) that holds the title;
//   - a lamp next to each bar (`light1`-`light3`: off, half lit, lit); the
//     lamp of the selected entry blinks;
//   - a dial (`dial0001`-`dial0012`) whose green arrow turns to point at
//     the selected entry;
//   - cogs (`cog0`, `cogB00`) and a toothed rack (`track`) at each side of
//     the screen.
//
// When the menu opens, the racks slide in from the sides, then the panel
// drops from the top of the screen with a spring, then the plates flip
// over (frames 2 to 6 of the bars and the title plate).
//
// Positions are those of the executable, in the pixels of its 640 x 480
// screen; the animation counts in the executable's front-end ticks. Nothing
// here draws: the game application draws the pieces where this says, so
// the arithmetic is unit tested.

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace openhogs::ui {

// The front end's springs (the executable's FUN_0041fd30): moves `value`
// one tick towards `target`. `velocity` is the spring's state, kept by the
// caller; `gain` pulls (tenths), `damping` slows, `maxSpeed` caps the
// velocity. Whole numbers, as the original, so the motion is the same.
int frontEndSpring(int value, int target, int& velocity, int gain, int damping, int maxSpeed);

// The rate at which the original's front end animates.
inline constexpr double kFrontEndTicksPerSecond = 25.0;

// The list menu shows at most this many entries (four bars, four lamps).
inline constexpr std::size_t kOriginalMenuEntries = 4;

// One picture of the menu at its place, in original screen pixels.
struct MenuPiece {
    std::string picture;  // entry of FEBMP.MAD, without ".mgl"
    // The part of the picture to draw (pixels); width 0 means all of it.
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    // Where it goes; the size is the source size unless stretched.
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;   // 0: the source width
    float height = 0.0f;  // 0: the source height
    bool mirrored = false;  // drawn flipped left to right, ending at x
};

// A line of text of the menu: centred on x, its top at y.
struct MenuTextLine {
    float centreX = 0.0f;
    float top = 0.0f;
};

// The animation and the layout of the list menu.
class OriginalMenu {
public:
    // Starts the opening animation (racks, panel, plates).
    void open(std::size_t selected);

    // Shows another list menu while this one is open: only the plates flip.
    void replaceContents(std::size_t selected);

    // The player moved to another entry: the dial turns, its lamp blinks.
    void select(std::size_t entry);

    // One tick of the front end's animation.
    void tick();

    // Advances by `seconds`, in whole ticks.
    void advance(double seconds);

    // The pictures to draw, from the top one down to the panel at the
    // bottom (the executable's order: the first piece covers the others).
    std::vector<MenuPiece> pieces() const;

    // Where the title and the entries are written; empty while the plates
    // are turned away (they show no text then).
    bool showsText() const { return plateFrame_ % 6 == 0; }
    MenuTextLine titleLine() const;
    MenuTextLine entryLine(std::size_t entry) const;

    // State, for tests and for the sound of the dial.
    int panelY() const { return panelY_; }
    int rackOffset() const { return rackOffset_; }
    int dialFrame() const { return dialFrame_; }
    int plateFrame() const { return plateFrame_; }
    int lampFrame(std::size_t entry) const { return lamps_.at(entry).frame; }
    bool settled() const;

private:
    // A lamp: either fading to a frame or playing the blink sequence.
    struct Lamp {
        int frame = 0;
        bool blinking = false;
        std::size_t step = 0;  // in the blink sequence
        int wait = 0;          // ticks before the next step
    };

    void tickLamp(Lamp& lamp) const;
    int offsetX() const;  // the executable's horizontal offset of the sprites
    int offsetY() const;  // the panel's height in screen pixels

    int panelY_ = 0;        // the panel's drop, in the executable's 820-high units
    int panelVelocity_ = 0;
    int rackOffset_ = 0;    // the racks' distance from their place, pixels
    int rackVelocity_ = 0;
    int dialFrame_ = 0;     // 0..11: dial0001..dial0012
    int dialTarget_ = 0;
    int plateFrame_ = 6;    // 2..6, shown as picture frame % 6
    std::array<Lamp, kOriginalMenuEntries> lamps_{};
    double pending_ = 0.0;  // seconds not yet turned into ticks
};

} // namespace openhogs::ui
