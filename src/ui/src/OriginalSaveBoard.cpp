// OriginalSaveBoard.cpp - see OriginalSaveBoard.h for an overview.
//
// From the executable's front end (docs/formats/front-end.md): the drawing
// function FUN_0041bef0 (case 8: the board, the lamps, the panel), the
// update FUN_00423e70 (case 8: the rise and the panel), the selection
// FUN_00427c90 (case 8: the lamps) and FUN_0042e310 (the panel's contents).

#include "openhogs/ui/OriginalSaveBoard.h"

#include <array>
#include <cstdlib>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
// The board rises from 800 units below to 1 (spring 8, 20, 20).
constexpr int kStartDrop = 800, kRestDrop = 1;
constexpr int kDropGain = 8, kDropDamping = 20, kDropMaxSpeed = 20;
constexpr int kOpenPanelFrame = 4;
// The chosen lamp: half lit and lit, 5 ticks each (animation 0x3EE).
constexpr int kBlinkTicks = 5;
constexpr int kLitLamp = 2;
constexpr int kShowTextDrop = 10;  // units from the rest

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kBoardX = 0.0f, kBoardY = 43.0f;  // + the drop
constexpr float kPanelX = 474.0f, kPanelY = 73.0f;
constexpr float kLampX = 385.0f, kLampY = 104.0f, kLampStep = 33.5f;
// The slots' text boxes (table 0x4C1728, type 8): x 200, width 270 of 1024
// and y 193 ... 590 of 820, written centred.
constexpr float kSlotCentreX = (200.0f + 135.0f) * 640.0f / 1024.0f;
constexpr std::array<float, kSaveBoardSlots> kSlotTop{193.0f, 247.0f, 302.0f, 363.0f, 420.0f, 475.0f, 529.0f, 590.0f};
constexpr float kSlotTextLift = 2.0f;  // the box is 30 units tall; the letters sit in its middle
// The panel's contents (FUN_0042e310): a row per hog from y 110, 29 apart;
// the pips at x 527, the career badge at 513 (499 beside pips).
constexpr float kBadgeRowY = 110.0f, kBadgeRowStep = 29.0f;
constexpr float kPipsX = 527.0f, kBadgeX = 513.0f, kBadgeBesidePipsX = 499.0f;
constexpr std::size_t kBadgeRows = 8;
constexpr float kMissionX = 801.0f * 640.0f / 1024.0f, kMissionY = 600.0f * 480.0f / 820.0f;

// The badges of the executable's ranks (table 0x4D29C0, the ranks
// numbered as their names in gtext from 63): the career's picture and the
// number of stripes (`strp1`, `strp2`) beside it.
struct RankBadge {
    int career;  // index into kCareerPictures, -1 for a grunt
    int pips;
};
constexpr std::array<const char*, 6> kCareerPictures{"pcHweap", "snipr", "sappr", "pcmedic", "cmndo", "pcmedal"};
constexpr std::array<RankBadge, 17> kRankBadges{{
    {-1, 0},                    // grunt
    {0, 0}, {0, 1}, {0, 2},     // gunner, bombardier, pyrotechnic
    {4, 0},                     // commando
    {2, 0}, {2, 1}, {2, 2},     // sapper, engineer, saboteur
    {1, 0}, {1, 1}, {1, 2},     // scout, sniper, spy
    {3, 0}, {3, 1}, {3, 2},     // orderly, medic, surgeon
    {5, 0}, {5, 0}, {5, 0},     // hero, ace, legend: the medal alone
}};
constexpr std::size_t kFirstRankEntry = 63;

int toPixelsY(int units) { return units * 480 / 820; }

} // namespace

void OriginalSaveBoard::open(std::size_t selected) {
    drop_ = kStartDrop;
    dropVelocity_ = 0;
    panelFrame_ = 0;
    lamps_.fill(kLitLamp);
    lampTicks_.fill(0);
    select(selected);
}

void OriginalSaveBoard::select(std::size_t selected) {
    selected_ = selected;
    lampTicks_.fill(0);
}

void OriginalSaveBoard::tick() {
    drop_ = frontEndSpring(drop_, kRestDrop, dropVelocity_, kDropGain, kDropDamping, kDropMaxSpeed);
    if (panelFrame_ < kOpenPanelFrame) ++panelFrame_;
    for (std::size_t i = 0; i < kSaveBoardSlots; ++i) {
        if (i == selected_) {
            // Lit and half lit in turn.
            lamps_[i] = (lampTicks_[i] / kBlinkTicks) % 2 == 0 ? 1 : kLitLamp;
            ++lampTicks_[i];
        } else if (lamps_[i] > 0) {
            --lamps_[i];  // a step a tick towards dark
        }
    }
}

void OriginalSaveBoard::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalSaveBoard::settled() const { return drop_ == kRestDrop && dropVelocity_ == 0 && panelFrame_ == kOpenPanelFrame; }

bool OriginalSaveBoard::showsText() const { return std::abs(drop_ - kRestDrop) <= kShowTextDrop; }

bool OriginalSaveBoard::showsInfo() const { return panelFrame_ == kOpenPanelFrame && showsText(); }

MenuTextLine OriginalSaveBoard::slotLine(std::size_t slot) const {
    return {kSlotCentreX, static_cast<float>(toPixelsY(static_cast<int>(kSlotTop.at(slot)))) + kSlotTextLift};
}

MenuTextLine OriginalSaveBoard::missionLine() const { return {kMissionX, kMissionY}; }

std::vector<MenuPiece> OriginalSaveBoard::pieces(const std::vector<game::RankId>& squad) const {
    std::vector<MenuPiece> result;
    const auto add = [&](std::string picture, float x, float y) {
        MenuPiece piece;
        piece.picture = std::move(picture);
        piece.x = x;
        piece.y = y;
        result.push_back(piece);
    };
    // The panel's contents, over everything.
    if (showsInfo()) {
        for (std::size_t row = 0; row < squad.size() && row < kBadgeRows; ++row) {
            const std::size_t number = game::rankSpec(squad[row]).textEntry - kFirstRankEntry;
            if (number >= kRankBadges.size()) continue;
            const RankBadge badge = kRankBadges[number];
            const float y = kBadgeRowY + kBadgeRowStep * static_cast<float>(row);
            if (badge.pips > 0) add(badge.pips == 1 ? "strp1" : "strp2", kPipsX, y);
            if (badge.career >= 0) {
                add(kCareerPictures[static_cast<std::size_t>(badge.career)], badge.pips > 0 ? kBadgeBesidePipsX : kBadgeX, y);
            }
        }
    }
    const float drop = static_cast<float>(toPixelsY(drop_));
    for (std::size_t i = 0; i < kSaveBoardSlots; ++i) {
        add("pclit" + std::to_string(lamps_[i]), kLampX, drop + kLampY + kLampStep * static_cast<float>(i));
    }
    add("pcsav", kBoardX, drop + kBoardY);
    add("pcsvinf" + std::to_string(panelFrame_), kPanelX, drop + kPanelY);
    return result;
}

} // namespace openhogs::ui
