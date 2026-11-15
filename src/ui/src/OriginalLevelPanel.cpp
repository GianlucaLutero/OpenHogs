// OriginalLevelPanel.cpp - see OriginalLevelPanel.h for an overview.
//
// From the executable's front end (docs/formats/front-end.md): the drawing
// function FUN_0041bef0 (case 20: the panel, its lamps, the lit arrow, the
// plate; case 2 for screen 16: the team holders), the update FUN_00423e70
// (cases 2 and 20) and the text boxes 108-123 of table 0x4C1728.

#include "openhogs/ui/OriginalLevelPanel.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace openhogs::ui {

namespace {

// --- Animation -------------------------------------------------------------
constexpr int kPanelStartY = 800;  // below the screen (table 0x4C0A18)
constexpr int kPlateStartY = -200;  // not read: above the screen
// Both with the executable's spring (gain 8, damping 17, top speed 80).
constexpr int kGain = 8, kDamping = 17, kMaxSpeed = 80;
constexpr int kTextShownY = 10;

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
constexpr float kPanelX = 8.0f, kPanelY = 53.0f;
// The slots' rows: 73 + 46 per slot, the last one 18 pixels lower.
constexpr float kRowY = 73.0f, kRowStep = 46.0f, kLastRowDrop = 18.0f;
// The lamps: columns 8-32 of lit1-3, at both ends of a slot.
constexpr float kLeftLampX = 101.0f, kRightLampX = 288.0f, kLampY = 55.0f;
constexpr int kLampFirstColumn = 8, kLampWidth = 24;
// The lit arrow of the chosen slot (all of lselpc2 but its last column).
constexpr float kArrowX = 303.0f, kArrowY = 57.0f;
constexpr int kArrowWidth = 25;
constexpr float kPlateX = 124.0f, kPlateY = -12.0f;
constexpr float kPlateCentre = 200.0f;  // sqname01 is 400 wide

// Text boxes (1024 x 820 units): slots 108-113, monitor rows 114-123.
constexpr float kTextUnitX = 640.0f / 1024.0f;
constexpr float kTextUnitY = 480.0f / 820.0f;
constexpr float kTitleTop = 65.0f * kTextUnitY;
constexpr float kSlotBoxX = 196.0f, kSlotBoxWidth = 180.0f;
constexpr std::array<float, kLevelPanelSlots> kSlotBoxY{242, 321, 400, 479, 555, 665};
constexpr std::array<float, kLevelPanelMonitorRows> kMonitorBoxY{257, 299, 343, 389, 431, 475, 519, 563, 607, 651};
// The monitor's dark screen spans x 332-540 of lselpc as drawn; its
// boxes (x 570, width 180) are left of its middle: centred on the screen.
constexpr float kMonitorCentreX = 436.0f;

// --- Team holders (screen 16) ----------------------------------------------
constexpr int kHolderOutX = -250;
constexpr int kHolderGain = 6, kHolderDamping = 15, kHolderMaxSpeed = 10;
constexpr float kHolderX = 8.0f, kHolderY = 114.0f, kHolderStep = 78.0f;
// The picture of who plays is centred on the holder's window (x 139, y 38).
constexpr float kPlayerCentreX = 139.0f, kPlayerCentreY = 38.0f;
constexpr float kPlayerWidth = 62.0f, kPlayerHeight = 50.0f;

int toPixelsY(int units) { return units * 480 / 820; }

float rowY(std::size_t slot) {
    return kRowY + kRowStep * static_cast<float>(slot) + (slot == kLevelPanelSlots - 1 ? kLastRowDrop : 0.0f);
}

} // namespace

LevelSlotLayout levelSlotLayout(std::size_t itemCount, std::size_t selected) {
    LevelSlotLayout layout;
    layout.items.fill(-1);
    if (itemCount == 0) return layout;
    const std::size_t scrolled = itemCount - 1;  // all but the last item
    const std::size_t rows = kLevelPanelSlots - 1;
    std::size_t first = 0;
    if (selected < scrolled && selected >= rows) first = selected - rows + 1;
    if (selected >= scrolled && scrolled > rows) first = scrolled - rows;  // on BACK: show the end
    for (std::size_t row = 0; row < rows && first + row < scrolled; ++row) {
        layout.items[row] = static_cast<int>(first + row);
    }
    layout.items[kLevelPanelSlots - 1] = static_cast<int>(itemCount - 1);
    layout.selectedSlot = selected >= scrolled ? kLevelPanelSlots - 1 : selected - first;
    return layout;
}

void OriginalLevelPanel::open(std::size_t slot) {
    panelY_ = kPanelStartY;
    panelVelocity_ = 0;
    plateY_ = kPlateStartY;
    plateVelocity_ = 0;
    select(slot);
}

void OriginalLevelPanel::select(std::size_t slot) {
    if (slot < kLevelPanelSlots) slot_ = slot;
}

