// OriginalVolume.h
//
// The original game's VOLUME CONTROL screen (screen 20, type 7 of the PC
// executable's front end; see docs/formats/front-end.md), as it draws and
// animates it:
//
//   - the OPTIONS list menu stays on screen and rises out of the way
//     (ui::OriginalMenu::raise);
//   - a brass panel (`vol01`-`vol06`, then `vol00` open) comes up from the
//     bottom of the screen with a spring and unfolds;
//   - once open, two knobs (`vollit01`-`vollit03`: dark to lit) slide on its
//     rails to show the MASTER and SFX levels; the selected knob lights up;
//   - the four entries, MODE, MASTER, SFX and VOICES, are written on the
//     panel, MODE and VOICES followed by their value (STEREO, ON...), which
//     blinks while selected.
//
// Positions are pixels of the original 640 x 480 screen and the animation
// counts in front-end ticks, as ui::OriginalMenu. Nothing here draws.

#pragma once

#include "openhogs/ui/OriginalMenu.h"

#include <array>
#include <cstddef>
#include <vector>

namespace openhogs::ui {

// The entries of VOLUME CONTROL, in the original's order.
inline constexpr std::size_t kVolumeEntries = 4;
inline constexpr std::size_t kVolumeMasterEntry = 1;
inline constexpr std::size_t kVolumeEffectsEntry = 2;

// Where an entry of the panel is written: its label ends at labelRight,
// its value (if any) starts at valueLeft, both centred on `middle`.
struct VolumeTextLine {
    float labelRight = 0.0f;
    float valueLeft = 0.0f;
    float middle = 0.0f;
};

class OriginalVolumePanel {
public:
    // Starts the panel coming up, `selected` being the selected entry.
    void open(std::size_t selected);

    // The panel goes back down (the list menu returns once it is gone).
    void close();

    // The player moved to another entry: its knob lights, its value blinks.
    void select(std::size_t entry);

    // One tick of the front end's animation.
    void tick();
    void advance(double seconds);

    // The pictures to draw, top one first: the knobs (once open) on the
    // panel. `masterLevel` and `effectsLevel` go from 0 to 10.
    std::vector<MenuPiece> pieces(int masterLevel, int effectsLevel) const;

    // The panel is open (unfolded and at rest enough to write on).
    bool showsText() const { return frame_ == kOpenFrame; }
    VolumeTextLine textLine(std::size_t entry) const;

    // Whether the selected entry's value is lit at this moment (it blinks).
    bool valueLit() const;

    // Gone below the screen after close(): the list menu may come down.
    bool closed() const;

    // State, for tests.
    int panelY() const { return panelY_; }
    int frame() const { return frame_; }
    int knobFrame(std::size_t knob) const { return knobFrames_.at(knob); }
    bool settled() const;

    static constexpr int kOpenFrame = 6;

private:
    float offsetY() const;  // the panel's distance from its place, pixels

    int panelY_ = 0;        // 820-high units below its place (DAT_00512958)
    int panelVelocity_ = 0;
    bool closing_ = false;
    int frame_ = kOpenFrame;  // 0..5: vol01..vol06; 6: vol00, open
    std::size_t selected_ = 0;
    std::array<int, 2> knobFrames_{};  // 0..2: vollit01..03, MASTER and SFX
    int blink_ = 0;                    // the executable's 0..100 counter
    double pending_ = 0.0;
};

} // namespace openhogs::ui
