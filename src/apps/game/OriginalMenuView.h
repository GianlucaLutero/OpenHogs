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
// VOLUME CONTROL is drawn as the original's: the OPTIONS list rises and
// the volume panel comes up below it, with its knobs (ui::OriginalVolume).
// Going back, the panel drops away before the list comes down again.
//
// A list of five to eight actions (SELECT TEAM) is drawn as the original's
// longer list: a monitor showing the entries, with a selector that slides
// down it (ui::OriginalSelectList).
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
#include "openhogs/ui/OriginalSelectList.h"
#include "openhogs/ui/OriginalVolume.h"

#include <cstddef>
#include <vector>

namespace openhogs::gameapp {

// Which of the original's screens a menu is shown as.
enum class OriginalScreen {
    List,      // the list menu (four actions or fewer)
    Question,  // a yes/no question, YES first
    Volume,    // VOLUME CONTROL: MODE, MASTER, SFX, VOICES over a list menu
    SelectList,  // the longer list: five to eight actions on a monitor
};

class OriginalMenuView {
public:
    OriginalMenuView(FrontEndPictures& pictures, const render::UiFont& font, const render::UiFont& questionFont);

    // Whether `menu` can be shown this way: four entries or fewer, all of
    // them actions (sliders and choices need the other screens).
    static bool suits(const ui::Menu& menu);

    // Whether `menu` is the original's VOLUME CONTROL (four entries:
    // a choice, two sliders, a choice).
    static bool suitsVolume(const ui::Menu& menu);

    // Whether `menu` can be shown as the longer list: five to eight
    // entries, all of them actions.
    static bool suitsSelectList(const ui::Menu& menu);

    // Follows `menu` (which must suit `screen`): opens, flips or turns the
    // dial when it changed since the last call. For Volume, `under` is the
    // list menu that stays on screen above the panel. draw() calls it too.
    void show(const ui::Menu& menu, OriginalScreen screen = OriginalScreen::List, const ui::Menu* under = nullptr);

    // Moves the animation on by `seconds`.
    void update(double seconds);

    // Draws `menu` (which must suit `screen`) centred on the canvas,
    // between ui.begin() and ui.end().
    void draw(render::UiRenderer& ui, const ui::Menu& menu, OriginalScreen screen = OriginalScreen::List,
              const ui::Menu* under = nullptr);

    // The next draw starts the opening animation again (call when the list
    // menu was hidden, e.g. by a match).
    void reset() { shown_ = nullptr; }

private:
    void drawPiece(render::UiRenderer& ui, const ui::MenuPiece& piece, float originX);
    void drawPieces(render::UiRenderer& ui, const std::vector<ui::MenuPiece>& pieces, float originX);
    void drawList(render::UiRenderer& ui, const ui::Menu& menu, float originX);
    void drawQuestion(render::UiRenderer& ui, const ui::Menu& menu, float originX);
    void drawVolume(render::UiRenderer& ui, const ui::Menu& menu, float originX);
    void drawVolumePanel(render::UiRenderer& ui, float originX);
    void drawSelectList(render::UiRenderer& ui, const ui::Menu& menu, float originX);

    FrontEndPictures& pictures_;
    const render::UiFont& font_;
    const render::UiFont& questionFont_;
    ui::OriginalMenu animation_;
    ui::OriginalDialog dialog_;
    ui::OriginalVolumePanel volume_;
    ui::OriginalSelectList selectList_;
    const ui::Menu* shown_ = nullptr;  // the menu drawn last time
    OriginalScreen screen_ = OriginalScreen::List;  // ...and how
    std::size_t selected_ = 0;
    bool volumeLeaving_ = false;  // the volume panel drops away under a list
    int masterLevel_ = 0;         // the knobs' levels, kept while leaving
    int effectsLevel_ = 0;
};

} // namespace openhogs::gameapp
