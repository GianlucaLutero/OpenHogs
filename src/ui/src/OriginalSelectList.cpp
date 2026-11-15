// OriginalSelectList.cpp - see OriginalSelectList.h for an overview.
//
// From the executable's front end (docs/formats/front-end.md): the drawing
// function FUN_0041bef0 (case 2: the monitor, its cogs and the plate), the
// foreground FUN_0041e780 (case 2: the box and the selector), the update
// FUN_00423e70 (case 2) and the text boxes of type 2 (table 0x4C1728).

#include "openhogs/ui/OriginalSelectList.h"

#include "openhogs/ui/OriginalDialog.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
constexpr int kListStartY = 546;   // below the screen (table 0x4C0A18)
constexpr int kListGain = 10, kListDamping = 15, kListMaxSpeed = 15;
constexpr int kPlateStartY = -150;
constexpr int kPlateGain = 3, kPlateDamping = 30, kPlateMaxSpeed = 6;
constexpr int kBoxStartX = 0x4C8;
constexpr int kBoxGain = 13, kBoxDamping = 30, kBoxMaxSpeed = 30;
constexpr int kSelectorStepsPerEntry = 5;  // the selector moves one step a tick
constexpr int kKnobFrames = 6;
constexpr int kTextShownY = 10;  // the words appear once the monitor is this close

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kBoxShift = 7.0f;
constexpr float kMonitorX = 385.0f, kMonitorY = 160.0f;
constexpr int kStemTop = 290, kStemRows = 4;  // the stem's rows, repeated below
constexpr int kStemRepeats = 25;
constexpr float kStemY = 450.0f, kStemStep = 4.0f;
constexpr float kCogX = 553.0f, kCogY = 180.0f;
constexpr float kLampX = 537.0f, kLampY = 202.0f;
constexpr float kKnobX = 348.0f, kKnobY = 170.0f;
constexpr float kBarX = 412.0f, kBarY = 186.0f;
// The bar is drawn in two parts, columns 0-40 and from column 30 drawn 20
// pixels further left; with the stretch width at 0 (decision 80) the first
// part covers the overlap, so columns 40-60 are not seen.
constexpr int kBarLeftColumns = 40, kBarRightColumn = 60;

// Text boxes in the executable's 1024 x 820 text units: the title's box is
// that of type 2 (y 65), the entries' boxes 5-12 of table 0x4C1728.
constexpr float kTextUnitX = 640.0f / 1024.0f;
constexpr float kTextUnitY = 480.0f / 820.0f;
constexpr float kPlateCentreX = 123.0f + 162.0f;  // the plate's glass (decision 92)
constexpr float kTitleTop = 65.0f * kTextUnitY;
constexpr float kEntryBoxX = 687.0f, kEntryBoxWidth = 180.0f;
constexpr float kEntryBoxY[kOriginalSelectListEntries]{369, 407, 447, 489, 527, 567, 607, 647};

std::string numbered(const char* format, int number) {
    char name[32];
    std::snprintf(name, sizeof name, format, number);
    return name;
}

// The executable's conversion of 820-high units to screen pixels.
int toPixelsY(int units) { return units * 480 / 820; }

} // namespace

void OriginalSelectList::open(std::size_t selected) {
    listY_ = kListStartY;
    listVelocity_ = 0;
    plateY_ = kPlateStartY;
    plateVelocity_ = 0;
    boxX_ = kBoxStartX;
    boxVelocity_ = 0;
    select(selected);
    selector_ = selectorTarget_;
}

void OriginalSelectList::select(std::size_t entry) {
    if (entry < kOriginalSelectListEntries) {
        selectorTarget_ = static_cast<int>(entry) * kSelectorStepsPerEntry;
    }
}

void OriginalSelectList::tick() {
    plateY_ = frontEndSpring(plateY_, 0, plateVelocity_, kPlateGain, kPlateDamping, kPlateMaxSpeed);
    boxX_ = frontEndSpring(boxX_, 0, boxVelocity_, kBoxGain, kBoxDamping, kBoxMaxSpeed);
    listY_ = frontEndSpring(listY_, 0, listVelocity_, kListGain, kListDamping, kListMaxSpeed);
    if (selector_ < selectorTarget_) ++selector_;
    if (selector_ > selectorTarget_) --selector_;
}

void OriginalSelectList::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalSelectList::settled() const {
    return listY_ == 0 && listVelocity_ == 0 && plateY_ == 0 && plateVelocity_ == 0 && boxX_ == 0 &&
           boxVelocity_ == 0 && selector_ == selectorTarget_;
}

bool OriginalSelectList::showsText() const { return std::abs(listY_) <= kTextShownY; }

float OriginalSelectList::listOffsetY() const { return static_cast<float>(toPixelsY(listY_)); }
float OriginalSelectList::plateOffsetY() const { return static_cast<float>(toPixelsY(plateY_)); }
float OriginalSelectList::boxOffsetX() const { return static_cast<float>(boxX_ * 640 / 1024); }

std::vector<MenuPiece> OriginalSelectList::pieces() const {
    std::vector<MenuPiece> result;
    const auto add = [&](std::string picture, float x, float y) -> MenuPiece& {
        MenuPiece piece;
        piece.picture = std::move(picture);
        piece.x = x;
        piece.y = y;
        result.push_back(piece);
        return result.back();
    };
    const float box = boxOffsetX() + kBoxShift;
    const float list = listOffsetY();
    // The monitor moves by half its offset, twice (whole pixels, as the
    // executable); the selector by its offset plus the selector's, twice.
    const float monitor = static_cast<float>(toPixelsY(listY_) / 2 * 2);
    const float selector = 2.0f * (static_cast<float>(toPixelsY(4 * selector_)) + list);

    // The foreground: the box, then the selector.
    addTray(result, box);
    addBox(result, box, 0);
    add(selector_ == selectorTarget_ ? "lit3.mgl1" : "lit1.mgl1", kLampX, kLampY + selector);
    add(numbered("selec%02d", selector_ % kKnobFrames), kKnobX, kKnobY + selector);
    MenuPiece& barLeft = add("select", kBarX, kBarY + selector);
    barLeft.sourceWidth = kBarLeftColumns;
    MenuPiece& barRight = add("select", kBarX + kBarLeftColumns, kBarY + selector);
    barRight.sourceX = kBarRightColumn;
    barRight.sourceWidth = 176 - kBarRightColumn;

    // The background: the stem, the cogs, the monitor, the plate.
    for (int i = 0; i < kStemRepeats; ++i) {
        MenuPiece& stem = add("counsele", kMonitorX, kStemY + monitor + kStemStep * static_cast<float>(i));
        stem.sourceY = kStemTop;
        stem.sourceHeight = kStemRows;
    }
    // The cogs turn as the monitor moves (FUN_0041fc30).
    const int cog = (((6 - listY_) * 2) / 25 + 600) % 6;
    add(numbered("selcog%d", cog + 1), kCogX, kCogY + monitor);
    add("counsele", kMonitorX, kMonitorY + monitor);
    addNamePlate(result, plateOffsetY());
    return result;
}

MenuTextLine OriginalSelectList::titleLine() const { return {kPlateCentreX, kTitleTop + plateOffsetY()}; }

MenuTextLine OriginalSelectList::entryLine(std::size_t entry) const {
    const float y = kEntryBoxY[entry < kOriginalSelectListEntries ? entry : kOriginalSelectListEntries - 1];
    return {(kEntryBoxX + kEntryBoxWidth / 2.0f) * kTextUnitX, y * kTextUnitY + listOffsetY()};
}

} // namespace openhogs::ui
