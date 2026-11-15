// OriginalMenuView.cpp - see OriginalMenuView.h for an overview.

#include "OriginalMenuView.h"

#include "openhogs/ui/HudLayout.h"

#include <algorithm>

namespace openhogs::gameapp {

namespace {

// The executable writes the menus with chars3 (30 pixels tall) in its
// 1024 x 820 text space, so on its 640 x 480 screen the letters are about
// 0.6 of their size.
constexpr float kTextScale = 0.6f;

// The executable's text colours (a font tint where 128 is the picture's own
// colour): the title, an entry, the selected entry. Disabled entries are a
// third as bright.
constexpr float kTint = 1.0f / 128.0f;
const glm::vec4 kTitleColour{100 * kTint, 100 * kTint, 60 * kTint, 1.0f};
const glm::vec4 kEntryColour{80 * kTint, 80 * kTint, 45 * kTint, 1.0f};
const glm::vec4 kSelectedColour{120 * kTint, 120 * kTint, 75 * kTint, 1.0f};
constexpr float kDisabledDimming = 1.0f / 3.0f;
// The question's title, set by the executable for these screens.
const glm::vec4 kQuestionColour{120 * kTint, 120 * kTint, 80 * kTint, 1.0f};
// The blinking value of VOLUME CONTROL (tint 200, brighter than the
// picture, is as bright as the font gets here).
const glm::vec4 kBlinkLitColour{1.0f, 1.0f, 1.0f, 1.0f};
const glm::vec4 kBlinkDarkColour{20 * kTint, 20 * kTint, 20 * kTint, 1.0f};

} // namespace

OriginalMenuView::OriginalMenuView(FrontEndPictures& pictures, const render::UiFont& font,
                                   const render::UiFont& questionFont)
    : pictures_(pictures), font_(font), questionFont_(questionFont) {}

bool OriginalMenuView::suits(const ui::Menu& menu) {
    const auto& items = menu.items();
    return !items.empty() && items.size() <= ui::kOriginalMenuEntries &&
           std::all_of(items.begin(), items.end(),
                       [](const ui::MenuItem& item) { return item.kind == ui::MenuItem::Kind::Action; });
}

bool OriginalMenuView::suitsVolume(const ui::Menu& menu) {
    using Kind = ui::MenuItem::Kind;
    const auto& items = menu.items();
    return items.size() == ui::kVolumeEntries && items[0].kind == Kind::Choice &&
           items[ui::kVolumeMasterEntry].kind == Kind::Slider && items[ui::kVolumeEffectsEntry].kind == Kind::Slider &&
           items[3].kind == Kind::Choice;
}

void OriginalMenuView::update(double seconds) {
    if (!shown_) return;
    if (screen_ == OriginalScreen::Question) {
        dialog_.advance(seconds);
        return;
    }
    animation_.advance(seconds);
    if (screen_ == OriginalScreen::Volume || volumeLeaving_) volume_.advance(seconds);
    if (volumeLeaving_ && volume_.closed()) {
        // The panel is gone: the list comes back down.
        volumeLeaving_ = false;
        animation_.raise(false);
    }
}

void OriginalMenuView::show(const ui::Menu& menu, OriginalScreen screen, const ui::Menu* under) {
    const std::size_t selected = menu.selectedIndex();
    const bool changed = shown_ != &menu || screen_ != screen;
    switch (screen) {
    case OriginalScreen::Question:
        // The question has its own machine: it opens whenever it appears.
        if (changed) {
            dialog_.open(selected);
        } else if (selected_ != selected) {
            dialog_.select(selected);
        }
        break;
    case OriginalScreen::Volume:
        if (changed) {
            // The list above stays if it is the one on screen; otherwise it
            // drops in first.
            const std::size_t underSelected = under ? under->selectedIndex() : 0;
            if (shown_ == nullptr || screen_ != OriginalScreen::List || shown_ != under) {
                animation_.open(underSelected);
            }
            animation_.raise(true);
            volume_.open(selected);
            volumeLeaving_ = false;
        } else if (selected_ != selected) {
            volume_.select(selected);
        }
        masterLevel_ = menu.items()[ui::kVolumeMasterEntry].value;
        effectsLevel_ = menu.items()[ui::kVolumeEffectsEntry].value;
        break;
    case OriginalScreen::List:
        if (shown_ == nullptr || screen_ == OriginalScreen::Question) {
            animation_.open(selected);
            volumeLeaving_ = false;
        } else if (screen_ == OriginalScreen::Volume) {
            // Back from VOLUME CONTROL to the list above it: the panel
            // leaves, then the list comes down.
            volume_.close();
            volumeLeaving_ = true;
            animation_.select(selected);
        } else if (shown_ != &menu) {
            animation_.replaceContents(selected);
        } else if (selected_ != selected) {
            animation_.select(selected);
        }
        break;
    }
    shown_ = &menu;
    screen_ = screen;
    selected_ = selected;
}

void OriginalMenuView::drawPiece(render::UiRenderer& ui, const ui::MenuPiece& piece, float originX) {
    const render::UiImage* image = pictures_.picture(piece.picture);
    if (!image) return;
    const float sourceWidth = piece.sourceWidth > 0 ? static_cast<float>(piece.sourceWidth) : image->width();
    const float sourceHeight = piece.sourceHeight > 0 ? static_cast<float>(piece.sourceHeight) : image->height();
    const float width = piece.width > 0.0f ? piece.width : sourceWidth;
    const float height = piece.height > 0.0f ? piece.height : sourceHeight;
    float u0 = static_cast<float>(piece.sourceX) / image->width();
    float u1 = (static_cast<float>(piece.sourceX) + sourceWidth) / image->width();
    const float v0 = static_cast<float>(piece.sourceY) / image->height();
    const float v1 = (static_cast<float>(piece.sourceY) + sourceHeight) / image->height();
    float x = originX + piece.x;
    if (piece.mirrored) {
        std::swap(u0, u1);
        x -= width;  // a mirrored piece ends at its x
    }
    ui.drawImageRegion(*image, x, piece.y, width, height, u0, v0, u1, v1);
}

void OriginalMenuView::drawPieces(render::UiRenderer& ui, const std::vector<ui::MenuPiece>& pieces,
                                  float originX) {
    // The executable lists the pieces from the top one down: draw them the
    // other way round so the first one ends up on top.
    for (auto piece = pieces.rbegin(); piece != pieces.rend(); ++piece) drawPiece(ui, *piece, originX);
}

void OriginalMenuView::draw(render::UiRenderer& ui, const ui::Menu& menu, OriginalScreen screen,
                            const ui::Menu* under) {
    show(menu, screen, under);
    // The original screen is 640 pixels wide; wider canvases get the same
    // layout in the middle (the canvas is 480 units high, like the screen).
    const float originX = (ui.canvasWidth() - ui::kOriginalScreenWidth) / 2.0f;
    switch (screen) {
    case OriginalScreen::Question: drawQuestion(ui, menu, originX); break;
    case OriginalScreen::Volume:
        if (under) drawList(ui, *under, originX);
        drawVolume(ui, menu, originX);
        break;
    case OriginalScreen::List:
        drawList(ui, menu, originX);
        if (volumeLeaving_) drawVolumePanel(ui, originX);
        break;
    }
}

void OriginalMenuView::drawVolumePanel(render::UiRenderer& ui, float originX) {
    drawPieces(ui, volume_.pieces(masterLevel_, effectsLevel_), originX);
}

void OriginalMenuView::drawVolume(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    drawVolumePanel(ui, originX);
    if (!volume_.showsText()) return;
    render::TextStyle style;
    style.scale = kTextScale;
    style.shadow = false;
    const float halfHeight = static_cast<float>(questionFont_.font().lineHeight) * kTextScale / 2.0f;
    const auto& items = menu.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto line = volume_.textLine(i);
        const float top = line.middle - halfHeight;
        style.align = render::TextAlign::Right;
        style.colour = kQuestionColour;
        ui.drawText(questionFont_, items[i].label, originX + line.labelRight, top, style);
        if (items[i].kind != ui::MenuItem::Kind::Choice) continue;
        // The selected value blinks, between the executable's tints 200 and 20.
        if (i == menu.selectedIndex()) {
            style.colour = volume_.valueLit() ? kBlinkLitColour : kBlinkDarkColour;
        }
        style.align = render::TextAlign::Left;
        ui.drawText(questionFont_, items[i].choiceText(), originX + line.valueLeft, top, style);
    }
}

