// openhogs-mapviewer - shows the maps of the original game in 3D.
//
// Usage:
//   openhogs-mapviewer [options] [MAP]
//
// Options:
//   --config <file>      OpenHogs configuration (default: config/openhogs.ini)
//   --game <folder>      use this game folder directly, without config or packs
//   --list               print the names of all maps and exit
//   --screenshot <file>  render MAP to a PNG file and exit (no window shown)
//   --size <W>x<H>       window size in pixels (default 1280x720)
//   --camera <x,y,z,yaw,pitch>  start position and angles of the camera
//
// Files are read through the content module, so active content packs
// (config/openhogs.ini) are applied unless --game is used.

#include "MapViewer.h"

#include "openhogs/apps/GameFiles.h"
#include "openhogs/core/PathUtils.h"
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
    std::string mapName;
    std::optional<std::vector<float>> camera;  // x, y, z, yaw, pitch
    bool listMaps = false;
    int width = 1280;
    int height = 720;
};

void printUsage() {
    std::cout << "OpenHogs map viewer\n\n"
                 "Usage: openhogs-mapviewer [options] [MAP]\n\n"
                 "  --config <file>      configuration file (default: config/openhogs.ini)\n"
                 "  --game <folder>      read this game folder directly (no config, no packs)\n"
                 "  --list               list the available maps\n"
                 "  --screenshot <file>  save MAP as a PNG image and exit\n"
                 "  --size <W>x<H>       window size (default 1280x720)\n"
                 "  --camera <x,y,z,yaw,pitch>  camera position and angles (default: whole map)\n\n"
                 "Controls: WASD move, Q/E down/up, Shift faster, right mouse or arrows look,\n"
                 "PageUp/PageDown change map, O objects, M markers, F frame map, F12 screenshot, Esc quit\n";
}

// Returns std::nullopt (after printing the usage) when the arguments are wrong.
std::optional<Options> parseArguments(const std::vector<std::string>& args) {
    Options options;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        const bool hasValue = i + 1 < args.size();

        if (arg == "--config" && hasValue) {
            options.configPath = args[++i];
        } else if (arg == "--game" && hasValue) {
            options.gameFolder = args[++i];
        } else if (arg == "--screenshot" && hasValue) {
            options.screenshotPath = args[++i];
        } else if (arg == "--size" && hasValue) {
            const std::string& size = args[++i];
            const auto x = size.find('x');
            try {
                options.width = std::stoi(size.substr(0, x));
                options.height = std::stoi(size.substr(x + 1));
            } catch (const std::exception&) {
                return std::nullopt;
            }
        } else if (arg == "--camera" && hasValue) {
            // Five comma-separated numbers, e.g. "0,4000,8000,180,-30".
            std::vector<float> values;
            std::string text = args[++i];
            try {
                std::size_t start = 0;
                while (start <= text.size()) {
                    auto comma = text.find(',', start);
                    if (comma == std::string::npos) comma = text.size();
                    values.push_back(std::stof(text.substr(start, comma - start)));
                    start = comma + 1;
                }
            } catch (const std::exception&) {
                return std::nullopt;
            }
            if (values.size() != 5) {
                return std::nullopt;
            }
            options.camera = values;
        } else if (arg == "--list") {
            options.listMaps = true;
        } else if (!arg.starts_with("--") && options.mapName.empty()) {
            options.mapName = arg;
        } else {
            return std::nullopt;
        }
    }
    return options;
}

int runViewer(const Options& options) {
    const auto vfs = apps::openGameFiles(options.gameFolder, options.configPath);
    const auto maps = scene::listMaps(vfs);
    if (maps.empty()) {
        throw std::runtime_error("No maps found: is the game folder correct?");
    }

    if (options.listMaps) {
        for (const auto& name : maps) {
            std::cout << name << "\n";
        }
        return 0;
    }

    std::size_t start = 0;
    if (!options.mapName.empty()) {
        const std::string wanted = core::toLower(options.mapName);
        const auto it = std::find_if(maps.begin(), maps.end(),
                                     [&](const std::string& name) { return core::toLower(name) == wanted; });
        if (it == maps.end()) {
            throw std::runtime_error("Unknown map '" + options.mapName + "' (use --list to see all maps)");
        }
        start = static_cast<std::size_t>(it - maps.begin());
    }

    const bool screenshotOnly = options.screenshotPath.has_value();
    mapviewer::MapViewer viewer(vfs, maps, start, options.width, options.height, screenshotOnly);
    if (options.camera) {
        const auto& c = *options.camera;
        viewer.setCamera({c[0], c[1], c[2]}, c[3], c[4]);
    }
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
