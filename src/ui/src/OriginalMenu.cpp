// OriginalMenu.cpp - see OriginalMenu.h for an overview.
//
// Every number below comes from the executable's front end (see
// docs/formats/front-end.md): the drawing function FUN_0041bef0 (case 1,
// the list menu), its text boxes (tables at 0x4C1548 and 0x4C1728), its
// update function FUN_00423e70 and the element ticker FUN_0041fec0.

#include "openhogs/ui/OriginalMenu.h"

#include <cstdlib>
#include <cstdio>

namespace openhogs::ui {

namespace {

// --- The opening animation -------------------------------------------------
constexpr int kPanelStartY = -650;    // 820-high units above its place (table 0x4C0A18)
constexpr int kRackStartOffset = -36;  // pixels (set on opening)
constexpr int kPlateStartFrame = 2;    // the plates start turned away...
constexpr int kPlateRestFrame = 6;     // ...and flip until frame 6 (= picture 1)
constexpr int kPlateFlipStartY = -50;  // once the panel is this close to its place

// The springs' settings (gain, damping, top speed), as the executable.
constexpr int kRackGain = 4, kRackDamping = 12, kRackMaxSpeed = 21;
constexpr int kPanelGain = 8, kPanelDamping = 17, kPanelMaxSpeed = 80;

// --- The lamps -------------------------------------------------------------
// The selected entry's lamp plays this sequence in a loop (table 0x4C0A78
// from pair 4): half lit, lit for ten ticks, half lit, off for five.
struct BlinkStep {
    int frame;
    int ticks;
};
constexpr std::array<BlinkStep, 4> kBlink{{{1, 1}, {2, 10}, {1, 1}, {0, 5}}};

// --- Layout (pixels of the 640 x 480 screen) -------------------------------
// The executable draws the sprites 10 pixels left of its table positions.
constexpr int kSpriteShiftX = -10;

constexpr float kPanelX = 60.0f;                   // fullmenu
constexpr int kPanelTopStripWidth = 340;           // its top is drawn again above it
constexpr int kPanelTopStripHeight = 50;
constexpr float kPanelTopStripY = -48.0f;
constexpr float kTitlePlateX = 296.0f, kTitlePlateY = 112.0f;
constexpr float kBarX = 319.0f, kBarY = 182.0f, kBarSpacing = 40.0f;
constexpr float kInnerBarShift = 12.0f;            // the two middle bars stand out
constexpr float kLowerBarsDrop = 2.0f;             // the two lower bars sit lower
constexpr float kLampX = 478.0f, kLampY = 176.0f;
constexpr int kLampHeight = 38;                    // each lamp is a 40-pixel band of light1-3
constexpr float kDialX = 140.0f, kDialY = 192.0f;
constexpr float kCogX = 44.0f, kCogY = 192.0f;
constexpr float kTallCogX = 524.0f, kTallCogY = 160.0f;
// The racks: the picture minus its first column, stretched to 64 x 638.
constexpr float kRackWidth = 64.0f, kRackHeight = 638.0f;
constexpr float kLeftRackX = 1.0f, kLeftRackY = -272.0f;
constexpr float kRightRackX = 666.0f, kRightRackY = -222.0f;  // mirrored: ends here

// Text boxes, in the executable's 1024 x 820 text units.
constexpr float kTextUnitX = 640.0f / 1024.0f;
constexpr float kTextUnitY = 480.0f / 820.0f;
constexpr float kTitleBoxX = 518.0f, kTitleBoxWidth = 210.0f, kTitleBoxY = 227.0f;
constexpr float kEntryBoxX = 528.0f, kEntryBoxWidth = 205.0f;
constexpr std::array<float, kOriginalMenuEntries> kEntryBoxY{329.0f, 398.0f, 468.0f, 537.0f};

// "name" + a number, as the picture names of FEBMP.MAD.
std::string numbered(const char* format, int number) {
    char name[32];
    std::snprintf(name, sizeof name, format, number);
    return name;
}

} // namespace

int frontEndSpring(int value, int target, int& velocity, int gain, int damping, int maxSpeed) {
    if (std::abs(velocity) > maxSpeed) velocity = velocity < 1 ? -maxSpeed : maxSpeed;
    velocity = ((((target - value) * gain) / 10 + velocity) * 10 / damping) / 2;
    const int step = velocity * 2;
    if (value == target) {
        // Overshooting: it swings back one unit past the target first.
        if (std::abs(step) > 2) return velocity > 0 ? target - 1 : target + 1;
        velocity = 0;
        return value;
    }
    // Always at least one unit, so it gets there.
    if (step != 0) return value + step;
    return value + (target > value ? 1 : -1);
}

void OriginalMenu::open(std::size_t selected) {
    panelY_ = kPanelStartY;
    panelVelocity_ = 0;
    rackOffset_ = kRackStartOffset;
    rackVelocity_ = 0;
    plateFrame_ = kPlateStartFrame;
    dialFrame_ = 0;
    lamps_ = {};
    select(selected);
}

void OriginalMenu::replaceContents(std::size_t selected) {
    plateFrame_ = kPlateStartFrame;
    select(selected);
}

void OriginalMenu::select(std::size_t entry) {
    if (entry >= kOriginalMenuEntries) return;
    // Four positions of the twelve dial pictures; the last ones are one
    // frame closer together (the executable's sel * 4, minus 1 past 4).
    const int frame = static_cast<int>(entry) * 4;
    dialTarget_ = frame > 4 ? frame - 1 : frame;
    for (std::size_t i = 0; i < lamps_.size(); ++i) {
        Lamp& lamp = lamps_[i];
        if (i == entry) {
            lamp.blinking = true;
            lamp.step = 0;
            lamp.frame = kBlink[0].frame;
            lamp.wait = kBlink[0].ticks;
        } else {
            lamp.blinking = false;  // fades to off
        }
    }
}

void OriginalMenu::tickLamp(Lamp& lamp) const {
    if (lamp.blinking) {
        if (--lamp.wait > 0) return;
        lamp.step = (lamp.step + 1) % kBlink.size();
        lamp.frame = kBlink[lamp.step].frame;
        lamp.wait = kBlink[lamp.step].ticks;
    } else if (lamp.frame > 0) {
        --lamp.frame;
    }
}

void OriginalMenu::tick() {
    // First the racks slide in, then the panel drops; the plates flip once
    // the panel is nearly in place.
    if (rackOffset_ != 0) {
        rackOffset_ = frontEndSpring(rackOffset_, 0, rackVelocity_, kRackGain, kRackDamping, kRackMaxSpeed);
    } else {
        panelY_ = frontEndSpring(panelY_, 0, panelVelocity_, kPanelGain, kPanelDamping, kPanelMaxSpeed);
        if (plateFrame_ < kPlateRestFrame && panelY_ > kPlateFlipStartY) ++plateFrame_;
    }
    if (dialFrame_ < dialTarget_) ++dialFrame_;
    if (dialFrame_ > dialTarget_) --dialFrame_;
    for (Lamp& lamp : lamps_) tickLamp(lamp);
}

void OriginalMenu::advance(double seconds) {
    pending_ += seconds * kFrontEndTicksPerSecond;
    while (pending_ >= 1.0) {
        tick();
        pending_ -= 1.0;
    }
}

bool OriginalMenu::settled() const {
    return rackOffset_ == 0 && rackVelocity_ == 0 && panelY_ == 0 && panelVelocity_ == 0 &&
           plateFrame_ == kPlateRestFrame && dialFrame_ == dialTarget_;
}

int OriginalMenu::offsetX() const { return kSpriteShiftX; }

int OriginalMenu::offsetY() const {
    // The executable converts to screen pixels and halves, then doubles:
    // the panel moves in steps of two pixels.
    const int pixels = panelY_ * 480 / 820;
    return pixels / 2 * 2;
}

std::vector<MenuPiece> OriginalMenu::pieces() const {
    const float ox = static_cast<float>(offsetX());
    const float oy = static_cast<float>(offsetY());
    const int plate = plateFrame_ % 6 + 1;
    std::vector<MenuPiece> result;

    for (std::size_t i = 0; i < kOriginalMenuEntries; ++i) {
        MenuPiece lamp;
        lamp.picture = numbered("light%d", lamps_[i].frame + 1);
        lamp.sourceY = static_cast<int>(i) * 40;
        lamp.sourceWidth = 48;
        lamp.sourceHeight = kLampHeight;
        lamp.x = ox + kLampX;
        lamp.y = oy + kLampY + kBarSpacing * static_cast<float>(i);
        result.push_back(lamp);
    }
    for (std::size_t i = 0; i < kOriginalMenuEntries; ++i) {
        MenuPiece bar;
        bar.picture = numbered("chose%d", plate);
        bar.x = ox + kBarX + ((i == 1 || i == 2) ? kInnerBarShift : 0.0f);
        bar.y = oy + kBarY + kBarSpacing * static_cast<float>(i) + (i >= 2 ? kLowerBarsDrop : 0.0f);
        result.push_back(bar);
    }

    MenuPiece dial;
    dial.picture = numbered("dial%04d", dialFrame_ + 1);
    dial.x = ox + kDialX;
    dial.y = oy + kDialY;
    result.push_back(dial);

    MenuPiece cog;
    cog.picture = "cog0";
    cog.x = ox + kCogX;
    cog.y = oy + kCogY;
    result.push_back(cog);

    MenuPiece tallCog;
    tallCog.picture = "cogB00";
    tallCog.x = ox + kTallCogX;
    tallCog.y = oy + kTallCogY;
    result.push_back(tallCog);

    // The racks do not drop with the panel; they slide in from the sides.
    const float rack = static_cast<float>(rackOffset_);
    MenuPiece leftRack;
    leftRack.picture = "track";
    leftRack.sourceX = 1;
    leftRack.sourceWidth = 63;
    leftRack.sourceHeight = 480;
    leftRack.x = ox + kLeftRackX + rack;
    leftRack.y = kLeftRackY;
    leftRack.width = kRackWidth;
    leftRack.height = kRackHeight;
    result.push_back(leftRack);
    MenuPiece rightRack = leftRack;
    rightRack.x = ox + kRightRackX - rack;
    rightRack.y = kRightRackY;
    rightRack.mirrored = true;
    result.push_back(rightRack);

    MenuPiece title;
    title.picture = numbered("title%d", plate);
    title.x = ox + kTitlePlateX;
    title.y = oy + kTitlePlateY;
    result.push_back(title);

    // The panel's top is drawn a second time above it, so no gap opens
    // while it bounces.
    MenuPiece topStrip;
    topStrip.picture = "fullmenu";
    topStrip.sourceWidth = kPanelTopStripWidth;
    topStrip.sourceHeight = kPanelTopStripHeight;
    topStrip.x = ox + kPanelX;
    topStrip.y = oy + kPanelTopStripY;
    result.push_back(topStrip);
    MenuPiece panel;
    panel.picture = "fullmenu";
    panel.x = ox + kPanelX;
    panel.y = oy;
    result.push_back(panel);
    return result;
}

MenuTextLine OriginalMenu::titleLine() const {
    return {(kTitleBoxX + kTitleBoxWidth / 2.0f) * kTextUnitX,
            kTitleBoxY * kTextUnitY + static_cast<float>(offsetY())};
}

MenuTextLine OriginalMenu::entryLine(std::size_t entry) const {
    return {(kEntryBoxX + kEntryBoxWidth / 2.0f) * kTextUnitX,
            kEntryBoxY.at(entry) * kTextUnitY + static_cast<float>(offsetY())};
}

} // namespace openhogs::ui
