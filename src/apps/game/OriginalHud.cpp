// OriginalHud.cpp - see OriginalHud.h for an overview.

#include "OriginalHud.h"

#include "openhogs/formats/BitmapImage.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TimImage.h"
#include "openhogs/ui/RifleSight.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// The instruments, and the damage digits with the explosion pictures.
const char* const kDashboardPictures = "Language/Tims/dashtims.mad";
const char* const kEffectPictures = "Language/Tims/expltims.mad";
// The weapon menu frame and the rifle sight's crosshair.
const char* const kFacePictures = "Language/Tims/facetims.mad";
// The radar's marks, and the text box's tubes.
const char* const kRadarPictures = "Language/Tims/mapicons.mtd";
const char* const kTextBoxPictures = "Language/Tims/tboxtims.mad";
const glm::vec4 kSightBlack{0.0f, 0.0f, 0.0f, 1.0f};

// The labels over the hogs: each of their two lines (name, then heart and
// health) is one row of FEText/CHARS2, 16 pixels, inside the executable's
// 48-pixel box. The heart is expltims' white `heart`, tinted pink as in
// the footage.
constexpr float kLabelLineHeight = 16.0f;
constexpr float kLabelLineGap = 1.0f;
constexpr float kHeartGap = 4.0f;  // between the heart and the number
constexpr glm::vec4 kHeartPink{1.0f, 0.3f, 0.6f, 1.0f};

// The weapon menu (FUN_00459550). MENUTIMS.MAD holds the panel (entry 0,
// `menu.bmp`, 257 x 274) and the selection frame (`SELECT.BMP`, the last
// entry); smlfont.bmp has the digits of the uses left (6 x 10 each) and
// halftone.bmp greys out a weapon with none left (the executable asks for
// smlhalft.bmp, which the PC folder does not have).
const char* const kMenuPictures = "Language/Tims/MENUTIMS.MAD";
const char* const kMenuDigits = "Language/Tims/smlfont.bmp";
const char* const kMenuHalftone = "Language/Tims/halftone.bmp";
// Where the panel sits on the 640 x 480 screen when fully in (right edge,
// between the angle gauge and the clock), and how far it slides out.
constexpr float kMenuRightMargin = 12.0f;
constexpr float kMenuTop = 100.0f;
constexpr float kMenuSlideDistance = 309.0f;  // the executable: 0x135
// On the panel: the windows (from the picture's bolts), the icons in them,
// the frame around the chosen one (the executable: icon - (15, 11)), the
// tabs along the top and the bar at the bottom for the weapon's name.
constexpr float kMenuFirstWindowX = 8.0f, kMenuFirstWindowY = 50.0f;
constexpr float kMenuWindowWidth = 72.0f, kMenuWindowHeight = 46.0f;
constexpr float kMenuIconX = 8.0f, kMenuIconY = 6.0f;  // in its window
constexpr float kMenuIconWidth = 56.0f, kMenuIconHeight = 34.0f;
constexpr float kMenuFrameX = -15.0f, kMenuFrameY = -11.0f;  // from the icon
constexpr float kMenuTabX = 6.0f, kMenuTabWidth = 58.0f, kMenuTabHeight = 30.0f;
constexpr float kMenuNameCentreX = 118.0f, kMenuNameTop = 246.0f;
constexpr float kMenuDigitWidth = 6.0f, kMenuDigitHeight = 10.0f;
constexpr int kMenuMostUsesShown = 9;  // the executable shows 9 for more
const glm::vec4 kMenuTabHighlight{1.0f, 0.9f, 0.5f, 0.3f};
const glm::vec4 kMenuNameColour{1.0f, 0.88f, 0.12f, 1.0f};

// The radar's marks (FUN_10009810): 10 x 11 pixels, drawn 5 pixels left
// of and above the point they mark.
constexpr float kRadarMarkWidth = 10.0f, kRadarMarkHeight = 11.0f, kRadarMarkOffset = 5.0f;

