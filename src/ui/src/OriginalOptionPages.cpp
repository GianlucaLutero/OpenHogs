// OriginalOptionPages.cpp - see OriginalOptionPages.h for an overview.

#include "openhogs/ui/OriginalOptionPages.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace openhogs::ui {

namespace {

constexpr float kTextUnitX = 640.0f / 1024.0f;
constexpr float kTextUnitY = 480.0f / 820.0f;

// --- CREDITS (text boxes of FUN_00428cf0, case 16) --------------------------
constexpr double kStartUnits = 1000.0;      // below the screen (table 0x4C0A18)
constexpr double kUnitsPerSecond = 100.0;   // elapsed milliseconds / 10
constexpr double kRowUnits = 80.0;
// A row is drawn while its top is within these (820-high units).
constexpr double kVisibleTop = -51.0, kVisibleBottom = 901.0;
// Columns: x 100 and 500, 500 wide.
constexpr float kNameCentreX = (100.0f + 250.0f) * kTextUnitX;
constexpr float kJobCentreX = (500.0f + 250.0f) * kTextUnitX;

// --- CONTROLS ---------------------------------------------------------------
constexpr float kBoardX = 156.0f, kBoardY = 25.0f;  // 0x9c, 0x19
// The executable's text boxes (66-77: x 280 and 530, y 250 + 85 a row;
// the title at 370, 94) do not fall on the windows of pckey006 as drawn,
// a tilted board: the words go in its windows and on its plate instead
// (decision 99). Middles of the windows, in screen pixels:
constexpr float kActionCentreX = 257.0f, kKeyCentreX = 375.0f;
constexpr float kFirstRowMiddle = 175.0f, kRowStep = 41.5f;
constexpr float kTextHalfHeight = 9.0f;
constexpr float kTitleCentreX = 285.0f, kTitleTop = 103.0f;

} // namespace

std::vector<CreditsRoll::Row> CreditsRoll::visibleRows(std::size_t rowCount) const {
    std::vector<Row> rows;
    if (rowCount == 0) return rows;
    // One lap: from below the screen until the last row has left the top.
    const double lapUnits = kStartUnits + static_cast<double>(rowCount) * kRowUnits - kVisibleTop;
    const double offset = kStartUnits - std::fmod(seconds_ * kUnitsPerSecond, lapUnits);
    for (std::size_t i = 0; i < rowCount; ++i) {
        const double y = offset + kRowUnits * static_cast<double>(i);
        if (y <= kVisibleTop || y >= kVisibleBottom) continue;
        const float top = static_cast<float>(y) * kTextUnitY;
        rows.push_back({i, {kNameCentreX, top}, {kJobCentreX, top}});
    }
    return rows;
}

void OriginalControlsBoard::open() { frame_ = 0; }

void OriginalControlsBoard::tick() {
    if (frame_ < kFacingFrame) ++frame_;
}

void OriginalControlsBoard::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

std::vector<MenuPiece> OriginalControlsBoard::pieces() const {
    char name[16];
    std::snprintf(name, sizeof name, "pckey%03d", frame_);
    MenuPiece board;
    board.picture = name;
    board.x = kBoardX;
    board.y = kBoardY;
    return {board};
}

MenuTextLine OriginalControlsBoard::titleLine() const { return {kTitleCentreX, kTitleTop}; }

MenuTextLine OriginalControlsBoard::actionLine(std::size_t row) const {
    return {kActionCentreX, kFirstRowMiddle + kRowStep * static_cast<float>(row) - kTextHalfHeight};
}

MenuTextLine OriginalControlsBoard::keyLine(std::size_t row) const {
    return {kKeyCentreX, kFirstRowMiddle + kRowStep * static_cast<float>(row) - kTextHalfHeight};
}

} // namespace openhogs::ui
