// openhogs - the game.
//
// Usage:
//   openhogs [options]
//
// Without --map the game starts on the main menu.
//
// Options:
//   --config <file>      OpenHogs configuration (default: config/openhogs.ini)
//   --game <folder>      use this game folder directly, without config or packs
//   --map <name>         skip the menus and play this map, e.g. ESTU
//   --list-maps          print the names of the maps, then exit
//   --hog <n>            hog to start with (1 = the first hog of the map; it
//                        must belong to the team that plays first)
//   --no-sound           play without sound
//   --no-intro           skip the start-up videos
//   --video <file>       with --screenshot: show this video at --seconds
//   --seconds <s>        with --screenshot: let the match (or the menu) run this long first
//   --auto-walk          with --screenshot: the active hog runs forward meanwhile
//   --weapon <n>         with --screenshot: the active hog selects its n-th weapon (1 = first)
//   --fire-at <s>        with --screenshot: the active hog fires at s seconds
//   --computer <who>     with --map: "others" = the computer plays every team
//                        but the first, "all" = it plays every team
//   --pause              with --screenshot: show the pause menu
//   --menu-keys <keys>   with --screenshot and no --map: press these menu keys
//                        first, e.g. "down,enter" (up, down, left, right,
//                        enter, back)
//   --screenshot <file>  render the match (or the menu) to a PNG file and exit
//   --size <W>x<H>       window size in pixels (default 1280x720)
//
// Files are read through the content module, so content packs
// (config/openhogs.ini) are applied unless --game is used.

#include "Application.h"

#include "openhogs/apps/GameFiles.h"
#include "openhogs/audio/AudioSettings.h"
#include "openhogs/core/IniFile.h"
#include "openhogs/scene/MapScene.h"

// SDL_main.h turns main() into the right entry point on every platform
// (e.g. WinMain on Windows). It must be included in this file only.
#include <SDL3/SDL_main.h>

#include <algorithm>
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
    std::optional<std::size_t> weaponSlot;  // 1-based, as typed
    std::optional<double> fireAt;
    bool pause = false;
    bool intro = true;
    std::optional<std::string> video;  // with --screenshot: a video to show
    std::string computer;  // "", "others" or "all"
    std::vector<ui::MenuInput> menuKeys;
    int width = 1280;
    int height = 720;
};

void printUsage() {
    std::cout << "OpenHogs\n\n"
                 "Usage: openhogs [options]\n\n"
                 "  --config <file>      configuration file (default: config/openhogs.ini)\n"
                 "  --game <folder>      read this game folder directly (no config, no packs)\n"
                 "  --map <name>         skip the menus and play this map, e.g. ESTU\n"
                 "  --list-maps          list the maps\n"
                 "  --hog <n>            hog to start with (1 = first; must be in the team that plays first)\n"
                 "  --no-sound           play without sound\n"
                 "  --no-intro           skip the start-up videos\n"
                 "  --video <file>       with --screenshot: show this video (e.g. Language/Streams/sheff.bik)\n"
                 "  --seconds <s>        with --screenshot: let the match (or the menu) run this long first\n"
                 "  --auto-walk          with --screenshot: the active hog runs forward meanwhile\n"
                 "  --weapon <n>         with --screenshot: the active hog selects its n-th weapon\n"
                 "  --fire-at <s>        with --screenshot: the active hog fires at s seconds\n"
                 "  --computer <who>     with --map: others (computer plays all teams but the first) or all\n"
                 "  --pause              with --screenshot: show the pause menu\n"
                 "  --menu-keys <keys>   with --screenshot and no --map: press these menu keys first,\n"
                 "                       e.g. down,enter (up, down, left, right, enter, back)\n"
                 "  --screenshot <file>  save the match (or the menu) as a PNG image and exit\n"
                 "  --size <W>x<H>       window size (default 1280x720)\n\n"
                 "Menus: arrows or W/A/S/D move, Enter picks, Esc goes back.\n"
                 "Match: W/S or Up/Down run / walk back, A/D or Left/Right turn, Space jump,\n"
                 "Q/E weapon, R/F aim, Ctrl or X fire (hold to throw further),\n"
                 "Tab other hog (before moving), Enter end turn, left mouse turns the camera,\n"
                 "wheel zooms, Esc or P pause menu, F12 screenshot\n";
}