// The text box (FUN_0045e6a0), from the middle of the 640-pixel screen:
// the tube's ends (x0, x1) and the window the text shows through.
constexpr float kBoxLeft = -120.0f, kBoxRight = 122.0f;
constexpr float kBoxWindowLeft = -102.0f;
// The small tube: npro4 ends (32 wide) at x0 - 14 (turned round) and
// x1 - 18, four npro3 pieces (64 wide) 48 apart from x0 + 18; it comes
// down from y = -50 to -5, the text 33 lower in a window 22 tall.
constexpr float kSmallEndLeft = -14.0f, kSmallEndRight = -18.0f, kSmallPieceStart = 18.0f, kSmallPieceStep = 48.0f;
constexpr float kSmallTravel = 45.0f, kSmallHidden = -50.0f, kSmallTextOffset = 33.0f, kTextWindowHeight = 22.0f;
// The big tube: npro2 ends (64 wide) at x0 - 64 (turned round) and x1,
// four npro1 pieces 64 apart from x0; it comes down from y = -110 to -5;
// the strength bar (240 wide, two rows of 8) 35 lower, from x0 + 1.
constexpr float kBigEndLeft = -64.0f, kBigPieceStep = 64.0f;
constexpr float kBigTravel = 105.0f, kBigHidden = -110.0f, kBarOffset = 35.0f;
constexpr float kBarLeft = 1.0f, kBarWidth = 240.0f, kBarRowHeight = 8.0f;
constexpr int kTubePieces = 4;
// The bar's second row is the team's colour less 70 in each channel.
constexpr float kBarShade = 70.0f / 255.0f;
// The text: the game's BIG font (the executable's macfont.bmp is not in
// the PC folder) at this height.
constexpr float kBoxTextHeight = 16.0f;

// TIM entry names end in ".tim"; the layout uses the bare names.
std::string bareName(std::string_view entryName) {
    std::string name(entryName);
    if (const auto dot = name.rfind('.'); dot != std::string::npos) {
        name.erase(dot);
    }
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name;
}

} // namespace

OriginalHud::OriginalHud(const core::VirtualFileSystem& vfs) {
    for (const char* path : {kDashboardPictures, kEffectPictures, kFacePictures, kRadarPictures, kTextBoxPictures}) {
        try {
            const auto archive = formats::PackageArchive::parse(vfs.readFile(path));
            for (const auto& entry : archive.entries()) {
                pictures_.emplace(bareName(entry.name), render::UiImage(formats::decodeTim(archive.entryData(entry))));
            }
        } catch (const std::exception& error) {
            std::cerr << "Warning: no HUD pictures from " << path << " (" << error.what() << ")\n";
        }
    }
    try {
        const auto archive = formats::PackageArchive::parse(vfs.readFile(kMenuPictures));
        const auto& entries = archive.entries();
        if (!entries.empty()) {
            menuPanel_.emplace(formats::decodeBitmap(archive.entryData(entries.front()), formats::BitmapTransparency::Magenta));
            // The frame's inside is black, and must let the icon show.
            auto frame = formats::decodeBitmap(archive.entryData(entries.back()));
            for (std::size_t i = 0; i + 3 < frame.pixels.size(); i += 4) {
                if (frame.pixels[i] == 0 && frame.pixels[i + 1] == 0 && frame.pixels[i + 2] == 0) frame.pixels[i + 3] = 0;
            }
            menuFrame_.emplace(frame);
        }
        menuDigits_.emplace(formats::decodeBitmap(vfs.readFile(kMenuDigits), formats::BitmapTransparency::PaletteIndex0));
        menuHalftone_.emplace(formats::decodeBitmap(vfs.readFile(kMenuHalftone), formats::BitmapTransparency::PaletteIndex0));
    } catch (const std::exception& error) {
        std::cerr << "Warning: no weapon menu pictures (" << error.what() << ")\n";
    }
}

