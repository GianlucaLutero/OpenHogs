// openhogs - the game.
//
// Usage:
//   openhogs [options]
//
// Options:
//   --config <file>      OpenHogs configuration (default: config/openhogs.ini)
//   --game <folder>      use this game folder directly, without config or packs
//   --map <name>         map to play, e.g. ESTU (default: the first map)
//   --list-maps          print the names of the maps, then exit
//   --hog <n>            hog to start with (1 = the first hog of the map; it
//                        must belong to the team that plays first)
//   --no-sound           play without sound
//   --seconds <s>        with --screenshot: let the match run this long first
//   --auto-walk          with --screenshot: the active hog runs forward meanwhile
//   --screenshot <file>  render the match to a PNG file and exit
//   --size <W>x<H>       window size in pixels (default 1280x720)
//
// Files are read through the content module, so content packs
// (config/openhogs.ini) are applied unless --game is used.

#include "Game.h"

#include "openhogs/apps/GameFiles.h"
#include "openhogs/audio/AudioSettings.h"
#include "openhogs/core/IniFile.h"
#include "openhogs/scene/MapScene.h"

// SDL_main.h turns main() into the right entry point on every platform
// (e.g. WinMain on Windows). It must be included in this file only.
#include <SDL3/SDL_main.h>

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace openhogs;

namespace {

struct Options {
    std::filesystem::path configPath = "config/openhogs.ini";
    std::optional<std::filesystem::path> gameFolder;
    std::optional<std::filesystem::path> screenshotPath;
    std::optional<std::string> mapName;
    std::optional<std::size_t> hog;  // 1-based, as typed
    double seconds = 0.0;
    bool listMaps = false;
    bool sound = true;
    bool autoWalk = false;
    int width = 1280;
    int height = 720;
};

void printUsage() {
    std::cout << "OpenHogs\n\n"
                 "Usage: openhogs [options]\n\n"
                 "  --config <file>      configuration file (default: config/openhogs.ini)\n"
                 "  --game <folder>      read this game folder directly (no config, no packs)\n"
                 "  --map <name>         map to play, e.g. ESTU (default: the first map)\n"
                 "  --list-maps          list the maps\n"
                 "  --hog <n>            hog to start with (1 = first; must be in the team that plays first)\n"
                 "  --no-sound           play without sound\n"
                 "  --seconds <s>        with --screenshot: let the match run this long first\n"
                 "  --auto-walk          with --screenshot: the active hog runs forward meanwhile\n"
                 "  --screenshot <file>  save the match as a PNG image and exit\n"
                 "  --size <W>x<H>       window size (default 1280x720)\n\n"
                 "Controls: W/S or Up/Down run / walk back, A/D or Left/Right turn, Space jump,\n"
                 "Tab other hog (before moving), Enter end turn, left mouse turns the camera,\n"
                 "wheel zooms, P pause, F12 screenshot, Esc quit\n";
}

bool parseSize(const std::string& text, int& width, int& height) {
    const auto split = text.find('x');
    if (split == std::string::npos) {
        return false;
    }
    width = std::stoi(text.substr(0, split));
    height = std::stoi(text.substr(split + 1));
    return width > 0 && height > 0;
}

// Returns std::nullopt when the arguments are wrong.
std::optional<Options> parseArguments(const std::vector<std::string>& args) {
    Options options;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--list-maps") {
                options.listMaps = true;
                continue;
            }
            if (arg == "--no-sound") {
                options.sound = false;
                continue;
            }
            if (arg == "--auto-walk") {
                options.autoWalk = true;
                continue;
            }
            if (i + 1 >= args.size()) {
                return std::nullopt;
            }
            const std::string& value = args[++i];
            if (arg == "--config") {
                options.configPath = value;
            } else if (arg == "--game") {
                options.gameFolder = value;
            } else if (arg == "--map") {
                options.mapName = value;
            } else if (arg == "--hog") {
                options.hog = static_cast<std::size_t>(std::stoul(value));
                if (*options.hog == 0) return std::nullopt;
            } else if (arg == "--seconds") {
                options.seconds = std::stod(value);
            } else if (arg == "--screenshot") {
                options.screenshotPath = value;
            } else if (arg == "--size") {
                if (!parseSize(value, options.width, options.height)) return std::nullopt;
            } else {
                return std::nullopt;
            }
        }
    } catch (const std::exception&) {
        return std::nullopt;  // a number that does not parse
    }
    return options;
}

int runGame(const Options& options) {
    const auto vfs = apps::openGameFiles(options.gameFolder, options.configPath);
    const auto maps = scene::listMaps(vfs);
    if (maps.empty()) {
        throw std::runtime_error("No maps found: is the game folder correct?");
    }
    if (options.listMaps) {
        for (const auto& map : maps) {
            std::cout << map << "\n";
        }
        return 0;
    }

    gameapp::GameOptions gameOptions;
    gameOptions.mapName = options.mapName.value_or(maps.front());
    gameOptions.width = options.width;
    gameOptions.height = options.height;
    if (options.hog) {
        gameOptions.startHog = *options.hog - 1;
    }
    if (!options.gameFolder) {
        gameOptions.volumes = audio::volumesFromIni(core::IniFile::load(options.configPath));
    }

    const bool screenshotOnly = options.screenshotPath.has_value();
    gameOptions.hiddenWindow = screenshotOnly;
    gameOptions.sound = options.sound && !screenshotOnly;
    gameOptions.autoWalk = options.autoWalk && screenshotOnly;

    gameapp::Game game(vfs, gameOptions);
    if (screenshotOnly) {
        game.simulate(options.seconds);
        game.saveScreenshot(*options.screenshotPath);
        std::cout << "Saved " << options.screenshotPath->string() << "\n";
        return 0;
    }
    game.run();
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const auto options = parseArguments(args);
    if (!options || (!args.empty() && args[0] == "--help")) {
        printUsage();
        return options ? 0 : 1;
    }

    try {
        return runGame(*options);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
