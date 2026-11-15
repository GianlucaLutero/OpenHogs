// OriginalSquad.cpp - see OriginalSquad.h for an overview.
//
// From the executable's front end (docs/formats/front-end.md): the drawing
// function FUN_0041bef0 (case 5: the boards, the faces, the options, the
// monitor, the plate; case 6: a hog's page), the foreground FUN_0041e780
// (case 5: the selectors) and the update FUN_00423e70 (cases 5 and 6).

#include "openhogs/ui/OriginalSquad.h"

#include <array>
#include <cstdio>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
constexpr int kPlateStartY = -150;  // not read: as the other plates
constexpr int kPlateGain = 3, kPlateDamping = 30, kPlateMaxSpeed = 6;
constexpr int kOptionsStartY = 0x3FC;
constexpr int kOptionsGain = 12, kOptionsDamping = 20, kOptionsMaxSpeed = 40;
constexpr int kMonitorStartY = 400;
constexpr int kMonitorGain = 5, kMonitorDamping = 14, kMonitorMaxSpeed = 40;
constexpr int kHogPageOutY = 500;
constexpr int kHogPageGain = 12, kHogPageDamping = 20, kHogPageMaxSpeed = 40;
constexpr int kStepsPerRow = 5;
constexpr int kLitLamp = 2;

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr std::size_t kRowsPerBoard = 5;
constexpr float kLeftBoardX = -32.0f, kRightBoardEndX = 672.0f;  // the right one mirrored
constexpr float kBoardY = -12.0f;          // sqpic
constexpr float kTurningBoardY = 20.0f;    // sqpics00-10
constexpr float kFaceX = 57.0f, kRightFaceShift = 462.0f;
constexpr float kFaceY = 74.0f, kRowStep = 74.0f, kLastFaceY = 368.0f;
// The name bars (columns 165-275 of sqpic, rows 100-135).
constexpr float kLeftNameX = kLeftBoardX + 220.0f, kRightNameX = kRightBoardEndX - 220.0f;
constexpr float kNameMiddleY = kBoardY + 117.0f;
constexpr float kSelectorX = -32.0f, kSelectorY = 64.0f;
constexpr float kRightSelectorEndX = 672.0f, kRightSelectorY = 63.0f;
constexpr float kLeftLampsX[2] = {30.0f, 111.0f};
constexpr float kRightLampsX[2] = {501.0f, 582.0f};
constexpr float kLampY = 84.0f;
// The options' panels: rows 187 and 215 of the executable's table, plus 27.
constexpr std::array<float, kSquadOptions> kOptionRow{214.0f, 242.0f};
constexpr float kOptionX = 428.0f, kOptionLampsX[2] = {429.0f, 601.0f};
constexpr float kOptionPanelDrop = -91.0f, kOptionLampDrop = -75.0f;
constexpr float kOptionTextX = 528.0f, kOptionTextDrop = -55.0f;  // middle of sqoptsF's plate
constexpr float kMonitorX = 232.0f, kMonitorY = 304.0f;
constexpr float kMonitorCentreX = kMonitorX + 100.0f, kMonitorTextTop = kMonitorY + 30.0f, kMonitorLine = 22.0f;
constexpr float kPlateX = 120.0f, kPlateDrop = -14.0f, kPlateCentre = 200.0f, kTitleTop = 24.0f;
// A hog's page: swap.mgl1 rows 0-245 and rows 212-256 again below.
constexpr float kHogPageX = 180.0f, kHogPageY = 260.0f;
constexpr float kHogArrowX = 230.0f, kHogArrowY = 347.0f;
constexpr float kHogLineStep = 33.0f;
constexpr float kHogTextCentreX = 336.0f, kHogTextTop = 348.0f;

int toPixelsY(int units) { return units * 480 / 820; }

std::string numbered(const char* format, int number) {
    char name[32];
    std::snprintf(name, sizeof name, format, number);
    return name;
}

float faceY(std::size_t row) { return row == kRowsPerBoard - 1 ? kLastFaceY : kFaceY + kRowStep * static_cast<float>(row); }