void OriginalHud::drawWeaponMenu(Interface& userInterface, const ui::WeaponMenu& menu) const {
    if (!weaponMenuSlide_.visible() || !menuPanel_) return;
    auto& ui = userInterface.renderer();
    const float slide = weaponMenuSlide_.offset() / ui::HudSlide::kHiddenOffset * kMenuSlideDistance;
    const float left = ui.canvasWidth() - kMenuRightMargin - menuPanel_->width() + slide;
    ui.drawImage(*menuPanel_, left, kMenuTop, menuPanel_->width(), menuPanel_->height());

    // The open tab lights up.
    ui.drawRect(left + kMenuTabX + static_cast<float>(menu.tab()) * kMenuTabWidth, kMenuTop, kMenuTabWidth,
                kMenuTabHeight, kMenuTabHighlight);

    for (std::size_t slot = 0; slot < ui::kWeaponMenuSlots; ++slot) {
        const float iconX = left + kMenuFirstWindowX + static_cast<float>(slot % ui::kWeaponMenuColumns) * kMenuWindowWidth + kMenuIconX;
        const float iconY = kMenuTop + kMenuFirstWindowY + static_cast<float>(slot / ui::kWeaponMenuColumns) * kMenuWindowHeight + kMenuIconY;
        const auto& entry = menu.slots()[slot];
        if (entry) {
            if (const auto* icon = userInterface.weaponIcon(entry->weapon)) {
                ui.drawImage(*icon, iconX, iconY, kMenuIconWidth, kMenuIconHeight);
            }
            if (!entry->usable() && menuHalftone_) {
                ui.drawImage(*menuHalftone_, iconX, iconY, kMenuIconWidth, kMenuIconHeight);
            }
            if (entry->uses && menuDigits_) {
                // The uses left in the icon's top-left corner, as the executable writes them.
                const int digit = std::min(*entry->uses, kMenuMostUsesShown);
                const float u0 = static_cast<float>(digit) * kMenuDigitWidth / menuDigits_->width();
                const float u1 = static_cast<float>(digit + 1) * kMenuDigitWidth / menuDigits_->width();
                ui.drawImageRegion(*menuDigits_, iconX, iconY, kMenuDigitWidth, kMenuDigitHeight, u0, 0.0f, u1,
                                   kMenuDigitHeight / menuDigits_->height());
            }
        }
        if (slot == menu.slot() && menuFrame_) {
            ui.drawImage(*menuFrame_, iconX + kMenuFrameX, iconY + kMenuFrameY, menuFrame_->width(), menuFrame_->height());
        }
    }

    // The chosen weapon's name in the bar at the bottom.
    if (const auto& current = menu.current()) {
        render::TextStyle style;
        style.align = render::TextAlign::Centre;
        style.colour = kMenuNameColour;
        ui.drawText(userInterface.smallFont(), ui::weaponDisplayName(current->weapon, userInterface.gameText()),
                    left + kMenuNameCentreX, kMenuTop + kMenuNameTop, style);
    }
}

void OriginalHud::setRadarMap(const formats::RgbaImage& picture) {
    radarMap_.emplace(picture);
}

void OriginalHud::drawRadar(render::UiRenderer& ui, const std::vector<ui::RadarMark>& marks,
                            const glm::vec3& cameraForward) const {
    if (!radarSlide_.visible() || !radarMap_) return;
    const ui::RadarView view(cameraForward, ui::kRadarCentre + glm::vec2(0.0f, radarSlide_.offset()));
    ui.drawImageQuad(*radarMap_, view.pictureCorners());
    for (const auto& mark : marks) {
        const char* name = "iconpig";
        switch (mark.kind) {
        case ui::RadarMarkKind::Hog: name = "iconpig"; break;
        case ui::RadarMarkKind::WeaponCrate: name = "iconpkup"; break;
        case ui::RadarMarkKind::HealthCrate: name = "iconhart"; break;
        case ui::RadarMarkKind::PromotionPoint: name = "iconprop"; break;
        }
        const auto* icon = picture(name);
        if (!icon) continue;
        const glm::vec2 point = view.project(mark.mapPoint);
        ui.drawImage(*icon, point.x - kRadarMarkOffset, point.y - kRadarMarkOffset, kRadarMarkWidth, kRadarMarkHeight,
                     mark.colour);
    }
}