// "down,down,enter" -> the menu inputs; std::nullopt for an unknown key.
std::optional<std::vector<ui::MenuInput>> parseMenuKeys(const std::string& text) {
    std::vector<ui::MenuInput> inputs;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto comma = std::min(text.find(',', start), text.size());
        const std::string key = text.substr(start, comma - start);
        if (key == "up") inputs.push_back(ui::MenuInput::Up);
        else if (key == "down") inputs.push_back(ui::MenuInput::Down);
        else if (key == "left") inputs.push_back(ui::MenuInput::Left);
        else if (key == "right") inputs.push_back(ui::MenuInput::Right);
        else if (key == "enter") inputs.push_back(ui::MenuInput::Confirm);
        else if (key == "back") inputs.push_back(ui::MenuInput::Back);
        else return std::nullopt;
        start = comma + 1;
    }
    return inputs;
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
            if (arg == "--pause") {
                options.pause = true;
                continue;
            }
            if (arg == "--no-intro") {
                options.intro = false;
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
            } else if (arg == "--weapon") {
                options.weaponSlot = static_cast<std::size_t>(std::stoul(value));
                if (*options.weaponSlot == 0) return std::nullopt;
            } else if (arg == "--fire-at") {
                options.fireAt = std::stod(value);
            } else if (arg == "--seconds") {
                options.seconds = std::stod(value);
            } else if (arg == "--video") {
                options.video = value;
            } else if (arg == "--screenshot") {
                options.screenshotPath = value;
            } else if (arg == "--computer") {
                if (value != "others" && value != "all") return std::nullopt;
                options.computer = value;
            } else if (arg == "--menu-keys") {
                const auto keys = parseMenuKeys(value);
                if (!keys) return std::nullopt;
                options.menuKeys = *keys;
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

    const bool screenshotOnly = options.screenshotPath.has_value();

    gameapp::ApplicationOptions appOptions;
    appOptions.width = options.width;
    appOptions.height = options.height;
    appOptions.hiddenWindow = screenshotOnly;
    appOptions.sound = options.sound && !screenshotOnly;
    appOptions.maps = maps;
    // The start-up videos, unless going straight to a match or a screenshot.
    appOptions.intro = options.intro && !screenshotOnly && !options.mapName;
    // The campaign is saved next to the configuration folder: saves/campaign.ini.
    appOptions.savePath = options.configPath.parent_path().parent_path() / "saves" / "campaign.ini";
    if (!options.gameFolder) {
        appOptions.volumes = audio::volumesFromIni(core::IniFile::load(options.configPath));
    }
    gameapp::Application app(vfs, appOptions);

    if (options.mapName) {
        gameapp::GameOptions gameOptions;
        gameOptions.mapName = *options.mapName;
        if (options.hog) {
            gameOptions.startHog = *options.hog - 1;
        }
        if (!options.computer.empty()) {
            auto teams = game::mapTeams(vfs, *options.mapName);
            if (options.computer == "others" && !teams.empty()) {
                teams.erase(teams.begin());
            }
            gameOptions.computerTeams = teams;
        }
        if (screenshotOnly) {
            gameOptions.autoWalk = options.autoWalk;
            gameOptions.autoFireAt = options.fireAt;
            if (options.weaponSlot) gameOptions.autoWeaponSlot = *options.weaponSlot - 1;
        }
        app.startMatch(gameOptions);
    }

    if (!screenshotOnly) {
        app.run();
        return 0;
    }
    if (options.video) {
        app.playCutscenes({*options.video});
        app.advanceCutscenes(options.seconds);
    } else if (auto* game = app.game()) {
        game->simulate(options.seconds);
        if (options.pause) game->pause();
    } else {
        app.sendMenuInputs(options.menuKeys);
        if (auto* started = app.game()) {
            started->simulate(options.seconds);
        } else {
            app.advanceMenus(options.seconds);
        }
    }
    app.saveScreenshot(*options.screenshotPath);
    std::cout << "Saved " << options.screenshotPath->string() << "\n";
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
