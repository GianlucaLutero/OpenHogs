// KeyBindings.cpp - see KeyBindings.h for an overview.

#include "openhogs/ui/KeyBindings.h"

#include "openhogs/core/IniFile.h"
#include "openhogs/core/PathUtils.h"

#include <algorithm>
#include <cctype>

namespace openhogs::ui {

namespace {

struct ControlInfo {
    std::string_view iniKey;
    std::vector<std::string_view> defaultKeys;  // SDL physical key names
};

// One row per Control, in the enum's order.
const std::array<ControlInfo, kControlCount>& controlTable() {
    static const std::array<ControlInfo, kControlCount> table{{
        {"Forward", {"W", "Up"}},
        {"Backward", {"S", "Down"}},
        {"TurnLeft", {"A", "Left"}},
        {"TurnRight", {"D", "Right"}},
        {"Fire", {"Left Ctrl", "X"}},
        {"Jump", {"Space"}},
        {"PreviousWeapon", {"Q"}},
        {"NextWeapon", {"E"}},
        {"Sight", {"V"}},
        {"AimDown", {"F"}},
        {"AimUp", {"R"}},
        {"NextHog", {"Tab"}},
        {"EndTurn", {"Return"}},
        {"Pause", {"P"}},
    }};
    return table;
}

const ControlInfo& info(Control control) {
    return controlTable()[static_cast<std::size_t>(control)];
}

bool sameKey(std::string_view a, std::string_view b) {
    return core::toLower(a) == core::toLower(b);
}

std::string upperCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

} // namespace

std::string_view controlIniKey(Control control) {
    return info(control).iniKey;
}

KeyBindings KeyBindings::defaults() {
    KeyBindings bindings;
    for (std::size_t i = 0; i < kControlCount; ++i) {
        for (const auto key : controlTable()[i].defaultKeys) bindings.keys_[i].emplace_back(key);
    }
    return bindings;
}

KeyBindings KeyBindings::fromIni(const core::IniFile& ini) {
    KeyBindings bindings = defaults();
    for (std::size_t i = 0; i < kControlCount; ++i) {
        // An empty or missing line keeps the defaults: a control without
        // any key would be a trap rather than a choice.
        auto keys = ini.getList("Controls", controlTable()[i].iniKey);
        std::erase_if(keys, [](const std::string& key) { return sameKey(key, kAlwaysPauseKey); });
        if (!keys.empty()) bindings.keys_[i] = std::move(keys);
    }
    return bindings;
}

const std::vector<std::string>& KeyBindings::keys(Control control) const {
    return keys_[static_cast<std::size_t>(control)];
}

bool KeyBindings::bind(Control control, std::string_view key) {
    if (key.empty() || sameKey(key, kAlwaysPauseKey)) {
        return false;
    }
    auto& chosen = keys_[static_cast<std::size_t>(control)];
    const std::vector<std::string> previous = chosen;
    for (auto& keys : keys_) {
        if (&keys == &chosen) continue;
        const auto lost = std::erase_if(keys, [&](const std::string& existing) { return sameKey(existing, key); });
        // A control left without any key takes the chosen control's old
        // one instead: the two swap, so nothing becomes unusable.
        if (lost > 0 && keys.empty() && !previous.empty()) keys.push_back(previous.front());
    }
    chosen = {std::string(key)};
    return true;
}

std::optional<Control> KeyBindings::controlFor(std::string_view key) const {
    for (std::size_t i = 0; i < kControlCount; ++i) {
        for (const auto& existing : keys_[i]) {
            if (sameKey(existing, key)) return static_cast<Control>(i);
        }
    }
    return std::nullopt;
}

std::string KeyBindings::describe(Control control) const {
    const auto& keys = this->keys(control);
    if (keys.empty()) return "-";
    std::string text;
    for (const auto& key : keys) {
        if (!text.empty()) text += " / ";
        text += upperCase(key);
    }
    return text;
}

std::vector<std::pair<std::string, std::string>> KeyBindings::iniValues() const {
    std::vector<std::pair<std::string, std::string>> values;
    for (std::size_t i = 0; i < kControlCount; ++i) {
        std::string line;
        for (const auto& key : keys_[i]) {
            if (!line.empty()) line += ", ";
            line += key;
        }
        values.emplace_back(std::string(controlTable()[i].iniKey), line);
    }
    return values;
}

} // namespace openhogs::ui
