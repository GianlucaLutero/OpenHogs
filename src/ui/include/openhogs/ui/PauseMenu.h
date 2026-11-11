// PauseMenu.h
//
// The menu over a paused match ("-GAME PAUSED-"):
//
//     CONTINUE        back to the match (Esc does the same)
//     ABORT SKIRMISH  -> ARE YOU SURE?  YES: leave the match / NO: back
//
// Like FrontEnd it only decides; the game pauses, resumes or ends the
// match when told to.

#pragma once

#include "openhogs/ui/FrontEnd.h"  // MenuSound
#include "openhogs/ui/Menu.h"
#include "openhogs/ui/UiStrings.h"

namespace openhogs::ui {

struct PauseResult {
    enum class Request { None, Resume, Abort };
    Request request = Request::None;
    MenuSound sound = MenuSound::None;
};

class PauseMenu {
public:
    explicit PauseMenu(const UiStrings& strings);

    // Shows the first page again (called each time the game pauses).
    void reset();

    PauseResult handle(MenuInput input);

    const Menu& menu() const { return confirming_ ? confirm_ : main_; }
    bool confirming() const { return confirming_; }

private:
    Menu main_;
    Menu confirm_;
    bool confirming_ = false;
};

} // namespace openhogs::ui