void OriginalLevelPanel::tick() {
    panelY_ = frontEndSpring(panelY_, 0, panelVelocity_, kGain, kDamping, kMaxSpeed);
    plateY_ = frontEndSpring(plateY_, 0, plateVelocity_, kGain, kDamping, kMaxSpeed);
}

void OriginalLevelPanel::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalLevelPanel::settled() const {
    return panelY_ == 0 && panelVelocity_ == 0 && plateY_ == 0 && plateVelocity_ == 0;
}

bool OriginalLevelPanel::showsText() const { return std::abs(panelY_) <= kTextShownY; }

// The executable halves the converted drop and doubles it again.
float OriginalLevelPanel::panelOffsetY() const { return static_cast<float>(toPixelsY(panelY_) / 2 * 2); }
float OriginalLevelPanel::plateOffsetY() const { return static_cast<float>(toPixelsY(plateY_)); }

std::vector<MenuPiece> OriginalLevelPanel::pieces() const {
    std::vector<MenuPiece> result;
    const float dy = panelOffsetY();
    for (std::size_t slot = 0; slot < kLevelPanelSlots; ++slot) {
        const char* lamp = slot == slot_ ? "lit3.mgl1" : "lit1.mgl1";
        for (const float x : {kLeftLampX, kRightLampX}) {
            MenuPiece piece;
            piece.picture = lamp;
            piece.sourceX = kLampFirstColumn;
            piece.sourceWidth = kLampWidth;
            piece.x = x;
            piece.y = rowY(slot) + dy + kLampY;
            result.push_back(piece);
        }
    }
    MenuPiece arrow;
    arrow.picture = "lselpc2";
    arrow.sourceWidth = kArrowWidth;
    arrow.x = kArrowX;
    arrow.y = rowY(slot_) + kArrowY + dy;
    result.push_back(arrow);
    MenuPiece panel;
    panel.picture = "lselpc";
    panel.x = kPanelX;
    panel.y = kPanelY + dy;
    result.push_back(panel);
    MenuPiece plate;
    plate.picture = "sqname01";
    plate.x = kPlateX;
    plate.y = kPlateY + plateOffsetY();
    result.push_back(plate);
    return result;
}

MenuTextLine OriginalLevelPanel::titleLine() const { return {kPlateX + kPlateCentre, kTitleTop + plateOffsetY()}; }

MenuTextLine OriginalLevelPanel::slotLine(std::size_t slot) const {
    const float y = kSlotBoxY.at(slot < kLevelPanelSlots ? slot : kLevelPanelSlots - 1);
    return {(kSlotBoxX + kSlotBoxWidth / 2.0f) * kTextUnitX, y * kTextUnitY + panelOffsetY()};
}

MenuTextLine OriginalLevelPanel::monitorLine(std::size_t row) const {
    const float y = kMonitorBoxY.at(row < kLevelPanelMonitorRows ? row : kLevelPanelMonitorRows - 1);
    return {kMonitorCentreX, y * kTextUnitY + panelOffsetY()};
}

// --- OriginalTeamHolders -----------------------------------------------------

void OriginalTeamHolders::open() {
    offsets_.assign(kHolders, kHolderOutX);
    velocities_.assign(kHolders, 0);
}

void OriginalTeamHolders::setPlayers(const std::vector<Player>& players) {
    for (std::size_t i = 0; i < kHolders; ++i) {
        players_[i] = i < players.size() ? players[i] : Player::Off;
        if (players_[i] != Player::Off) shown_[i] = players_[i];
    }
}

void OriginalTeamHolders::tick() {
    for (std::size_t i = 0; i < kHolders; ++i) {
        const int target = players_[i] == Player::Off ? kHolderOutX : 0;
        offsets_[i] = frontEndSpring(offsets_[i], target, velocities_[i], kHolderGain, kHolderDamping, kHolderMaxSpeed);
    }
}

void OriginalTeamHolders::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalTeamHolders::settled() const {
    for (std::size_t i = 0; i < kHolders; ++i) {
        const int target = players_[i] == Player::Off ? kHolderOutX : 0;
        if (offsets_[i] != target || velocities_[i] != 0) return false;
    }
    return true;
}

std::vector<MenuPiece> OriginalTeamHolders::pieces() const {
    std::vector<MenuPiece> result;
    for (std::size_t i = 0; i < kHolders; ++i) {
        if (offsets_[i] <= kHolderOutX) continue;
        const float x = static_cast<float>(offsets_[i]);
        const float y = kHolderY + kHolderStep * static_cast<float>(i);
        if (shown_[i] != Player::Off) {
            MenuPiece player;
            player.picture = shown_[i] == Player::Person ? "pcpad" : "pcchip";
            player.x = x + kHolderX + kPlayerCentreX - kPlayerWidth / 2.0f;
            player.y = y + kPlayerCentreY - kPlayerHeight / 2.0f;
            result.push_back(player);
        }
        MenuPiece holder;
        holder.picture = "com_man";
        holder.x = x + kHolderX;
        holder.y = y;
        result.push_back(holder);
    }
    return result;
}

} // namespace openhogs::ui