void OriginalHud::postMessage(const Interface& userInterface, const std::string& text, float scrollPerTick) {
    const auto& font = userInterface.bigFont();
    const float scale = kBoxTextHeight / static_cast<float>(std::max(font.font().lineHeight, 1));
    textBox_.post(text, render::UiRenderer::textWidth(font, text, scale), scrollPerTick);
}

void OriginalHud::drawTextBox(Interface& userInterface, const std::vector<TeamStrength>& teams) const {
    const auto* entry = textBox_.current();
    if (!entry) return;
    auto& ui = userInterface.renderer();
    const float middle = ui.canvasWidth() / 2.0f;
    const float x0 = middle + kBoxLeft;
    const float x1 = middle + kBoxRight;

    // An end of the tube; the left one is the right one turned round.
    const auto drawEnd = [&](const render::UiImage& image, float x, float y, bool left) {
        if (left) {
            ui.drawImageRegion(image, x, y, image.width(), image.height(), 1.0f, 0.0f, 0.0f, 1.0f);
        } else {
            ui.drawImage(image, x, y, image.width(), image.height());
        }
    };

    if (entry->kind == ui::TextBoxKind::Message) {
        const float y = kSmallHidden + textBox_.lowered() * kSmallTravel;
        if (const auto* piece = picture("npro3")) {
            for (int i = 0; i < kTubePieces; ++i) {
                ui.drawImage(*piece, x0 + kSmallPieceStart + static_cast<float>(i) * kSmallPieceStep, y, piece->width(),
                             piece->height());
            }
        }
        if (const auto* end = picture("npro4")) {
            drawEnd(*end, x0 + kSmallEndLeft, y, true);
            drawEnd(*end, x1 + kSmallEndRight, y, false);
        }
        // The text, cut to the tube's window.
        const auto& font = userInterface.bigFont();
        render::TextStyle style;
        style.scale = kBoxTextHeight / static_cast<float>(std::max(font.font().lineHeight, 1));
        style.shadow = false;
        const float windowLeft = middle + kBoxWindowLeft;
        const float windowTop = y + kSmallTextOffset;
        ui.setClip(windowLeft, std::max(windowTop, 0.0f), ui::TextBox::kWindowWidth, kTextWindowHeight);
        ui.drawText(font, entry->text, windowLeft + textBox_.textX(), windowTop + (kTextWindowHeight - kBoxTextHeight) / 2.0f,
                    style);
        ui.clearClip();
        return;
    }

    // The teams' strength: the big tube and a bar shared by health left.
    const float y = kBigHidden + textBox_.lowered() * kBigTravel;
    if (const auto* piece = picture("npro1")) {
        for (int i = 0; i < kTubePieces; ++i) {
            ui.drawImage(*piece, x0 + static_cast<float>(i) * kBigPieceStep, y, piece->width(), piece->height());
        }
    }
    if (const auto* end = picture("npro2")) {
        drawEnd(*end, x0 + kBigEndLeft, y, true);
        drawEnd(*end, x1, y, false);
    }
    float total = 0.0f;
    for (const auto& team : teams) total += std::max(team.health, 0.0f);
    if (total <= 0.0f) return;
    float x = x0 + kBarLeft;
    const float right = x + kBarWidth;
    const float barTop = y + kBarOffset;
    std::size_t shown = 0;
    for (const auto& team : teams) {
        if (team.health > 0.0f) ++shown;
    }
    for (const auto& team : teams) {
        if (team.health <= 0.0f) continue;
        --shown;
        // The last team takes what is left, so rounding leaves no gap; the
        // others leave one pixel between them.
        const float width = shown == 0 ? right - x : std::round(kBarWidth * team.health / total);
        const float drawn = shown == 0 ? width : width - 1.0f;
        ui.drawRect(x, barTop, drawn, kBarRowHeight, team.colour);
        const glm::vec4 shade = glm::max(team.colour - glm::vec4(kBarShade, kBarShade, kBarShade, 0.0f), glm::vec4(0.0f));
        ui.drawRect(x, barTop + kBarRowHeight, drawn, kBarRowHeight, shade);
        x += width;
    }
}

