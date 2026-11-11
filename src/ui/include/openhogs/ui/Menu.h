// Menu.h
//
// One menu screen: a title and a vertical list of items the player moves
// through with up/down and picks with confirm. An item is an action
// ("START GAME"), a slider ("MASTER" volume) or a choice between a few
// words ("PLAYER" / "CPU"); sliders and choices change with left/right.
//
// The menu only keeps the selection and slider values and tells the caller
// what happened; what an item does is up to the screen that owns the menu
// (see FrontEnd and PauseMenu). That keeps it reusable and testable.

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace openhogs::ui {

// The player's commands in a menu, whatever key or button produced them.
enum class MenuInput { Up, Down, Left, Right, Confirm, Back };

struct MenuItem {
    enum class Kind { Action, Slider, Choice };

    int id = 0;  // chosen by the owner, returned in MenuResponse
    std::string label;
    Kind kind = Kind::Action;
    bool enabled = true;  // disabled items are shown but skipped
    int value = 0;        // sliders: 0 .. maxValue; choices: index into `choices`
    int maxValue = 10;
    std::vector<std::string> choices;  // choices: the words to pick from
    std::string detail;                // actions: extra text shown on the right (a rank, a cost)

    // The word a choice shows; empty for other items.
    std::string choiceText() const;
};

struct MenuResponse {
    enum class Type {
        None,       // nothing changed (e.g. up on a one-item menu)
        Moved,      // the selection moved
        Changed,    // a slider or choice changed; see Menu::selectedItem()
        Activated,  // confirm on an action
        Back,       // the player wants to leave this menu
    };
    Type type = Type::None;
    int itemId = 0;  // the item concerned (Moved: the newly selected one)
};

class Menu {
public:
    Menu() = default;
    Menu(std::string title, std::vector<MenuItem> items);

    MenuResponse handle(MenuInput input);

    const std::string& title() const { return title_; }
    void setTitle(std::string title) { title_ = std::move(title); }
    const std::vector<MenuItem>& items() const { return items_; }
    std::size_t selectedIndex() const { return selected_; }
    const MenuItem& selectedItem() const { return items_.at(selected_); }

    // The item with this id. Throws std::out_of_range if there is none.
    const MenuItem& item(int id) const;
    void setValue(int id, int value);

    // Selects the item with this id (if it is enabled).
    void select(int id);

    // First item to show when only `rows` fit, chosen so the selected item
    // is always visible (long lists such as the maps scroll).
    std::size_t firstVisible(std::size_t rows) const;

private:
    std::size_t indexOf(int id) const;  // throws std::out_of_range
    bool moveSelection(int direction);

    std::string title_;
    std::vector<MenuItem> items_;
    std::size_t selected_ = 0;
};

} // namespace openhogs::ui