void OriginalMenuView::drawQuestion(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    drawPieces(ui, dialog_.pieces(), originX);
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kQuestionColour;
    const auto title = dialog_.titleLine();
    ui.drawText(questionFont_, menu.title(), originX + title.centreX, title.top, style);
    if (!dialog_.showsAnswers()) return;
    const auto& items = menu.items();
    for (std::size_t i = 0; i < items.size() && i < 2; ++i) {
        style.colour = i == menu.selectedIndex() ? kSelectedColour : kEntryColour;
        const auto line = dialog_.answerLine(i);
        ui.drawText(questionFont_, items[i].label, originX + line.centreX, line.top, style);
    }
}

void OriginalMenuView::drawList(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    drawPieces(ui, animation_.pieces(), originX);
    if (!animation_.showsText()) return;
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = animation_.titleLine();
    ui.drawText(font_, menu.title(), originX + title.centreX, title.top, style);

    const auto& items = menu.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        glm::vec4 colour = i == menu.selectedIndex() ? kSelectedColour : kEntryColour;
        if (!items[i].enabled) colour = glm::vec4(glm::vec3(colour) * kDisabledDimming, 1.0f);
        style.colour = colour;
        const auto line = animation_.entryLine(i);
        ui.drawText(font_, items[i].label, originX + line.centreX, line.top, style);
    }
}

} // namespace openhogs::gameapp
