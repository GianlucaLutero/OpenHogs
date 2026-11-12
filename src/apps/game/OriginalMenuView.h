// OriginalMenuView.h
//
// Draws a short menu (at most four entries, all of them actions: the main
// menu, ONE PLAYER, ARE YOU SURE?) as the original game's list menu: the
// brass panel with its bars, lamps, dial and cogs, which drops in with a
// spring (ui::OriginalMenu has the layout and the animation; see
// docs/formats/front-end.md). The title and the entries are written in the
// original's menu font, chars3.
//
// The view follows the menu it is given each frame: a menu it was not
// showing opens with the full animation (or only flips its plates if a
// list menu was already on screen), and a new selection turns the dial.
//
// Needs a current OpenGL context.

#pragma once

#include "FrontEndPictures.h"

#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>

namespace openhogs::gameapp {

class OriginalMenuView {
public:
    OriginalMenuView(FrontEndPictures& pictures, const render::UiFont& font);

    // Whether `menu` can be shown this way: four entries or fewer, all of
    // them actions (sliders and choices need the other screens).
    static bool suits(const ui::Menu& menu);

    // Follows `menu` (which must suit): opens, flips or turns the dial when
    // it changed since the last call. draw() calls it too.
    void show(const ui::Menu& menu);

    // Moves the animation on by `seconds`.
    void update(double seconds);

    // Draws `menu` (which must suit) centred on the canvas, between
    // ui.begin() and ui.end().
    void draw(render::UiRenderer& ui, const ui::Menu& menu);

    // The next draw starts the opening animation again (call when the list
    // menu was hidden, e.g. by a match).
    void reset() { shown_ = nullptr; }

private:
    void drawPiece(render::UiRenderer& ui, const ui::MenuPiece& piece, float originX);

    FrontEndPictures& pictures_;
    const render::UiFont& font_;
    ui::OriginalMenu animation_;
    const ui::Menu* shown_ = nullptr;  // the menu drawn last time
    std::size_t selected_ = 0;
};

} // namespace openhogs::gameapp