void stepLamp(int& lamp, bool lit) {
    if (lit && lamp < kLitLamp) ++lamp;
    if (!lit && lamp > 0) --lamp;
}

} // namespace

void OriginalSquad::open(std::size_t selected) {
    boardFrame_ = 0;
    plateY_ = kPlateStartY;
    plateVelocity_ = 0;
    optionsY_ = kOptionsStartY;
    optionsVelocity_ = 0;
    monitorY_ = kMonitorStartY;
    monitorVelocity_ = 0;
    hogPage_ = false;
    hogPageY_ = kHogPageOutY;
    left_ = {};
    right_ = {};
    optionLamps_.assign(kSquadOptions, 0);
    select(selected);
    left_.position = left_.target;
    right_.position = right_.target;
}

void OriginalSquad::select(std::size_t selected) {
    selected_ = selected;
    if (selected < kRowsPerBoard) {
        left_.target = static_cast<int>(selected) * kStepsPerRow;
    } else if (selected < kSquadHogs) {
        right_.target = static_cast<int>(selected - kRowsPerBoard) * kStepsPerRow;
    }
}

void OriginalSquad::openHogPage(std::size_t line) {
    hogPage_ = true;
    hogLine_ = line;
}

void OriginalSquad::selectHogLine(std::size_t line) { hogLine_ = line; }

void OriginalSquad::closeHogPage() { hogPage_ = false; }

void OriginalSquad::tick() {
    if (boardFrame_ < kBoardOpenFrame) ++boardFrame_;
    plateY_ = frontEndSpring(plateY_, 0, plateVelocity_, kPlateGain, kPlateDamping, kPlateMaxSpeed);
    optionsY_ = frontEndSpring(optionsY_, 0, optionsVelocity_, kOptionsGain, kOptionsDamping, kOptionsMaxSpeed);
    monitorY_ = frontEndSpring(monitorY_, 0, monitorVelocity_, kMonitorGain, kMonitorDamping, kMonitorMaxSpeed);
    hogPageY_ = frontEndSpring(hogPageY_, hogPage_ ? 0 : kHogPageOutY, hogPageVelocity_, kHogPageGain, kHogPageDamping,
                               kHogPageMaxSpeed);
    for (Selector* s : {&left_, &right_}) {
        if (s->position < s->target) ++s->position;
        if (s->position > s->target) --s->position;
    }
    stepLamp(left_.lamp, selected_ < kRowsPerBoard);
    stepLamp(right_.lamp, selected_ >= kRowsPerBoard && selected_ < kSquadHogs);
    for (std::size_t i = 0; i < kSquadOptions; ++i) stepLamp(optionLamps_[i], selected_ == kSquadHogs + i);
}

void OriginalSquad::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalSquad::settled() const {
    return boardFrame_ == kBoardOpenFrame && plateY_ == 0 && plateVelocity_ == 0 && optionsY_ == 0 &&
           optionsVelocity_ == 0 && monitorY_ == 0 && monitorVelocity_ == 0 && left_.position == left_.target &&
           right_.position == right_.target && hogPageY_ == (hogPage_ ? 0 : kHogPageOutY) && hogPageVelocity_ == 0;
}

bool OriginalSquad::showsHogPage() const { return hogPage_ && hogPageY_ < 10; }

