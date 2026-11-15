// OriginalCareer.cpp - see OriginalCareer.h for an overview.
//
// From the executable's front end (docs/formats/front-end.md): the drawing
// function FUN_0041bef0 (cases 0 and 13: the board, the bar), the
// foreground FUN_0041e780 (case 13: the icons) and the update FUN_00423e70
// (case 0).

#include "openhogs/ui/OriginalCareer.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
constexpr int kBoardStartX = -800, kBoardStartY = -250, kBoardRestY = 70;
constexpr int kBoardGain = 15, kBoardDamping = 30, kBoardMaxSpeed = 30;
constexpr int kBarStartY = -700;
constexpr int kBarGain = 10, kBarDamping = 17, kBarMaxSpeed = 50;
constexpr int kOpenFrame = 6;
constexpr int kTextShownY = 10;  // units from the board's place
// The chosen icon is hidden for 5 ticks out of 11 (counter 5 .. -5).
constexpr int kBlinkHigh = 5, kBlinkLow = -5;

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kBoardBaseX = 130.0f + 35.0f;
// The x shift of each unfolding picture (they differ in width).
constexpr std::array<float, 7> kFrameShiftX{51.0f, 35.0f, 35.0f, 3.0f, 3.0f, 3.0f, 3.0f};
constexpr float kBoardTopY = -48.0f;    // alpha at 2 × drop − 48
constexpr float kPostY1 = -74.0f, kPostY2 = -102.0f;  // its rows 2-30 drawn again above
constexpr int kPostTop = 2, kPostRows = 28;
constexpr float kBarLeftX = 184.0f, kBarRightX = 432.0f, kBarY = 4.0f;
constexpr float kBarPieceWidth = 20.0f;
constexpr float kBarPoleHeight = 100.0f;
constexpr float kIconsX = 260.0f, kIconStep = 30.0f, kIconsY = 408.0f;
constexpr std::array<const char*, kCareerIcons> kIcons{"pcHweap", "sappr", "snipr", "pcmedic"};
// Words: the title in the bar, the lines on the monitor's dark screen
// (x 205-420, y 140-360 of the board at rest).
constexpr float kTitleCentreX = 318.0f, kTitleTop = 24.0f;
constexpr float kMonitorCentreX = 312.0f, kMonitorTop = 150.0f, kMonitorLine = 24.0f;

int toPixelsY(int units) { return units * 480 / 820; }

} // namespace

void OriginalCareerBoard::open(std::size_t selected) {
    boardX_ = kBoardStartX;
    boardXVelocity_ = 0;
    boardY_ = kBoardStartY;
    boardYVelocity_ = 0;
    barY_ = kBarStartY;
    barVelocity_ = 0;
    frame_ = 0;
    blink_ = 0;
    selected_ = selected;
}

void OriginalCareerBoard::tick() {
    boardX_ = frontEndSpring(boardX_, 0, boardXVelocity_, kBoardGain, kBoardDamping, kBoardMaxSpeed);
    boardY_ = frontEndSpring(boardY_, kBoardRestY, boardYVelocity_, kBoardGain, kBoardDamping, kBoardMaxSpeed);
    barY_ = frontEndSpring(barY_, 0, barVelocity_, kBarGain, kBarDamping, kBarMaxSpeed);
    if (frame_ < kOpenFrame && std::abs(boardY_ - kBoardRestY) < 100) ++frame_;
    if (--blink_ < kBlinkLow) blink_ = kBlinkHigh;
}

void OriginalCareerBoard::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalCareerBoard::settled() const {
    return boardX_ == 0 && boardXVelocity_ == 0 && boardY_ == kBoardRestY && boardYVelocity_ == 0 && barY_ == 0 &&
           barVelocity_ == 0 && frame_ == kOpenFrame;
}

bool OriginalCareerBoard::showsText() const {
    return frame_ == kOpenFrame && std::abs(boardY_ - kBoardRestY) <= kTextShownY && std::abs(boardX_) <= kTextShownY;
}

float OriginalCareerBoard::boardX() const { return static_cast<float>(boardX_ * 640 / 1024) + kBoardBaseX; }
float OriginalCareerBoard::boardY() const { return 2.0f * static_cast<float>(toPixelsY(boardY_)); }
float OriginalCareerBoard::barY() const { return static_cast<float>(toPixelsY(barY_)) + kBarY; }

std::vector<MenuPiece> OriginalCareerBoard::pieces(bool careerIcons) const {
    std::vector<MenuPiece> result;
    const auto add = [&](std::string picture, float x, float y) -> MenuPiece& {
        MenuPiece piece;
        piece.picture = std::move(picture);
        piece.x = x;
        piece.y = y;
        result.push_back(piece);
        return result.back();
    };
    // The career icons (the executable's foreground), the chosen one blinking.
    if (careerIcons && showsText()) {
        for (std::size_t i = 0; i < kCareerIcons; ++i) {
            if (i == selected_ && blink_ > 0) continue;
            add(kIcons[i], kIconsX + kIconStep * static_cast<float>(i), kIconsY);
        }
    }
    // The bar: left end, middle pieces, right end, and the poles above.
    const float bar = barY();
    MenuPiece& left = add("propoint", kBarLeftX, bar);
    left.sourceWidth = 20;
    for (float x = kBarLeftX + kBarPieceWidth; x < kBarRightX; x += kBarPieceWidth) {
        MenuPiece& middle = add("propoint", x, bar);
        middle.sourceX = 20;
        middle.sourceWidth = 20;
    }
    MenuPiece& right = add("propoint", kBarRightX, bar);
    right.sourceX = 40;
    right.sourceWidth = 20;
    for (const float x : {kBarLeftX, kBarRightX}) {
        MenuPiece& pole = add("propoint", x, bar - kBarPoleHeight);
        pole.sourceX = x == kBarLeftX ? 0 : 40;
        pole.sourceWidth = 20;
        pole.sourceHeight = 1;
        pole.width = 20.0f;
        pole.height = kBarPoleHeight;
    }
    // The board, its top rows drawn again above it as a post.
    char name[32];
    std::snprintf(name, sizeof name, "alpha%02d", frame_ + 2);
    const float x = boardX() + kFrameShiftX[static_cast<std::size_t>(frame_)];
    const float y = boardY();
    for (const float postY : {kPostY1, kPostY2}) {
        MenuPiece& post = add(name, x, y + postY);
        post.sourceY = kPostTop;
        post.sourceHeight = kPostRows;
    }
    add(name, x, y + kBoardTopY);
    return result;
}

MenuTextLine OriginalCareerBoard::titleLine() const { return {kTitleCentreX, barY() + kTitleTop - kBarY}; }

MenuTextLine OriginalCareerBoard::monitorLine(std::size_t line) const {
    return {kMonitorCentreX, kMonitorTop + kMonitorLine * static_cast<float>(line) + boardY() -
                                 2.0f * static_cast<float>(toPixelsY(kBoardRestY))};
}

} // namespace openhogs::ui
