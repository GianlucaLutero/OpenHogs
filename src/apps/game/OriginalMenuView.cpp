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
// The level panel's words (types 20 and 21): the chosen one, the others,
// and all of them when another screen is on top.
const glm::vec4 kLevelSelectedColour{120 * kTint, 120 * kTint, 100 * kTint, 1.0f};
const glm::vec4 kLevelColour{90 * kTint, 90 * kTint, 70 * kTint, 1.0f};
const glm::vec4 kInactiveLevelColour{45 * kTint, 45 * kTint, 40 * kTint, 1.0f};
// MISSION RESULTS: the layout of words alone (decision 106).
constexpr float kResultsScale = 0.8f;
constexpr float kResultsTitleTop = 60.0f, kResultsLinesTop = 130.0f, kResultsChoicesTop = 360.0f;
constexpr float kResultsLineStep = 30.0f;
// The credits, written in grey (tint 120 on every channel).
const glm::vec4 kCreditsColour{120 * kTint, 120 * kTint, 120 * kTint, 1.0f};
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

namespace {

bool allActions(const ui::Menu& menu) {
    const auto& items = menu.items();
    return std::all_of(items.begin(), items.end(),
                       [](const ui::MenuItem& item) { return item.kind == ui::MenuItem::Kind::Action; });
}

} // namespace

bool OriginalMenuView::suitsSelectList(const ui::Menu& menu) {
    const std::size_t count = menu.items().size();
    return count > ui::kOriginalMenuEntries && count <= ui::kOriginalSelectListEntries && allActions(menu);
}

