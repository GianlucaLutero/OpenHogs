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

} // namespace

OriginalMenuView::OriginalMenuView(FrontEndPictures& pictures, const render::UiFont& font)
    : pictures_(pictures), font_(font) {}

bool OriginalMenuView::suits(const ui::Menu& menu) {
    const auto& items = menu.items();
    return !items.empty() && items.size() <= ui::kOriginalMenuEntries &&
           std::all_of(items.begin(), items.end(),
                       [](const ui::MenuItem& item) { return item.kind == ui::MenuItem::Kind::Action; });
}

void OriginalMenuView::update(double seconds) {
    if (shown_) animation_.advance(seconds);
}

void OriginalMenuView::show(const ui::Menu& menu) {
    if (shown_ == nullptr) {
        animation_.open(menu.selectedIndex());
    } else if (shown_ != &menu) {
        animation_.replaceContents(menu.selectedIndex());
    } else if (selected_ != menu.selectedIndex()) {
        animation_.select(menu.selectedIndex());
    }
    shown_ = &menu;
    selected_ = menu.selectedIndex();
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

void OriginalMenuView::draw(render::UiRenderer& ui, const ui::Menu& menu) {
    show(menu);
    // The original screen is 640 pixels wide; wider canvases get the same
    // layout in the middle (the canvas is 480 units high, like the screen).
    const float originX = (ui.canvasWidth() - ui::kOriginalScreenWidth) / 2.0f;

    // The executable lists the pieces from the top one down: draw them the
    // other way round so the first one ends up on top.
    const auto pieces = animation_.pieces();
    for (auto piece = pieces.rbegin(); piece != pieces.rend(); ++piece) drawPiece(ui, *piece, originX);

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
