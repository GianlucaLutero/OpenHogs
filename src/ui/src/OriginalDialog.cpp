// OriginalDialog.cpp - see OriginalDialog.h for an overview.
//
// The numbers come from the executable's front end (docs/formats/front-end.md):
// the drawing code shared by screen types 3, 4, 11 and 12 (FUN_0041bef0 at
// 0x41C492), the pictures each element shows (FUN_0041f110, type 12), the
// opening values (FUN_0041b250) and the update of type 12 (FUN_00423e70).

#include "openhogs/ui/OriginalDialog.h"

#include <cstdio>
#include <string>

namespace openhogs::ui {

namespace {

// --- The opening animation -------------------------------------------------
constexpr int kPlateStartY = -150;   // the plate starts above its place
constexpr int kBoxStartX = 0x4C8;    // the box starts off the right of the screen
constexpr int kPlateGain = 3, kPlateDamping = 30, kPlateMaxSpeed = 6;
constexpr int kBoxGain = 10, kBoxDamping = 40, kBoxMaxSpeed = 20;
// The dial's pictures for YES and NO.
constexpr int kDialFramesPerAnswer = 6;

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kBoxShift = 7.0f;  // the executable's +7 on the box's offset
constexpr float kLeftArmX = 80.0f, kRightArmX = 435.0f;
constexpr float kPlateX = 123.0f, kPlateDrop = 16.0f;
constexpr float kTrayX = 368.0f, kTrayY = 98.0f;
constexpr float kBoxX = 280.0f, kBoxY = 345.0f;
constexpr float kOpenBoxShift = 32.0f;  // `yes` is narrower than yesno01-06
constexpr float kDialX = 392.0f, kDialY = 393.0f;
constexpr float kTurnedDialShift = 16.0f;  // the narrower dial pictures
constexpr float kPipeX = 312.0f, kPipeY = 130.0f;
constexpr float kPipeSideX = 359.0f;
constexpr int kPipeSideColumn = 35;
constexpr float kPipeFootY = 290.0f;
constexpr float kRiserX = 608.0f, kRiserY = 72.0f;
constexpr int kRiserTopRows = 39;

// Where the words go: the middle of the plate's glass and of the box's
// windows (measured on the pictures; the executable's text boxes, at
// 1024 x 820 units, do not line up with these pictures, see
// docs/open-decisions.md).
constexpr float kPlateGlassCentre = 162.0f;
constexpr float kTitleTop = 77.0f * 480.0f / 820.0f;  // the executable's title box
constexpr float kYesWindowCentre = 84.0f, kNoWindowCentre = 154.0f;
constexpr float kWindowTextTop = 38.0f;  // windows from row 36 to 58

std::string numbered(const char* format, int number) {
    char name[32];
    std::snprintf(name, sizeof name, format, number);
    return name;
}

} // namespace

void OriginalDialog::open(std::size_t selected) {
    plateY_ = kPlateStartY;
    plateVelocity_ = 0;
    boxX_ = kBoxStartX;
    boxVelocity_ = 0;
    boxFrame_ = 0;
    dialFrame_ = 0;
    select(selected);
    dialFrame_ = dialTarget_;
}

void OriginalDialog::select(std::size_t answer) {
    dialTarget_ = answer == 0 ? 0 : kDialFramesPerAnswer;
}

void OriginalDialog::tick() {
    plateY_ = frontEndSpring(plateY_, 0, plateVelocity_, kPlateGain, kPlateDamping, kPlateMaxSpeed);
    boxX_ = frontEndSpring(boxX_, 0, boxVelocity_, kBoxGain, kBoxDamping, kBoxMaxSpeed);
    if (boxFrame_ < kOpenBoxFrame) ++boxFrame_;
    if (dialFrame_ < dialTarget_) ++dialFrame_;
    if (dialFrame_ > dialTarget_) --dialFrame_;
}

void OriginalDialog::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalDialog::settled() const {
    return plateY_ == 0 && plateVelocity_ == 0 && boxX_ == 0 && boxVelocity_ == 0 &&
           boxFrame_ == kOpenBoxFrame && dialFrame_ == dialTarget_;
}

float OriginalDialog::boxOffsetX() const { return static_cast<float>(boxX_ * 640 / 1024); }

float OriginalDialog::plateOffsetY() const { return static_cast<float>(2 * (plateY_ * 480 / 820)); }

std::vector<MenuPiece> OriginalDialog::pieces() const {
    const float plateY = plateOffsetY();
    const float box = boxOffsetX() + kBoxShift;
    std::vector<MenuPiece> result;
    const auto add = [&](std::string picture, float x, float y) -> MenuPiece& {
        MenuPiece piece;
        piece.picture = std::move(picture);
        piece.x = x;
        piece.y = y;
        result.push_back(piece);
        return result.back();
    };

    add("namarm1", kLeftArmX, plateY);
    add("namarm2", kRightArmX, plateY);
    add("name0", kPlateX, plateY + kPlateDrop);
    add("info1.mgl1", box + kTrayX, kTrayY);  // not info1.mgl, another picture
    if (boxFrame_ == kOpenBoxFrame) {
        const float turned = dialFrame_ > 1 ? kTurnedDialShift : 0.0f;
        add(numbered("yesdial%d", dialFrame_ + 1), box + kDialX + turned, kDialY);
    }
    if (boxFrame_ == kOpenBoxFrame) {
        add("yes", box + kBoxX + kOpenBoxShift, kBoxY);
    } else {
        add(numbered("yesno%02d", boxFrame_ + 1), box + kBoxX, kBoxY);
    }

    // The pipe from the tray down to the box: its upper part, its right
    // edge at full height, and its foot.
    MenuPiece& pipe = add("mainbar0", box + kPipeX, kPipeY);
    pipe.sourceWidth = 48;
    pipe.sourceHeight = 160;
    MenuPiece& side = add("mainbar0", box + kPipeSideX, kPipeY);
    side.sourceX = kPipeSideColumn;
    side.sourceWidth = 48 - kPipeSideColumn;
    side.sourceHeight = 176;
    MenuPiece& foot = add("mainbar0", box + kPipeX, kPipeFootY);
    foot.sourceY = 100;
    foot.sourceWidth = 48;
    foot.sourceHeight = 60;

    // The riser on the right: its end, and its first rows stretched up to
    // the top of the screen.
    MenuPiece& riser = add("mainbar1", box + kRiserX, kRiserY);
    riser.sourceWidth = 32;
    riser.sourceHeight = kRiserTopRows;
    MenuPiece& riserTop = add("mainbar1", box + kRiserX, 0.0f);
    riserTop.sourceWidth = 32;
    riserTop.sourceHeight = 2;
    riserTop.width = 32.0f;
    riserTop.height = kRiserY;
    return result;
}

MenuTextLine OriginalDialog::titleLine() const {
    return {kPlateX + kPlateGlassCentre, kTitleTop + plateOffsetY()};
}

MenuTextLine OriginalDialog::answerLine(std::size_t answer) const {
    const float boxX = boxOffsetX() + kBoxShift + kBoxX + kOpenBoxShift;
    return {boxX + (answer == 0 ? kYesWindowCentre : kNoWindowCentre), kBoxY + kWindowTextTop};
}

} // namespace openhogs::ui
