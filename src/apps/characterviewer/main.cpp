// openhogs-characterviewer - shows the hogs of the original game, animated.
//
// Usage:
//   openhogs-characterviewer [options]
//
// Options:
//   --config <file>        OpenHogs configuration (default: config/openhogs.ini)
//   --game <folder>        use this game folder directly, without config or packs
//   --list                 print the classes, teams and animations, then exit
//   --class <id>           hog class: ace, grunt, heavy, legend, medic, sapper,
//                          saboteur, sniper, spy (default: grunt)
//   --team <name>          team textures: british, american, french, german,
//                          japanese, russian, teamlard (default: british)
//   --detail <level>       high, medium or low (default: high)
//   --animation <n>        animation number (default: 0)
//   --frame <n>            start frame (default: 0)
//   --fps <n>              playback speed in frames per second (default: 25)
//   --expression <name>    face: auto (follows the animation, the default),
//                          neutral, happy, sad, shocked, angry, eyes-closed,
//                          dying, gormless
//   --no-blink             do not blink
//   --view <yaw,pitch>     camera angles around the hog, in degrees
//   --distance <units>     camera distance from the hog (default 650)
//   --screenshot <file>    render the chosen frame to a PNG file and exit
//   --size <W>x<H>         window size in pixels (default 1024x768)
//
// Files are read through the content module, so content packs that replace
// models or team textures (config/openhogs.ini) are shown unless --game is used.

#include "CharacterViewer.h"

#include "openhogs/apps/GameFiles.h"
#include "openhogs/core/PathUtils.h"

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
    std::string className = "grunt";
    std::string team = "british";
    std::string detail = "high";
    std::size_t animation = 0;
    float frame = 0.0f;
    float framesPerSecond = 25.0f;
    std::string expression = "auto";
    bool blink = true;
    std::optional<std::pair<float, float>> view;  // yaw, pitch
    std::optional<float> distance;
    bool list = false;
    int width = 1024;
    int height = 768;
};

void printUsage() {
    std::cout << "OpenHogs character viewer\n\n"
                 "Usage: openhogs-characterviewer [options]\n\n"
                 "  --config <file>      configuration file (default: config/openhogs.ini)\n"
                 "  --game <folder>      read this game folder directly (no config, no packs)\n"
                 "  --list               list the classes, teams and animations\n"
                 "  --class <id>         ace, grunt, heavy, legend, medic, sapper, saboteur, sniper, spy\n"
                 "  --team <name>        british, american, french, german, japanese, russian, teamlard\n"
                 "  --detail <level>     high, medium or low\n"
                 "  --animation <n>      animation number (see --list)\n"
                 "  --frame <n>          start frame\n"
                 "  --fps <n>            playback speed (default 25 frames per second)\n"
                 "  --expression <name>  auto, neutral, happy, sad, shocked, angry, eyes-closed, dying, gormless\n"
                 "  --no-blink           do not blink\n"
                 "  --view <yaw,pitch>   camera angles around the hog, in degrees\n"
                 "  --distance <units>   camera distance from the hog (default 650)\n"
                 "  --screenshot <file>  save the chosen frame as a PNG image and exit\n"
                 "  --size <W>x<H>       window size (default 1024x768)\n\n"
                 "Controls: Left/Right animation, Up/Down class, T team, L detail, E face, B blink, Space pause,\n"
                 ", and . step frames, - and + speed, left mouse orbit, wheel zoom, F12 screenshot, Esc quit\n";
}