std::vector<MenuPiece> OriginalSquad::pieces(std::size_t hogs) const {
    std::vector<MenuPiece> result;
    const auto add = [&](std::string picture, float x, float y) -> MenuPiece& {
        MenuPiece piece;
        piece.picture = std::move(picture);
        piece.x = x;
        piece.y = y;
        result.push_back(piece);
        return result.back();
    };

    // On top: a hog's page.
    if (hogPageY_ < kHogPageOutY) {
        const float dy = 2.0f * static_cast<float>(toPixelsY(hogPageY_));
        add(hogLine_ % 2 == 0 ? "swap01" : "swap02", kHogArrowX, kHogArrowY + kHogLineStep * static_cast<float>(hogLine_) + dy);
        MenuPiece& top = add("swap.mgl1", kHogPageX, kHogPageY + dy);
        top.sourceHeight = 245;
        MenuPiece& bottom = add("swap.mgl1", kHogPageX, kHogPageY + 245.0f + dy);
        bottom.sourceY = 212;
        bottom.sourceHeight = 44;
    }

    // The selectors (the executable's foreground).
    if (showsText()) {
        const float leftRow = 2.0f * static_cast<float>(left_.position * 37 / 5);
        const float rightRow = 2.0f * static_cast<float>(right_.position * 37 / 5);
        if (hogs > kRowsPerBoard) {
            for (const float x : kRightLampsX) add(numbered("lit%d", right_.lamp + 1), x, kLampY + rightRow);
            MenuPiece& dial = add("sqdial01", kRightSelectorEndX, kRightSelectorY + rightRow);
            dial.mirrored = true;
        }
        for (const float x : kLeftLampsX) add(numbered("lit%d", left_.lamp + 1), x, kLampY + leftRow);
        add("sqdial01", kSelectorX, kSelectorY + leftRow);
    }

    // The options' panels, the last one on top.
    const float options = static_cast<float>(toPixelsY(optionsY_));
    for (std::size_t k = kSquadOptions; k-- > 0;) {
        const float row = 2.0f * (kOptionRow[k] + options);
        for (const float x : kOptionLampsX) add(numbered("lit%d", optionLamps_[k] + 1), x, row + kOptionLampDrop);
        add("sqoptsF", kOptionX, row + kOptionPanelDrop);
    }

    // The faces in the boards' windows.
    if (showsText()) {
        for (std::size_t hog = 0; hog < hogs && hog < kSquadHogs; ++hog) {
            const bool right = hog >= kRowsPerBoard;
            const std::size_t row = right ? hog - kRowsPerBoard : hog;
            const float x = kFaceX + (right ? kRightFaceShift : 0.0f);
            add(numbered("face%da", static_cast<int>(hog % 9) + 1), x, faceY(row));
            add("backgr~1", x + 3.0f, faceY(row) + 3.0f);
        }
    }

    // The boards, the monitor, the plate.
    if (boardFrame_ == kBoardOpenFrame) {
        add("sqpic", kLeftBoardX, kBoardY);
        MenuPiece& mirror = add("sqpic", kRightBoardEndX, kBoardY);
        mirror.mirrored = true;
    } else {
        const std::string turning = numbered("sqpics%02d", boardFrame_ * 2);
        add(turning, kLeftBoardX, kTurningBoardY);
        MenuPiece& mirror = add(turning, kRightBoardEndX, kTurningBoardY);
        mirror.mirrored = true;
    }
    add("pigpro", kMonitorX, kMonitorY + 2.0f * static_cast<float>(toPixelsY(monitorY_)));
    add("sqname01", kPlateX, kPlateDrop + 2.0f * static_cast<float>(toPixelsY(plateY_)));
    return result;
}

MenuTextLine OriginalSquad::titleLine() const {
    return {kPlateX + kPlateCentre, kTitleTop + 2.0f * static_cast<float>(toPixelsY(plateY_))};
}

MenuTextLine OriginalSquad::nameLine(std::size_t hog) const {
    const bool right = hog >= kRowsPerBoard;
    const std::size_t row = right ? hog - kRowsPerBoard : hog;
    return {right ? kRightNameX : kLeftNameX, kNameMiddleY + kRowStep * static_cast<float>(row)};
}

MenuTextLine OriginalSquad::optionLine(std::size_t option) const {
    const float row = 2.0f * (kOptionRow[option < kSquadOptions ? option : 0] + static_cast<float>(toPixelsY(optionsY_)));
    return {kOptionTextX, row + kOptionTextDrop};
}

MenuTextLine OriginalSquad::monitorLine(std::size_t line) const {
    return {kMonitorCentreX,
            kMonitorTextTop + kMonitorLine * static_cast<float>(line) + 2.0f * static_cast<float>(toPixelsY(monitorY_))};
}

MenuTextLine OriginalSquad::hogPageLine(std::size_t line) const {
    return {kHogTextCentreX, kHogTextTop + kHogLineStep * static_cast<float>(line)};
}

} // namespace openhogs::ui
