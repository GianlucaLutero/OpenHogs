// OriginalMenuView.h
//
// Draws a short menu (at most four entries, all of them actions: the main
// menu, ONE PLAYER) as the original game's list menu: the brass panel with
// its bars, lamps, dial and cogs, which drops in with a spring
// (ui::OriginalMenu has the layout and the animation; see
// docs/formats/front-end.md). The title and the entries are written in the
// original's menu font, chars3.
//
// A yes/no question (REALLY QUIT APP?) is drawn as the original's question
// instead: the plate and the box with the YES and NO windows and their
// dial (ui::OriginalDialog), written with the larger font.
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
#include "openhogs/ui/OriginalDialog.h"
#include "openhogs/ui/OriginalMenu.h"

#include <cstddef>
#include <vector>

namespace openhogs::gameapp {

class OriginalMenuView {
public:
    OriginalMenuView(FrontEndPictures& pictures, const render::UiFont& font, const render::UiFont& questionFont);

    // Whether `menu` can be shown this way: four entries or fewer, all of
    // them actions (sliders and choices need the other screens).
    static bool suits(const ui::Menu& menu);

    // Follows `menu` (which must suit; `question`: a yes/no question, YES
    // first): opens, flips or turns the dial when it changed since the last
    // call. draw() calls it too.
    void show(const ui::Menu& menu, bool question = false);

    // Moves the animation on by `seconds`.
    void update(double seconds);

    // Draws `menu` (which must suit) centred on the canvas, between
    // ui.begin() and ui.end().
    void draw(render::UiRenderer& ui, const ui::Menu& menu, bool question = false);

    // The next draw starts the opening animation again (call when the list
    // menu was hidden, e.g. by a match).
    void reset() { shown_ = nullptr; }

private:
    void drawPiece(render::UiRenderer& ui, const ui::MenuPiece& piece, float originX);
    void drawPieces(render::UiRenderer& ui, const std::vector<ui::MenuPiece>& pieces, float originX);
    void drawList(render::UiRenderer& ui, const ui::Menu& menu, float originX);
    void drawQuestion(render::UiRenderer& ui, const ui::Menu& menu, float originX);

    FrontEndPictures& pictures_;
    const render::UiFont& font_;
    const render::UiFont& questionFont_;
    ui::OriginalMenu animation_;
    ui::OriginalDialog dialog_;
    const ui::Menu* shown_ = nullptr;  // the menu drawn last time
    bool question_ = false;            // ...and whether it was a question
    std::size_t selected_ = 0;
};

} // namespace openhogs::gameapp
