// MessageBanner.h
//
// The big messages shown in the middle of the screen during a match, such
// as "GET READY TOMMY'S TROTTERS..." at the start of a turn or "VICTORY TO
// SUSHI-SWINE!!" at the end. A message stays for a while and then goes; a
// new message replaces the current one straight away (the newest news is
// the one that matters).

#pragma once

#include <optional>
#include <string>

namespace openhogs::ui {

class MessageBanner {
public:
    // Shows `text` for `seconds`; a negative time keeps it until clear().
    void show(std::string text, double seconds);
    void clear();

    // Advances the clock by `seconds`.
    void update(double seconds);

    // The message to show now, if any.
    const std::optional<std::string>& current() const { return text_; }

    // 0 .. 1: how far the current message is through its time (for fading
    // it out); 0 for a message without an end.
    double progress() const;

private:
    std::optional<std::string> text_;
    double totalSeconds_ = 0.0;
    double secondsLeft_ = 0.0;
};

} // namespace openhogs::ui