void OriginalMenuView::update(double seconds) {
    if (!shown_) return;
    if (screen_ == OriginalScreen::Question) {
        dialog_.advance(seconds);
        return;
    }
    if (screen_ == OriginalScreen::SelectList || screen_ == OriginalScreen::Players) {
        selectList_.advance(seconds);
        holders_.advance(seconds);
        return;
    }
    if (screen_ == OriginalScreen::LevelList || screen_ == OriginalScreen::FieldConditions) {
        levelPanel_.advance(seconds);
        return;
    }
    if (screen_ == OriginalScreen::Controls) {
        controlsBoard_.advance(seconds);
        return;
    }
    if (screen_ == OriginalScreen::Credits) {
        credits_.advance(seconds);
        return;
    }
    if (screen_ == OriginalScreen::Squad || screen_ == OriginalScreen::HogPage) {
        squad_.advance(seconds);
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
    case OriginalScreen::SelectList:
    case OriginalScreen::Players: {
        // The monitor shows eight rows; longer menus scroll.
        const std::size_t row = selected - menu.firstVisible(ui::kOriginalSelectListEntries);
        if (changed) {
            selectList_.open(row);
            holders_.open();
        } else {
            selectList_.select(row);
        }
        break;
    }
    case OriginalScreen::Controls:
        if (changed) controlsBoard_.open();
        break;
    case OriginalScreen::Results: break;  // words only, nothing to animate
    case OriginalScreen::Squad:
        if (screen_ == OriginalScreen::HogPage) {
            squad_.closeHogPage();  // back from a hog's page: the squad stays
            squad_.select(selected);
        } else if (changed) {
            squad_.open(selected);
        } else {
            squad_.select(selected);
        }
        break;
    case OriginalScreen::HogPage:
        if (screen_ != OriginalScreen::Squad && screen_ != OriginalScreen::HogPage) squad_.open(squadHog_);
        if (screen_ != OriginalScreen::HogPage) {
            squad_.select(squadHog_);
            squad_.openHogPage(selected);
        } else {
            squad_.selectHogLine(selected);
        }
        break;
    case OriginalScreen::Credits:
        if (changed) credits_.open();
        break;
    case OriginalScreen::LevelList:
    case OriginalScreen::FieldConditions: {
        // The panel follows the level list (FIELD CONDITIONS: the one below).
        const ui::Menu& levels = screen == OriginalScreen::LevelList || !under ? menu : *under;
        const auto layout = ui::levelSlotLayout(levels.items().size(), levels.selectedIndex());
        const bool samePanel = screen_ == OriginalScreen::LevelList || screen_ == OriginalScreen::FieldConditions;
        if (changed && !(samePanel && shown_ != nullptr)) {
            levelPanel_.open(layout.selectedSlot);
        } else {
            levelPanel_.select(layout.selectedSlot);
        }
        break;
    }
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
        if (shown_ == nullptr || screen_ == OriginalScreen::Question || screen_ == OriginalScreen::SelectList) {
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
    case OriginalScreen::SelectList: drawSelectList(ui, menu, originX); break;
    case OriginalScreen::Players:
        drawPieces(ui, holders_.pieces(), originX);
        drawSelectList(ui, menu, originX);
        break;
    case OriginalScreen::LevelList: drawLevelPanel(ui, menu, originX, true); break;
    case OriginalScreen::Controls: drawControls(ui, menu, originX); break;
    case OriginalScreen::Credits: drawCredits(ui, originX); break;
    case OriginalScreen::Squad: drawSquad(ui, menu, originX, true); break;
    case OriginalScreen::Results: drawResults(ui, menu, originX); break;
    case OriginalScreen::HogPage:
        if (under) drawSquad(ui, *under, originX, false);
        drawHogPage(ui, menu, originX);
        break;
    case OriginalScreen::FieldConditions:
        if (under) drawLevelPanel(ui, *under, originX, false);
        drawFieldConditions(ui, menu, originX);
        break;
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

void OriginalMenuView::drawSelectList(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    drawPieces(ui, selectList_.pieces(), originX);
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = selectList_.titleLine();
    ui.drawText(font_, menu.title(), originX + title.centreX, title.top, style);
    if (!selectList_.showsText()) return;
    const auto& items = menu.items();
    const std::size_t first = menu.firstVisible(ui::kOriginalSelectListEntries);
    for (std::size_t row = 0; row < ui::kOriginalSelectListEntries && first + row < items.size(); ++row) {
        const std::size_t i = first + row;
        glm::vec4 colour = i == menu.selectedIndex() ? kSelectedColour : kEntryColour;
        if (!items[i].enabled) colour = glm::vec4(glm::vec3(colour) * kDisabledDimming, 1.0f);
        style.colour = colour;
        const auto line = selectList_.entryLine(row);
        // A choice shows its word (the team's nation, PLAYER or CPU).
        const std::string& text = items[i].kind == ui::MenuItem::Kind::Choice ? items[i].choiceText() : items[i].label;
        ui.drawText(font_, text, originX + line.centreX, line.top, style);
    }
}

void OriginalMenuView::drawControls(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    drawPieces(ui, controlsBoard_.pieces(), originX);
    if (!controlsBoard_.showsText()) return;
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = controlsBoard_.titleLine();
    ui.drawText(questionFont_, menu.title(), originX + title.centreX, title.top, style);
    const auto& items = menu.items();
    const std::size_t first = menu.firstVisible(ui::kControlsBoardRows);
    for (std::size_t row = 0; row < ui::kControlsBoardRows && first + row < items.size(); ++row) {
        const auto& item = items[first + row];
        style.colour = first + row == menu.selectedIndex() ? kLevelSelectedColour : kLevelColour;
        const auto action = controlsBoard_.actionLine(row);
        ui.drawText(questionFont_, item.label, originX + action.centreX, action.top, style);
        const auto key = controlsBoard_.keyLine(row);
        ui.drawText(questionFont_, item.detail, originX + key.centreX, key.top, style);
    }
}

void OriginalMenuView::drawCredits(render::UiRenderer& ui, float originX) {
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kCreditsColour;
    for (const auto& row : credits_.visibleRows(creditRows_.size())) {
        const auto& [name, job] = creditRows_[row.index];
        ui.drawText(questionFont_, name, originX + row.name.centreX, row.name.top, style);
        ui.drawText(questionFont_, job, originX + row.job.centreX, row.job.top, style);
    }
}

void OriginalMenuView::drawSquad(render::UiRenderer& ui, const ui::Menu& squad, float originX, bool active) {
    const auto& items = squad.items();
    const std::size_t hogs = items.size() >= ui::kSquadOptions ? items.size() - ui::kSquadOptions : 0;
    drawPieces(ui, squad_.pieces(hogs), originX);
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = squad_.titleLine();
    ui.drawText(font_, squad.title(), originX + title.centreX, title.top, style);
    if (!squad_.showsText()) return;
    const float half = static_cast<float>(font_.font().lineHeight) * kTextScale / 2.0f;
    const std::size_t selected = active ? squad.selectedIndex() : squadHog_;
    for (std::size_t i = 0; i < items.size(); ++i) {
        glm::vec4 colour = i == selected ? kSelectedColour : kEntryColour;
        if (!items[i].enabled) colour = glm::vec4(glm::vec3(colour) * kDisabledDimming, 1.0f);
        style.colour = colour;
        const auto line = i < hogs ? squad_.nameLine(i) : squad_.optionLine(i - hogs);
        ui.drawText(font_, items[i].label, originX + line.centreX, line.top - half, style);
    }
    // The monitor: the chosen hog's rank, then the page's lines (hidden by
    // a hog's page).
    if (!active) return;
    std::vector<std::string> monitor;
    if (selected < hogs) monitor.push_back(items[selected].detail);
    monitor.insert(monitor.end(), squadLines_.begin(), squadLines_.end());
    style.colour = kLevelColour;
    for (std::size_t i = 0; i < monitor.size(); ++i) {
        const auto line = squad_.monitorLine(i);
        ui.drawText(font_, monitor[i], originX + line.centreX, line.top, style);
    }
}

void OriginalMenuView::drawResults(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    // The executable's results screens (type 10) draw no picture: the title,
    // the lines and the choices are written over the background.
    render::TextStyle style;
    style.scale = kResultsScale;
    style.align = render::TextAlign::Centre;
    style.colour = kTitleColour;
    const float centre = originX + ui::kOriginalScreenWidth / 2.0f;
    ui.drawText(questionFont_, menu.title(), centre, kResultsTitleTop, style);
    style.colour = kCreditsColour;
    float y = kResultsLinesTop;
    for (const auto& line : squadLines_) {
        ui.drawText(questionFont_, line, centre, y, style);
        y += kResultsLineStep;
    }
    const auto& items = menu.items();
    y = kResultsChoicesTop;
    for (std::size_t i = 0; i < items.size(); ++i) {
        style.colour = i == menu.selectedIndex() ? kSelectedColour : kEntryColour;
        ui.drawText(questionFont_, items[i].label, centre, y, style);
        y += kResultsLineStep;
    }
}

void OriginalMenuView::drawHogPage(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    if (!squad_.showsHogPage()) return;
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    const auto& items = menu.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        glm::vec4 colour = i == menu.selectedIndex() ? kSelectedColour : kEntryColour;
        if (!items[i].enabled) colour = glm::vec4(glm::vec3(colour) * kDisabledDimming, 1.0f);
        style.colour = colour;
        const auto line = squad_.hogPageLine(i);
        ui.drawText(font_, items[i].label, originX + line.centreX, line.top, style);
    }
}

void OriginalMenuView::drawLevelPanel(render::UiRenderer& ui, const ui::Menu& levels, float originX, bool active) {
    drawPieces(ui, levelPanel_.pieces(), originX);
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = levelPanel_.titleLine();
    if (active) ui.drawText(font_, levels.title(), originX + title.centreX, title.top, style);
    if (!levelPanel_.showsText()) return;
    const auto layout = ui::levelSlotLayout(levels.items().size(), levels.selectedIndex());
    for (std::size_t slot = 0; slot < ui::kLevelPanelSlots; ++slot) {
        if (layout.items[slot] < 0) continue;
        const auto& item = levels.items()[static_cast<std::size_t>(layout.items[slot])];
        if (!active) {
            style.colour = kInactiveLevelColour;
        } else {
            style.colour = slot == layout.selectedSlot ? kLevelSelectedColour : kLevelColour;
        }
        const auto line = levelPanel_.slotLine(slot);
        ui.drawText(font_, item.label, originX + line.centreX, line.top, style);
    }
}

void OriginalMenuView::drawFieldConditions(render::UiRenderer& ui, const ui::Menu& menu, float originX) {
    render::TextStyle style;
    style.scale = kTextScale;
    style.align = render::TextAlign::Centre;
    style.shadow = false;
    style.colour = kTitleColour;
    const auto title = levelPanel_.titleLine();
    ui.drawText(font_, menu.title(), originX + title.centreX, title.top, style);
    if (!levelPanel_.showsText()) return;
    const auto& items = menu.items();
    const std::size_t first = menu.firstVisible(ui::kLevelPanelMonitorRows);
    for (std::size_t row = 0; row < ui::kLevelPanelMonitorRows && first + row < items.size(); ++row) {
        const auto& item = items[first + row];
        style.colour = first + row == menu.selectedIndex() ? kLevelSelectedColour : kLevelColour;
        // "LABEL: VALUE", as the executable writes FIELD CONDITIONS.
        std::string text = item.label;
        if (item.kind == ui::MenuItem::Kind::Choice) text += ": " + item.choiceText();
        const auto line = levelPanel_.monitorLine(row);
        ui.drawText(font_, text, originX + line.centreX, line.top, style);
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
