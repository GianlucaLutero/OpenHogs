// Menu.cpp - see Menu.h for an overview.

#include "openhogs/ui/Menu.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace openhogs::ui {

Menu::Menu(std::string title, std::vector<MenuItem> items) : title_(std::move(title)), items_(std::move(items)) {
    // Start on the first item that can be picked.
    const auto first = std::find_if(items_.begin(), items_.end(), [](const MenuItem& item) { return item.enabled; });
    selected_ = first == items_.end() ? 0 : static_cast<std::size_t>(first - items_.begin());
}

MenuResponse Menu::handle(MenuInput input) {
    if (input == MenuInput::Back) {
        return {MenuResponse::Type::Back, 0};
    }
    if (items_.empty()) {
        return {};
    }
    switch (input) {
    case MenuInput::Up:
    case MenuInput::Down:
        if (moveSelection(input == MenuInput::Down ? 1 : -1)) {
            return {MenuResponse::Type::Moved, selectedItem().id};
        }
        return {};

    case MenuInput::Left:
    case MenuInput::Right: {
        MenuItem& item = items_[selected_];
        if (item.kind != MenuItem::Kind::Slider) {
            return {};
        }
        const int value = std::clamp(item.value + (input == MenuInput::Right ? 1 : -1), 0, item.maxValue);
        if (value == item.value) {
            return {};
        }
        item.value = value;
        return {MenuResponse::Type::Changed, item.id};
    }

    case MenuInput::Confirm:
        if (selectedItem().kind == MenuItem::Kind::Action && selectedItem().enabled) {
            return {MenuResponse::Type::Activated, selectedItem().id};
        }
        return {};

    case MenuInput::Back:
        break;  // handled above
    }
    return {};
}

bool Menu::moveSelection(int direction) {
    // The selection wraps around (down on the last item goes to the first)
    // and skips disabled items. Gives up after a full lap.
    const auto count = static_cast<int>(items_.size());
    int index = static_cast<int>(selected_);
    for (int step = 0; step < count - 1; ++step) {
        index = (index + direction + count) % count;
        if (items_[static_cast<std::size_t>(index)].enabled) {
            selected_ = static_cast<std::size_t>(index);
            return true;
        }
    }
    return false;
}

std::size_t Menu::indexOf(int id) const {
    const auto found = std::find_if(items_.begin(), items_.end(), [id](const MenuItem& item) { return item.id == id; });
    if (found == items_.end()) {
        throw std::out_of_range("Menu \"" + title_ + "\" has no item " + std::to_string(id));
    }
    return static_cast<std::size_t>(found - items_.begin());
}

const MenuItem& Menu::item(int id) const {
    return items_[indexOf(id)];
}

void Menu::setValue(int id, int value) {
    MenuItem& item = items_[indexOf(id)];
    item.value = std::clamp(value, 0, item.maxValue);
}

void Menu::select(int id) {
    const std::size_t index = indexOf(id);
    if (items_[index].enabled) {
        selected_ = index;
    }
}

std::size_t Menu::firstVisible(std::size_t rows) const {
    if (rows == 0 || items_.size() <= rows) {
        return 0;
    }
    // Keep the selection in the middle where possible.
    const std::size_t half = rows / 2;
    const std::size_t first = selected_ > half ? selected_ - half : 0;
    return std::min(first, items_.size() - rows);
}

} // namespace openhogs::ui
