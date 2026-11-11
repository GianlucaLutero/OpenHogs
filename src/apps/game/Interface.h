// Interface.h
//
// Everything the game application needs to draw its 2D interface (menus,
// HUD, messages), loaded once and shared by the menus and the match:
//
//   - the renderer (render::UiRenderer);
//   - the game's fonts: FEText/BIG (titles, menu entries, messages) and
//     FEText/SMALL (small labels). Both have the accented letters of the
//     translations, unlike GameChars and BigChars (docs/formats/fonts.md);
//   - the weapon icons of Language/Tims/MENUTIMS.MAD;
//   - the texts (ui::UiStrings) and the game text table (weapon names).
//
// It also has the drawing functions shared by several screens: a menu, a
// big centred message, a bar. Needs a current OpenGL context.

#pragma once

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Weapons.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace openhogs::gameapp {

// The interface's colours.
namespace colours {
inline constexpr glm::vec4 kTitle{1.0f, 0.82f, 0.25f, 1.0f};     // gold
inline constexpr glm::vec4 kText{0.95f, 0.93f, 0.85f, 1.0f};     // off-white
inline constexpr glm::vec4 kSelected{1.0f, 0.95f, 0.4f, 1.0f};   // yellow
inline constexpr glm::vec4 kDisabled{0.5f, 0.5f, 0.5f, 1.0f};
inline constexpr glm::vec4 kPanel{0.05f, 0.07f, 0.04f, 0.72f};   // dark khaki, see-through
inline constexpr glm::vec4 kHighlight{0.55f, 0.45f, 0.1f, 0.55f};
inline constexpr glm::vec4 kBarBack{0.1f, 0.1f, 0.1f, 0.85f};
} // namespace colours

class Interface {
public:
    // Throws std::runtime_error if the fonts are missing: there is no
    // interface without them. Missing icons or texts only cause warnings.
    explicit Interface(const core::VirtualFileSystem& vfs);

    render::UiRenderer& renderer() { return renderer_; }
    const render::UiFont& bigFont() const { return *bigFont_; }
    const render::UiFont& smallFont() const { return *smallFont_; }
    const ui::UiStrings& strings() const { return strings_; }
    const std::vector<std::string>& gameText() const { return gameText_; }

    // The icon of a weapon, or nullptr if it was not found.
    const render::UiImage* weaponIcon(game::WeaponId weapon) const;

    // --- Shared drawing (between renderer().begin() and end()) -----------

    // A menu in a panel centred on the screen, at most `rows` items tall,
    // with optional lines of information under its title.
    void drawMenu(const ui::Menu& menu, std::size_t rows = 10, const std::vector<std::string>& lines = {});

    // A picture stretched to fill the height of the screen, centred.
    void drawPicture(const render::UiImage& image);

    // A big message across the middle of the screen at height `y`, gold
    // unless another colour is given.
    void drawBanner(std::string_view text, float y, float alpha = 1.0f, const glm::vec4& colour = colours::kTitle);

    // A bar `fraction` (0..1) full.
    void drawBar(float x, float y, float width, float height, float fraction, const glm::vec4& colour);

private:
    void loadWeaponIcons(const core::VirtualFileSystem& vfs);

    render::UiRenderer renderer_;
    std::unique_ptr<render::UiFont> bigFont_;
    std::unique_ptr<render::UiFont> smallFont_;
    std::map<game::WeaponId, render::UiImage> weaponIcons_;
    ui::UiStrings strings_;
    std::vector<std::string> gameText_;
};

} // namespace openhogs::gameapp