void OriginalHud::drawRifleSight(render::UiRenderer& ui, bool crosshair) const {
    ui.drawTriangles(ui::sightMaskTriangles(ui.canvasWidth()), kSightBlack);
    if (!crosshair) return;
    if (const auto* picture = this->picture("croshair")) {
        const glm::vec2 corner = ui::crosshairCorner(ui.canvasWidth());
        ui.drawImage(*picture, corner.x, corner.y, ui::kCrosshairSize, ui::kCrosshairSize);
    }
}

void OriginalHud::addDamage(const glm::vec3& head, float amount, const glm::vec4& colour) {
    damageNumbers_.add(head, amount, colour);
}

void OriginalHud::drawDamageNumbers(render::UiRenderer& ui, const glm::mat4& viewProjection,
                                    const glm::vec3& cameraPosition) const {
    for (const auto& number : damageNumbers_.numbers()) {
        const glm::vec4 clip = viewProjection * glm::vec4(number.position, 1.0f);
        if (clip.w <= 0.0f) continue;  // behind the camera
        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        const float centreX = (ndc.x * 0.5f + 0.5f) * ui.canvasWidth();
        const float centreY = (0.5f - ndc.y * 0.5f) * render::kCanvasHeight;
        const float size = ui::floatingDigitSize(glm::length(number.position - cameraPosition));

        // The digits side by side, centred on the point.
        float x = centreX - size * static_cast<float>(number.text.size()) / 2.0f;
        for (const char digit : number.text) {
            if (const auto* image = picture(std::string("num") + digit)) {
                ui.drawImage(*image, x, centreY - size / 2.0f, size, size, number.colour);
            }
            x += size;
        }
    }
}

void OriginalHud::drawHogLabel(Interface& userInterface, const ui::HogLabelBox& box, const ui::HogLabelText& label,
                               float alpha, bool arrows) const {
    auto& ui = userInterface.renderer();
    glm::vec4 colour = ui::teamMessageColour(label.nation);
    colour.a = alpha;

    // The two lines stand at the bottom of the box, right over the hog
    // (or next to it, for a moved label), centred across it.
    const render::UiFont& font = userInterface.labelFont();
    render::TextStyle style;
    style.scale = kLabelLineHeight / static_cast<float>(std::max(1, font.font().lineHeight));
    style.align = render::TextAlign::Centre;
    style.colour = colour;
    const float centreX = box.box.position.x + box.box.size.x / 2.0f;
    const float bottom = box.box.position.y + box.box.size.y;
    const float healthTop = bottom - kLabelLineHeight;
    const float nameTop = label.showHealth ? healthTop - kLabelLineGap - kLabelLineHeight : healthTop;
    ui.drawText(font, label.text, centreX, nameTop, style);

    if (label.showHealth) {
        // The heart and the number side by side, centred together.
        const std::string number = std::to_string(label.health);
        const float numberWidth = render::UiRenderer::textWidth(font, number, style.scale);
        const float left = centreX - (kLabelLineHeight + kHeartGap + numberWidth) / 2.0f;
        if (const auto* heart = picture("heart")) {
            glm::vec4 pink = kHeartPink;
            pink.a = alpha;
            ui.drawImage(*heart, left, healthTop, kLabelLineHeight, kLabelLineHeight, pink);
        }
        style.align = render::TextAlign::Left;
        ui.drawText(font, number, left + kLabelLineHeight + kHeartGap, healthTop, style);
    }

    if (arrows && box.arrow) {
        if (const auto* arrow = picture("arrow000")) {
            const auto& rect = *box.arrow;
            // The picture points down and to the left; mirrored by swapping
            // its texture coordinates.
            const float u0 = box.arrowFlipX ? 1.0f : 0.0f;
            const float v0 = box.arrowFlipY ? 1.0f : 0.0f;
            ui.drawImageRegion(*arrow, rect.position.x, rect.position.y, rect.size.x, rect.size.y, u0, v0, 1.0f - u0,
                               1.0f - v0, colour);
        }
    }
}