// Splits "a<separator>b" into two numbers.
std::optional<std::pair<float, float>> parsePair(const std::string& text, char separator) {
    const auto split = text.find(separator);
    if (split == std::string::npos) {
        return std::nullopt;
    }
    try {
        return std::pair(std::stof(text.substr(0, split)), std::stof(text.substr(split + 1)));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Returns std::nullopt when the arguments are wrong.
std::optional<Options> parseArguments(const std::vector<std::string>& args) {
    Options options;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--list") {
                options.list = true;
                continue;
            }
            if (arg == "--no-blink") {
                options.blink = false;
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
            } else if (arg == "--screenshot") {
                options.screenshotPath = value;
            } else if (arg == "--class") {
                options.className = value;
            } else if (arg == "--team") {
                options.team = core::toLower(value);
            } else if (arg == "--detail") {
                options.detail = core::toLower(value);
            } else if (arg == "--animation") {
                options.animation = static_cast<std::size_t>(std::stoul(value));
            } else if (arg == "--frame") {
                options.frame = std::stof(value);
            } else if (arg == "--expression") {
                options.expression = core::toLower(value);
            } else if (arg == "--fps") {
                options.framesPerSecond = std::stof(value);
            } else if (arg == "--view") {
                options.view = parsePair(value, ',');
                if (!options.view) return std::nullopt;
            } else if (arg == "--distance") {
                options.distance = std::stof(value);
            } else if (arg == "--size") {
                const auto size = parsePair(value, 'x');
                if (!size) return std::nullopt;
                options.width = static_cast<int>(size->first);
                options.height = static_cast<int>(size->second);
            } else {
                return std::nullopt;
            }
        }
    } catch (const std::exception&) {
        return std::nullopt;  // a number that does not parse
    }
    return options;
}

void printLists(const scene::CharacterLibrary& library) {
    std::cout << "Classes:\n";
    for (const auto& characterClass : scene::characterClasses()) {
        std::cout << "  " << characterClass.id << " (" << characterClass.name << ")\n";
    }
    std::cout << "Teams:\n";
    for (const auto team : scene::characterTeams()) {
        std::cout << "  " << team << "\n";
    }
    std::cout << "Animations (number: frames, name):\n";
    const auto& animations = library.animations();
    for (std::size_t i = 0; i < animations.size(); ++i) {
        std::cout << "  " << i << ": " << animations[i].frames.size() << " frames  " << scene::animationName(i)
                  << "\n";
    }
}

characterviewer::ViewerState makeStartState(const Options& options, const scene::CharacterLibrary& library) {
    characterviewer::ViewerState state;

    const auto* characterClass = scene::findCharacterClass(options.className);
    if (characterClass == nullptr) {
        throw std::runtime_error("Unknown class '" + options.className + "' (use --list)");
    }
    state.classIndex = static_cast<std::size_t>(characterClass - scene::characterClasses().data());

    const auto teams = scene::characterTeams();
    const auto team = std::find(teams.begin(), teams.end(), options.team);
    if (team == teams.end()) {
        throw std::runtime_error("Unknown team '" + options.team + "' (use --list)");
    }
    state.teamIndex = static_cast<std::size_t>(team - teams.begin());

    if (options.detail == "high") {
        state.detail = scene::CharacterDetail::High;
    } else if (options.detail == "medium") {
        state.detail = scene::CharacterDetail::Medium;
    } else if (options.detail == "low") {
        state.detail = scene::CharacterDetail::Low;
    } else {
        throw std::runtime_error("Unknown detail level '" + options.detail + "' (high, medium or low)");
    }

    if (options.animation >= library.animations().size()) {
        throw std::runtime_error("There are only " + std::to_string(library.animations().size()) +
                                 " animations (0 to " + std::to_string(library.animations().size() - 1) + ")");
    }
    state.animation = options.animation;
    state.frame = options.frame;
    state.framesPerSecond = options.framesPerSecond;
    state.blinking = options.blink;
    if (options.expression != "auto") {
        state.expression = scene::parseExpression(options.expression);
        if (!state.expression) {
            throw std::runtime_error("Unknown expression '" + options.expression + "' (see --help)");
        }
    }
    if (options.distance) {
        state.orbitDistance = *options.distance;
    }
    if (options.view) {
        state.orbitYawDegrees = options.view->first;
        state.orbitPitchDegrees = options.view->second;
    }
    return state;
}

int runViewer(const Options& options) {
    const auto vfs = apps::openGameFiles(options.gameFolder, options.configPath);
    const scene::CharacterLibrary library(vfs);

    if (options.list) {
        printLists(library);
        return 0;
    }

    const auto start = makeStartState(options, library);
    const bool screenshotOnly = options.screenshotPath.has_value();
    characterviewer::CharacterViewer viewer(library, start, options.width, options.height, screenshotOnly);
    if (screenshotOnly) {
        viewer.saveScreenshot(*options.screenshotPath);
        std::cout << "Saved " << options.screenshotPath->string() << "\n";
        return 0;
    }
    viewer.run();
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
        return runViewer(*options);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
