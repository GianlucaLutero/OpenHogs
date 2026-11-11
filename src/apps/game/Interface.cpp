// Interface.cpp - see Interface.h for an overview.

#include "Interface.h"

#include "openhogs/formats/BitmapFont.h"
#include "openhogs/formats/BitmapImage.h"
#include "openhogs/formats/PackageArchive.h"
#include "openhogs/formats/TextTable.h"

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>

namespace openhogs::gameapp {

namespace {

// Menu layout, in canvas units.
constexpr float kMenuTitleScale = 1.0f;
constexpr float kMenuItemScale = 0.75f;
constexpr float kMenuRowHeight = 28.0f;
constexpr float kMenuPadding = 18.0f;
constexpr float kMenuMinWidth = 300.0f;
constexpr float kSliderWidth = 150.0f;
constexpr float kSliderHeight = 12.0f;
constexpr float kBannerScale = 1.25f;

// The weapon icons are stored in MENUTIMS.MAD in the order of the weapon
// entries of the game text table, right after "menu.bmp": entry 1 is the
// icon of text entry 97 (TROTTER), entry 2 of 98 (KNIFE), and so on. So
// the weapon id (= its text entry) gives the icon directly.
const char* const kMenuPictures = "Language/Tims/MENUTIMS.MAD";
constexpr int kFirstWeaponTextEntry = 97;

std::unique_ptr<render::UiFont> loadFont(const core::VirtualFileSystem& vfs, const std::string& name) {
    try {
        return std::make_unique<render::UiFont>(formats::loadBitmapFont(vfs.readFile("FEText/" + name + ".tab"),
                                                                        vfs.readFile("FEText/" + name + ".BMP")));
    } catch (const std::exception& error) {
        throw std::runtime_error("Cannot load the font FEText/" + name + ": " + error.what());
    }
}

std::vector<std::string> loadTextTable(const core::VirtualFileSystem& vfs, const std::string& name) {
    try {
        return formats::TextTable::parse(vfs.readFile("Language/Text/" + name + ".ofs"),
                                         vfs.readFile("Language/Text/" + name + ".bin"))
            .entries;
    } catch (const std::exception& error) {
        std::cerr << "Warning: no text table " << name << ", using English (" << error.what() << ")\n";
        return {};
    }
}

} // namespace

glm::vec4 teamColour(int team) {
    // In scene::characterTeams() order: british, american, french, german,
    // japanese, russian, team lard. OpenHogs' own choice
    // (docs/open-decisions.md).
    constexpr std::array<glm::vec4, 7> kColours{{
        {0.90f, 0.25f, 0.20f, 1.0f},  // red
        {0.35f, 0.55f, 1.00f, 1.0f},  // blue
        {0.40f, 0.85f, 0.95f, 1.0f},  // light blue
        {0.65f, 0.65f, 0.60f, 1.0f},  // grey
        {0.95f, 0.95f, 0.95f, 1.0f},  // white
        {0.95f, 0.75f, 0.20f, 1.0f},  // gold
        {0.90f, 0.50f, 0.80f, 1.0f},  // pink
    }};
    const auto count = static_cast<int>(kColours.size());
    return kColours[static_cast<std::size_t>(((team % count) + count) % count)];
}

Interface::Interface(const core::VirtualFileSystem& vfs) {
    bigFont_ = loadFont(vfs, "BIG");
    smallFont_ = loadFont(vfs, "SMALL");
    gameText_ = loadTextTable(vfs, "gtext");
    strings_ = ui::loadUiStrings(loadTextTable(vfs, "fetext"), gameText_);
    loadWeaponIcons(vfs);
}

void Interface::loadWeaponIcons(const core::VirtualFileSystem& vfs) {
    try {
        const auto archive = formats::PackageArchive::parse(vfs.readFile(kMenuPictures));
        const auto& entries = archive.entries();
        for (const auto& weapon : game::allWeapons()) {
            const auto index = static_cast<std::size_t>(static_cast<int>(weapon.id) - kFirstWeaponTextEntry + 1);
            if (index >= entries.size()) continue;
            const auto image = formats::decodeBitmap(archive.entryData(entries[index]));
            weaponIcons_.emplace(weapon.id, render::UiImage(image));
        }
    } catch (const std::exception& error) {
        std::cerr << "Warning: no weapon icons (" << error.what() << ")\n";
    }
}

const render::UiImage* Interface::weaponIcon(game::WeaponId weapon) const {
    const auto found = weaponIcons_.find(weapon);
    return found == weaponIcons_.end() ? nullptr : &found->second;
}

void Interface::drawBar(float x, float y, float width, float height, float fraction, const glm::vec4& colour) {
    renderer_.drawRect(x, y, width, height, colours::kBarBack);
    const float inset = 2.0f;
    const float inner = (width - 2.0f * inset) * std::clamp(fraction, 0.0f, 1.0f);
    renderer_.drawRect(x + inset, y + inset, inner, height - 2.0f * inset, colour);
}

void Interface::drawBanner(std::string_view text, float y, float alpha) {
    render::TextStyle style;
    style.scale = kBannerScale;
    style.colour = colours::kTitle;
    style.colour.a = alpha;
    style.align = render::TextAlign::Centre;
    // Very long messages (translations) shrink to fit the screen.
    const float maxWidth = renderer_.canvasWidth() - 40.0f;
    const float width = render::UiRenderer::textWidth(*bigFont_, text, style.scale);
    if (width > maxWidth) {
        style.scale *= maxWidth / width;
    }
    renderer_.drawText(*bigFont_, text, renderer_.canvasWidth() / 2.0f, y, style);
}

void Interface::drawMenu(const ui::Menu& menu, std::size_t rows) {
    const auto& items = menu.items();
    const std::size_t first = menu.firstVisible(rows);
    const std::size_t shown = std::min(rows, items.size() - first);

    // Wide enough for the title and the longest entry (and its slider).
    float width = render::UiRenderer::textWidth(*bigFont_, menu.title(), kMenuTitleScale);
    for (const auto& item : items) {
        float itemWidth = render::UiRenderer::textWidth(*bigFont_, item.label, kMenuItemScale);
        if (item.kind == ui::MenuItem::Kind::Slider) itemWidth += kSliderWidth + kMenuPadding;
        width = std::max(width, itemWidth);
    }
    width = std::max(kMenuMinWidth, width + 2.0f * kMenuPadding);
    const float titleHeight = static_cast<float>(bigFont_->font().lineHeight) * kMenuTitleScale;
    const float height = kMenuPadding * 3.0f + titleHeight + kMenuRowHeight * static_cast<float>(shown);

    const float left = (renderer_.canvasWidth() - width) / 2.0f;
    const float top = (render::kCanvasHeight - height) / 2.0f;
    renderer_.drawRect(left, top, width, height, colours::kPanel);

    render::TextStyle titleStyle;
    titleStyle.scale = kMenuTitleScale;
    titleStyle.colour = colours::kTitle;
    titleStyle.align = render::TextAlign::Centre;
    renderer_.drawText(*bigFont_, menu.title(), left + width / 2.0f, top + kMenuPadding, titleStyle);

    const float itemHeight = static_cast<float>(bigFont_->font().lineHeight) * kMenuItemScale;
    float y = top + kMenuPadding * 2.0f + titleHeight;
    for (std::size_t i = first; i < first + shown; ++i) {
        const auto& item = items[i];
        const bool selected = i == menu.selectedIndex();
        if (selected) {
            renderer_.drawRect(left + 6.0f, y - 2.0f, width - 12.0f, kMenuRowHeight, colours::kHighlight);
        }
        render::TextStyle style;
        style.scale = kMenuItemScale;
        style.colour = !item.enabled ? colours::kDisabled : selected ? colours::kSelected : colours::kText;
        const float textY = y + (kMenuRowHeight - itemHeight) / 2.0f - 2.0f;

        if (item.kind == ui::MenuItem::Kind::Slider) {
            renderer_.drawText(*bigFont_, item.label, left + kMenuPadding, textY, style);
            const float sliderX = left + width - kMenuPadding - kSliderWidth;
            const float fraction =
                item.maxValue > 0 ? static_cast<float>(item.value) / static_cast<float>(item.maxValue) : 0.0f;
            drawBar(sliderX, y + (kMenuRowHeight - kSliderHeight) / 2.0f - 2.0f, kSliderWidth, kSliderHeight,
                    fraction, style.colour);
        } else {
            style.align = render::TextAlign::Centre;
            renderer_.drawText(*bigFont_, item.label, left + width / 2.0f, textY, style);
        }
        y += kMenuRowHeight;
    }

    // Arrows when the list goes on above or below.
    render::TextStyle arrowStyle;
    arrowStyle.colour = colours::kTitle;
    arrowStyle.align = render::TextAlign::Right;
    if (first > 0) {
        renderer_.drawText(*smallFont_, "...", left + width - 8.0f, top + kMenuPadding * 2.0f + titleHeight - 12.0f,
                           arrowStyle);
    }
    if (first + shown < items.size()) {
        renderer_.drawText(*smallFont_, "...", left + width - 8.0f, top + height - 14.0f, arrowStyle);
    }
}

} // namespace openhogs::gameapp
