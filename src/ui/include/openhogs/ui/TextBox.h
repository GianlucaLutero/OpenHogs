// TextBox.h
//
// The original's text box: a brass tube that comes down from the top of
// the screen on its pipes to show a message ("BAZOOKA X3", ">S FINDS
// USEFUL PROVISIONS!", the Boot Camp's instructions), then goes back up
// (FUN_0045e6a0, FUN_0045ee30; pictures npro1-npro4 of
// Language/Tims/tboxtims.mad; see docs/formats/hud.md, "Text box").
//
// - Messages wait in a queue of eight and are shown one after the other.
// - The tube comes down in 10 ticks (25 per second) along the table at
//   0x4D0190 (0, 10, 15, 30, 40, 90, 100, 95, 100, 100 %: it overshoots
//   and bounces) and goes back up the same way.
// - A message that fits the tube's window (206 pixels) is centred and
//   stays 3 seconds from the moment the tube starts down; a longer one
//   scrolls through the window from right to left, a few pixels a tick,
//   and the tube goes up once it has gone by.
// - The bigger tube (npro1/npro2) shows the teams' strength instead: a bar
//   split between the teams by the health they have left, for 4 seconds,
//   at the start of a turn.
//
// This class only keeps the state; OriginalHud draws it. Distances are in
// the original's 640 x 480 pixels.

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>

namespace openhogs::ui {

enum class TextBoxKind {
    Message,       // the small tube with a line of text
    TeamStrength,  // the big tube with the teams' strength bar
};

struct TextBoxEntry {
    TextBoxKind kind = TextBoxKind::Message;
    std::string text;
    float textWidth = 0.0f;       // in pixels, as it will be drawn
    float scrollPerTick = 0.0f;   // pixels a tick when it scrolls
};

class TextBox {
public:
    // The window the text shows through, and the queue's size.
    static constexpr float kWindowWidth = 206.0f;
    static constexpr std::size_t kQueueSize = 8;
    // How long a box stays when its text needs no scrolling, in ticks
    // (the executable: 300 and 400 hundredths of a second).
    static constexpr int kMessageTicks = 75;
    static constexpr int kTeamStrengthTicks = 100;
    static constexpr int kSlideSteps = 10;
    // Scrolling speeds the executable asks for: crate messages, the Boot
    // Camp's instructions.
    static constexpr float kPickupScroll = 3.0f;
    static constexpr float kTrainingScroll = 5.0f;

    // Adds a message; ignored when the queue is full.
    void post(const std::string& text, float textWidth, float scrollPerTick = kPickupScroll);
    void postTeamStrength();

    // One tick (25 per second).
    void tick();

    // The box on screen, if any.
    const TextBoxEntry* current() const { return showing_ ? &queue_.front() : nullptr; }

    // How far down the tube has come: 0 (out of sight) to 1 (in place),
    // past 1 while it bounces.
    float lowered() const;

    // Where the text starts, from the window's left edge (negative once it
    // has scrolled past the edge). The text is cut to the window.
    float textX() const;

    bool scrolls() const;

private:
    enum class Stage { Lowering, Shown, Raising };

    void start();

    std::deque<TextBoxEntry> queue_;
    bool showing_ = false;
    Stage stage_ = Stage::Lowering;
    int step_ = 0;        // 0 .. kSlideSteps - 1 along the slide table
    int ticksLeft_ = 0;   // for boxes that do not scroll
    float scrolled_ = 0.0f;
};

} // namespace openhogs::ui