const render::UiImage* OriginalHud::picture(std::string_view name) const {
    const auto found = pictures_.find(name);
    return found == pictures_.end() ? nullptr : &found->second;
}

void OriginalHud::update(double elapsedSeconds, const std::optional<ui::HudInfo>& turn, bool weaponMenuOpen,
                         bool radarShown) {
    // Which instruments are wanted now.
    const bool playing = turn && turn->playing;
    const bool wantClock = turn.has_value();
    const bool wantPower = playing && turn->usesPower;
    const bool wantAim = playing && turn->canAim;

    const bool newTurn = turn && turn->turnNumber != lastTurn_;
    if (turn) {
        lamp_ = ui::nextClockLamp(lamp_, turn->exactSecondsLeft, newTurn);
        lastTurn_ = turn->turnNumber;
        if (wantAim) {
            lastAimDegrees_ = turn->aimDegrees;
            lastAimLimited_ = turn->aimLimited;
        }
    }

    damageNumbers_.update(elapsedSeconds);
    for (int tick = ticker_.advance(elapsedSeconds); tick > 0; --tick) {
        powerSlide_.tick(wantPower);
        weaponMenuSlide_.tick(weaponMenuOpen);
        radarSlide_.tick(radarShown);
        textBox_.tick();
        clockSlide_.tick(wantClock);
        aimFade_.tick(wantAim);
    }
}

void OriginalHud::drawPiece(render::UiRenderer& ui, const ui::HudPiece& piece, float yOffset,
                            const glm::vec4& tint) const {
    const auto* image = picture(piece.picture);
    if (!image) return;
    const float x = ui::anchoredX(piece.x, piece.anchor, ui.canvasWidth());
    const float y = piece.y + yOffset;
    if (piece.flippedVertically) {
        ui.drawImageRegion(*image, x, y, image->width(), image->height(), 0.0f, 1.0f, 1.0f, 0.0f, tint);
    } else {
        ui.drawImage(*image, x, y, image->width(), image->height(), tint);
    }
}

void OriginalHud::draw(Interface& userInterface, const std::optional<ui::HudInfo>& turn) const {
    if (!turn) return;
    auto& ui = userInterface.renderer();
    drawAngleGauge(userInterface, *turn);
    drawPowerGauge(ui, *turn);
    drawClock(ui, *turn);
}

