// MessageBanner.cpp - see MessageBanner.h for an overview.

#include "openhogs/ui/MessageBanner.h"

#include <utility>

namespace openhogs::ui {

void MessageBanner::show(std::string text, double seconds) {
    text_ = std::move(text);
    totalSeconds_ = seconds;
    secondsLeft_ = seconds;
}

void MessageBanner::clear() {
    text_.reset();
}

void MessageBanner::update(double seconds) {
    if (!text_ || totalSeconds_ < 0.0) {
        return;  // nothing shown, or shown until cleared
    }
    secondsLeft_ -= seconds;
    if (secondsLeft_ <= 0.0) {
        text_.reset();
    }
}

double MessageBanner::progress() const {
    if (!text_ || totalSeconds_ <= 0.0) {
        return 0.0;
    }
    return 1.0 - secondsLeft_ / totalSeconds_;
}

} // namespace openhogs::ui
