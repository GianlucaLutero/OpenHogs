// Tests for finding the configuration file (content/ConfigLocation.h).
// They protect what makes a released package usable at once: the
// settings are found next to the program wherever it is started from,
// openhogs.ini is made from the default the first time, and an existing
// openhogs.ini (the user's own settings) is never overwritten.

#include "openhogs/content/ConfigLocation.h"

#include "TempDirectory.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>

using namespace openhogs::content;
using openhogs::tests::TempDirectory;
namespace fs = std::filesystem;

namespace {

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path) << text;
}

std::string readText(const fs::path& path) {
    std::ifstream in(path);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

} // namespace

TEST_CASE("The first run makes openhogs.ini from the default; later runs keep the user's", "[content][config]") {
    TempDirectory folder;
    const fs::path config = folder.path() / "config" / "openhogs.ini";
    writeText(folder.path() / "config" / "openhogs.default.ini", "[Game]\nDirectory = default\n");

    const auto first = locateConfig(config, std::nullopt);
    CHECK(first.createdFromDefault);
    CHECK(readText(config) == "[Game]\nDirectory = default\n");

    writeText(config, "[Game]\nDirectory = mine\n");
    const auto second = locateConfig(config, std::nullopt);
    CHECK_FALSE(second.createdFromDefault);
    CHECK(readText(config) == "[Game]\nDirectory = mine\n");
}

TEST_CASE("A relative configuration path is also looked up next to the program", "[content][config]") {
    // Started from another folder (or by double-clicking), the program
    // still finds the config/ folder beside it.
    TempDirectory program;
    TempDirectory elsewhere;  // the current folder, without a config/
    writeText(program.path() / "config" / "openhogs.ini", "[Game]\n");
    const fs::path before = fs::current_path();
    fs::current_path(elsewhere.path());
    const auto found = locateConfig("config/openhogs.ini", program.path());
    fs::current_path(before);
    CHECK(found.path == program.path() / "config" / "openhogs.ini");
}
