// NameEntry.cpp - see NameEntry.h for an overview.

#include "openhogs/ui/NameEntry.h"

#include <cctype>
#include <cstring>
#include <utility>

namespace openhogs::ui {

namespace {

constexpr std::size_t kKeyRows = 3;  // DELETE, SPACE, END

} // namespace

NameEntry::NameEntry(std::string title, std::string current, std::size_t maxLength)
    : title_(std::move(title)), current_(std::move(current)), maxLength_(maxLength) {
    moveToEnd();
}

std::size_t NameEntry::letterCount() const { return std::strlen(kNameLetters); }

std::size_t NameEntry::rows() const { return (letterCount() + kNameGridColumns - 1) / kNameGridColumns; }

char NameEntry::letterAt(std::size_t column, std::size_t row) const {
    if (column >= kNameGridColumns) return '\0';
    const std::size_t index = row * kNameGridColumns + column;
    return index < letterCount() ? kNameLetters[index] : '\0';
}

bool NameEntry::isCell(std::size_t column, std::size_t row) const {
    if (column == kNameGridColumns) return row < kKeyRows;
    return letterAt(column, row) != '\0';
}

NameEntry::Key NameEntry::keyAtCursor() const {
    if (column_ < kNameGridColumns) return Key::Letter;
    switch (row_) {
    case 0: return Key::Delete;
    case 1: return Key::Space;
    default: return Key::End;
    }
}

void NameEntry::moveToEnd() {
    column_ = kNameGridColumns;
    row_ = kKeyRows - 1;
}

NameEntry::Result NameEntry::handle(MenuInput input) {
    std::size_t column = column_;
    std::size_t row = row_;
    switch (input) {
    case MenuInput::Left:
        if (column == 0) return Result::None;
        --column;
        // From the keys' column, land on the last letter of that row.
        while (column > 0 && !isCell(column, row)) --column;
        break;
    case MenuInput::Right:
        if (column == kNameGridColumns) return Result::None;
        ++column;
        // Past the last letter of a row: the keys' column.
        if (!isCell(column, row)) column = kNameGridColumns;
        if (row >= kKeyRows && column == kNameGridColumns) return Result::None;
        break;
    case MenuInput::Up:
        if (row == 0) return Result::None;
        --row;
        break;
    case MenuInput::Down:
        ++row;
        if (!isCell(column, row)) return Result::None;
        break;
    case MenuInput::Confirm:
        switch (keyAtCursor()) {
        case Key::Letter: return add(letterAt(column_, row_));
        case Key::Space: return add(' ');
        case Key::Delete: return erase();
        case Key::End: return edited_ && name_.empty() ? Result::Refused : Result::Done;
        }
        return Result::None;
    case MenuInput::Back: return Result::Cancelled;
    }
    if (!isCell(column, row)) return Result::None;
    column_ = column;
    row_ = row;
    return Result::Moved;
}

NameEntry::Result NameEntry::type(char letter) {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
    if (upper != ' ' && std::strchr(kNameLetters, upper) == nullptr) return Result::None;
    const Result result = add(upper);
    // As the executable: after a typed letter the cursor waits on END.
    if (result == Result::Changed) moveToEnd();
    return result;
}

NameEntry::Result NameEntry::erase() {
    edited_ = true;
    if (name_.empty()) return Result::Refused;
    name_.pop_back();
    return Result::Changed;
}

NameEntry::Result NameEntry::add(char letter) {
    edited_ = true;
    if (letter == '\0' || name_.size() >= maxLength_) return Result::Refused;
    name_.push_back(letter);
    return Result::Changed;
}

} // namespace openhogs::ui
