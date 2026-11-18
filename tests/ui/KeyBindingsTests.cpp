// Tests for the player's key bindings (ui::KeyBindings): the CONTROLS
// screen and the [Controls] section of openhogs.ini.
//
// A wrong binding can leave a player unable to fire, end a turn or reach
// the pause menu, so these protect the rules that keep every control
// usable.

#include "openhogs/core/IniFile.h"
#include "openhogs/ui/KeyBindings.h"

#include <catch2/catch_test_macros.hpp>

using namespace openhogs;
using namespace openhogs::ui;

TEST_CASE("Keys written in [Controls] replace a control's defaults", "[ui][keys]") {
    // A hand-edited file must be honoured line by line; controls it does
    // not mention keep working with their default keys, and Escape (which
    // always pauses) cannot be taken.
    const auto ini = core::IniFile::parse("[Controls]\nFire = Space, Escape\nJump =\n");
    const auto bindings = KeyBindings::fromIni(ini);

    CHECK(bindings.keys(Control::Fire) == std::vector<std::string>{"Space"});
    CHECK(bindings.keys(Control::Jump) == std::vector<std::string>{"Space"});  // empty line: default
    CHECK(bindings.keys(Control::Forward) == std::vector<std::string>{"W", "Up"});
}

TEST_CASE("A key given to a control is taken from the others", "[ui][keys]") {
    // One key must never do two things. A control that loses its only key
    // takes the other's old one, so no control becomes unusable.
    auto bindings = KeyBindings::defaults();
    REQUIRE(bindings.bind(Control::Fire, "space"));

    CHECK(bindings.keys(Control::Fire) == std::vector<std::string>{"space"});
    CHECK(bindings.controlFor("Space") == Control::Fire);
    CHECK(bindings.keys(Control::Jump) == std::vector<std::string>{"Left Ctrl"});  // swapped

    REQUIRE(bindings.bind(Control::Forward, "Down"));
    CHECK(bindings.keys(Control::Backward) == std::vector<std::string>{"S"});  // still has S

    CHECK_FALSE(bindings.bind(Control::Jump, "Escape"));  // the pause key stays free
}

TEST_CASE("Saved bindings read back the same", "[ui][keys]") {
    // What the CONTROLS screen saves must give the same keys at the next
    // start.
    auto bindings = KeyBindings::defaults();
    bindings.bind(Control::EndTurn, "Keypad Enter");
    std::string text;
    for (const auto& [key, value] : bindings.iniValues()) text = core::setIniValue(text, "Controls", key, value);

    const auto read = KeyBindings::fromIni(core::IniFile::parse(text));
    for (std::size_t i = 0; i < kControlCount; ++i) {
        CHECK(read.keys(static_cast<Control>(i)) == bindings.keys(static_cast<Control>(i)));
    }
    CHECK(read.describe(Control::EndTurn) == "KEYPAD ENTER");
}
