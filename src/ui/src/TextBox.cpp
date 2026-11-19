// TextBox.cpp - see TextBox.h for an overview.

#include "openhogs/ui/TextBox.h"

#include <array>

namespace openhogs::ui {

namespace {

// How far down the tube is at each step, in percent (0x4D0190).
constexpr std::array<int, TextBox::kSlideSteps> kSlidePercent{0, 10, 15, 30, 40, 90, 100, 95, 100, 100};

} // namespace

void TextBox::post(const std::string& text, float textWidth, float scrollPerTick) {
    if (text.empty() || queue_.size() >= kQueueSize) return;
    queue_.push_back({TextBoxKind::Message, text, textWidth, scrollPerTick});
    if (!showing_) start();
}

void TextBox::postTeamStrength() {
    if (queue_.size() >= kQueueSize) return;
    queue_.push_back({TextBoxKind::TeamStrength, {}, 0.0f, 0.0f});
    if (!showing_) start();
}

void TextBox::start() {
    showing_ = true;
    stage_ = Stage::Lowering;
    step_ = 0;
    scrolled_ = 0.0f;
    const auto& entry = queue_.front();
    ticksLeft_ = entry.kind == TextBoxKind::TeamStrength ? kTeamStrengthTicks : kMessageTicks;
}

bool TextBox::scrolls() const {
    const auto* entry = current();
    return entry && entry->kind == TextBoxKind::Message && entry->textWidth > kWindowWidth;
}

void TextBox::tick() {
    if (!showing_) return;

    // The clock runs from the moment the tube starts down; scrolling text
    // moves the whole time, even while the tube is still coming.
    if (ticksLeft_ > 0) --ticksLeft_;
    const bool scrolling = scrolls();
    if (scrolling) scrolled_ += queue_.front().scrollPerTick;

    switch (stage_) {
    case Stage::Lowering:
        if (step_ < kSlideSteps - 1) {
            ++step_;
        } else {
            stage_ = Stage::Shown;
        }
        break;
    case Stage::Shown: {
        const bool done = scrolling ? scrolled_ >= kWindowWidth + queue_.front().textWidth : ticksLeft_ == 0;
        if (done) stage_ = Stage::Raising;
        break;
    }
    case Stage::Raising:
        if (step_ > 0) {
            --step_;
        } else {
            queue_.pop_front();
            showing_ = false;
            if (!queue_.empty()) start();
        }
        break;
    }
}

float TextBox::lowered() const {
    if (!showing_) return 0.0f;
    return static_cast<float>(kSlidePercent[static_cast<std::size_t>(step_)]) / 100.0f;
}

float TextBox::textX() const {
    const auto* entry = current();
    if (!entry) return 0.0f;
    if (!scrolls()) return (kWindowWidth - entry->textWidth) / 2.0f;
    return kWindowWidth - scrolled_;
}

} // namespace openhogs::ui
