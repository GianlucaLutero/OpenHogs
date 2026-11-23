// NameEntry.h - typing a name the way the original's front end does it.
//
// PLEASE NAME YOUR TEAM and PLEASE NAME YOUR PIG (screens 18 and 13 of the
// executable, drawn on the information board) show the letters the game's
// font has, in a grid, with three keys after them: DELETE (the last
// letter), SPACE and END (done). The player moves over the grid and picks
// letters, or types them on the keyboard: a typed letter goes straight into
// the name and the cursor goes back to END, so Enter then finishes
// (FUN_0042af50, FUN_0042e440).
//
// The name starts empty: END before anything is typed keeps the old name
// (the executable copies its buffer only once a key was used), while END
// on a name typed and then wiped out is refused.
//
// The executable's limits: 13 letters for a team, 7 for a hog.
//
// This class holds the name and the cursor; it draws nothing.

#pragma once

#include "openhogs/ui/Menu.h"

#include <cstddef>
#include <string>

namespace openhogs::ui {

inline constexpr std::size_t kTeamNameLength = 13;
inline constexpr std::size_t kHogNameLength = 7;

// The letters of the game's font, in the order of its text table (fetext 0).
inline constexpr const char* kNameLetters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ-*&'+,0123456789";

// Letters per row of the grid; the three keys stand in a column after it.
inline constexpr std::size_t kNameGridColumns = 11;

class NameEntry {
public:
    enum class Key { Letter, Delete, Space, End };

    enum class Result {
        None,
        Moved,      // the cursor moved
        Changed,    // a letter was added or removed
        Refused,    // a letter that does not fit, or END with no name
        Done,       // END: the name is finished
        Cancelled,  // Back: leave without changing anything
    };

    NameEntry() = default;
    // `current`: the name now, kept if nothing is typed (shown meanwhile).
    NameEntry(std::string title, std::string current, std::size_t maxLength);

    const std::string& title() const { return title_; }
    const std::string& name() const { return name_; }
    const std::string& current() const { return current_; }
    // Whether anything was typed: only then does the name replace the old one.
    bool edited() const { return edited_; }
    std::size_t maxLength() const { return maxLength_; }

    // The grid: rows of letters, and the keys' column at kNameGridColumns.
    std::size_t letterCount() const;
    std::size_t rows() const;
    // The cursor, as a column and a row; the keys are at column
    // kNameGridColumns, rows 0 (DELETE), 1 (SPACE) and 2 (END).
    std::size_t column() const { return column_; }
    std::size_t row() const { return row_; }
    Key keyAtCursor() const;
    char letterAt(std::size_t column, std::size_t row) const;  // '\0' outside the letters

    // Moves over the grid or picks what is under the cursor.
    Result handle(MenuInput input);
    // A key typed on the keyboard: a letter of the grid (lower case is
    // taken as upper case) or a space goes into the name.
    Result type(char letter);
    // Backspace on the keyboard: removes the last letter.
    Result erase();

private:
    Result add(char letter);
    void moveToEnd();
    bool isCell(std::size_t column, std::size_t row) const;

    std::string title_;
    std::string name_;
    std::string current_;
    bool edited_ = false;
    std::size_t maxLength_ = kTeamNameLength;
    std::size_t column_ = kNameGridColumns;
    std::size_t row_ = 2;  // END
};

} // namespace openhogs::ui