void OriginalHud::drawAngleGauge(Interface& userInterface, const ui::HudInfo& turn) const {
    auto& ui = userInterface.renderer();

    // The fan and the arrow go under the dial's brass rim.
    if (aimFade_.level() > 0) {
        const glm::vec4 fanTint{1.0f, 1.0f, 1.0f, aimFade_.fanAlpha()};
        for (const auto& wedge : ui::angleGaugeFan()) {
            drawPiece(ui, wedge, 0.0f, fanTint);
        }
        if (const auto* arrow = picture("angpoint")) {
            const glm::vec2 pivot{ui::anchoredX(ui::kAimPointerPivot.x, ui::kAimPointerAnchor, ui.canvasWidth()),
                                  ui::kAimPointerPivot.y};
            ui.drawImageQuad(*arrow, ui::aimPointerCorners(lastAimDegrees_, pivot),
                             glm::vec4(1.0f, 1.0f, 1.0f, aimFade_.pointerAlpha()));
        }
        // A mortar or guided missile cannot aim below the mark (45 degrees).
        if (lastAimLimited_) {
            drawPiece(ui, ui::kAimLimitMark, 0.0f, glm::vec4(1.0f, 1.0f, 1.0f, aimFade_.pointerAlpha()));
        }
    }
    for (const auto& piece : ui::angleGaugeFrame()) {
        drawPiece(ui, piece);
    }
    if (const auto* icon = userInterface.weaponIcon(turn.weapon)) {
        const auto& place = ui::kAngleGaugeIcon;
        ui.drawImage(*icon, ui::anchoredX(place.x, place.anchor, ui.canvasWidth()), place.y, icon->width(),
                     icon->height());
    }
}

void OriginalHud::drawPowerGauge(render::UiRenderer& ui, const ui::HudInfo& turn) const {
    if (!powerSlide_.visible()) return;
    const float offset = powerSlide_.offset();
    for (const auto& piece : ui::powerGaugeFrame()) {
        drawPiece(ui, piece, offset);
    }
    if (turn.power) {
        drawPiece(ui, ui::kPowerLamp, offset);
    }
    // The needle stands in the tube's scale window, in front of the scale.
    if (const auto* needle = picture("powg1")) {
        const float x = ui::anchoredX(ui::powerNeedleX(turn.power.value_or(0.0f)), ui::HudAnchor::Centre,
                                      ui.canvasWidth());
        ui.drawImage(*needle, x, ui::kPowerNeedleY + offset, needle->width(), needle->height());
    }
}

void OriginalHud::drawClockDigit(render::UiRenderer& ui, float x, float offset, int digit, float roll) const {
    // The digit rolls upwards and the next one down follows it from below;
    // both are cut to the clock's window. The digits are 25 pixels high in
    // a 28-pixel window: one pixel of the window shows above them.
    const int next = (digit + 9) % 10;
    const float top = ui::kClockWindowTop + offset;
    const float bottom = ui::kClockWindowBottom + offset;
    const float y = top + 1.0f;
    for (const auto& [value, digitTop] : {std::pair{digit, y - roll}, std::pair{next, y - roll + ui::kClockDigitHeight}}) {
        const auto* image = picture("timer" + std::to_string(value));
        if (!image) continue;
        const float visibleTop = std::max(digitTop, top);
        const float visibleBottom = std::min(digitTop + image->height(), bottom);
        if (visibleBottom <= visibleTop) continue;
        const float v0 = (visibleTop - digitTop) / image->height();
        const float v1 = (visibleBottom - digitTop) / image->height();
        ui.drawImageRegion(*image, x, visibleTop, image->width(), visibleBottom - visibleTop, 0.0f, v0, 1.0f, v1);
    }
}

void OriginalHud::drawClock(render::UiRenderer& ui, const ui::HudInfo& turn) const {
    if (!clockSlide_.visible()) return;
    const float offset = clockSlide_.offset();

    // The digits roll in the window, behind the clock's face; the lamp
    // glows on top.
    const auto digits = ui::clockDigits(turn.exactSecondsLeft);
    drawClockDigit(ui, ui::anchoredX(ui::kClockDigitX[0], ui::HudAnchor::Right, ui.canvasWidth()), offset,
                   digits.tens, digits.tensRoll);
    drawClockDigit(ui, ui::anchoredX(ui::kClockDigitX[1], ui::HudAnchor::Right, ui.canvasWidth()), offset,
                   digits.units, digits.unitsRoll);
    for (const auto& piece : ui::clockFrame()) {
        drawPiece(ui, piece, offset);
    }
    drawPiece(ui, ui::kClockLamp, offset, ui::lampColour(lamp_));
}

} // namespace openhogs::gameapp
