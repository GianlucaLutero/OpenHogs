// openhogs-soundtest - plays the game's sounds and music.
//
// Usage:
//   openhogs-soundtest [--config <file> | --game <folder>] <what to play> [--render <out.wav>]
//
// What to play (can be combined, everything starts together):
//   --play <file>                a sound file, e.g. Audio/L_MGUN.wav or MUSIC/Track05.ogg
//   --bank <file.srl> --sound <n> sound number n of a sound bank
//   --music <n>                  music track n (2 to 32)
// Listing:
//   --list-banks                 every sound bank with its numbered sounds
//   --list-music                 the music tracks
// Output:
//   --render <out.wav>           write the mix to a WAV file instead of the speakers
//   --seconds <s>                stop after s seconds (default: when everything ends)
//
// Volumes come from the [Audio] section of the configuration.

#include "openhogs/audio/AudioDevice.h"
#include "openhogs/audio/AudioSettings.h"
#include "openhogs/audio/Mixer.h"
#include "openhogs/audio/SoundLibrary.h"
#include "openhogs/content/ContentManager.h"
#include "openhogs/core/IniFile.h"
#include "openhogs/core/PathUtils.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace openhogs;

namespace {

constexpr std::uint32_t kOutputRate = 48000;

struct Options {
    fs::path configPath = "config/openhogs.ini";
    std::optional<fs::path> gameFolder;
    std::vector<std::string> files;  // --play
    std::optional<std::string> bank;
    std::vector<std::size_t> bankSounds;  // --sound
    std::vector<int> musicTracks;  // --music
    std::optional<fs::path> renderPath;
    std::optional<double> seconds;
    bool listBanks = false;
    bool listMusic = false;
};

void printUsage() {
    std::cout << "OpenHogs sound test\n\n"
                 "Usage: openhogs-soundtest [--config <file> | --game <folder>] <what> [output]\n\n"
                 "What to play (combinable):\n"
                 "  --play <file>                  e.g. Audio/L_MGUN.wav, Speech/Sku1/Sarge/SGEN0101.wav\n"
                 "  --bank <file.srl> --sound <n>  sound n of a bank, e.g. --bank Audio/sfxday.srl --sound 39\n"
                 "  --music <n>                    music track 2..32\n"
                 "  --list-banks | --list-music    list what is available\n\n"
                 "Output:\n"
                 "  --render <out.wav>             write the mix to a WAV file instead of the speakers\n"
                 "  --seconds <s>                  stop after s seconds\n";
}

std::optional<Options> parseArguments(const std::vector<std::string>& args) {
    Options options;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            const bool hasValue = i + 1 < args.size();
            if (arg == "--list-banks") {
                options.listBanks = true;
            } else if (arg == "--list-music") {
                options.listMusic = true;
            } else if (!hasValue) {
                return std::nullopt;
            } else if (arg == "--config") {
                options.configPath = args[++i];
            } else if (arg == "--game") {
                options.gameFolder = args[++i];
            } else if (arg == "--play") {
                options.files.push_back(args[++i]);
            } else if (arg == "--bank") {
                options.bank = args[++i];
            } else if (arg == "--sound") {
                options.bankSounds.push_back(static_cast<std::size_t>(std::stoul(args[++i])));
            } else if (arg == "--music") {
                options.musicTracks.push_back(std::stoi(args[++i]));
            } else if (arg == "--render") {
                options.renderPath = args[++i];
            } else if (arg == "--seconds") {
                options.seconds = std::stod(args[++i]);
            } else {
                return std::nullopt;
            }
        }
    } catch (const std::exception&) {
        return std::nullopt;  // a number that does not parse
    }
    if (!options.bankSounds.empty() && !options.bank) {
        return std::nullopt;
    }
    return options;
}

// Speech and music get their own volume settings; everything else is an effect.
audio::SoundCategory categoryForPath(const std::string& path) {
    const std::string key = core::makeLookupKey(path);
    if (key.starts_with("speech/")) return audio::SoundCategory::Speech;
    if (key.starts_with("music/")) return audio::SoundCategory::Music;
    return audio::SoundCategory::Effects;
}

void listBanks(audio::SoundLibrary& library) {
    for (const auto& path : library.soundBanks()) {
        const auto bank = library.loadBank(path);
        std::cout << path << " (" << bank.name << ", " << bank.files.size() << " sounds)\n";
        for (std::size_t i = 0; i < bank.files.size(); ++i) {
            std::cout << "  " << i << "  " << bank.files[i] << "\n";
        }
    }
}

void listMusic(audio::SoundLibrary& library) {
    for (const int track : library.musicTracks()) {
        std::cout << track << "  " << audio::SoundLibrary::musicTrackPath(track) << "\n";
    }
}

// Starts every requested sound and returns the length of the longest one.
double startSounds(const Options& options, audio::SoundLibrary& library, audio::Mixer& mixer) {
    double longest = 0.0;
    const auto start = [&](const std::string& label, std::shared_ptr<const formats::PcmSound> sound,
                           audio::SoundCategory category) {
        std::printf("Playing %s (%.1f s, %u Hz, %s)\n", label.c_str(), sound->durationSeconds(), sound->sampleRate,
                    sound->channels == 1 ? "mono" : "stereo");
        longest = std::max(longest, sound->durationSeconds());
        mixer.play(std::move(sound), category);
    };

    for (const auto& file : options.files) {
        start(file, library.load(file), categoryForPath(file));
    }
    if (options.bank) {
        const auto bank = library.loadBank(*options.bank);
        for (const std::size_t number : options.bankSounds) {
            start(*options.bank + " #" + std::to_string(number) + " (" + bank.file(number).value_or("?") + ")",
                  library.loadFromBank(bank, number), audio::SoundCategory::Effects);
        }
    }
    for (const int track : options.musicTracks) {
        const std::string path = audio::SoundLibrary::musicTrackPath(track);
        start(path, library.load(path), audio::SoundCategory::Music);
    }
    return longest;
}

void renderToFile(audio::Mixer& mixer, double seconds, const fs::path& path) {
    formats::PcmSound output;
    output.sampleRate = mixer.outputRate();
    output.channels = 2;

    const auto totalFrames = static_cast<std::size_t>(seconds * mixer.outputRate());
    constexpr std::size_t kChunkFrames = 4096;
    std::vector<float> chunk;
    for (std::size_t done = 0; done < totalFrames && mixer.activeVoiceCount() > 0; done += chunk.size() / 2) {
        // The last chunk is shorter so the file lasts exactly `seconds`.
        chunk.assign(2 * std::min(kChunkFrames, totalFrames - done), 0.0f);
        mixer.mix(chunk);
        for (const float value : chunk) {
            output.samples.push_back(static_cast<std::int16_t>(value * 32767.0f));
        }
    }

    const auto bytes = formats::encodeWave(output);
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("Cannot write " + path.string());
    }
    std::printf("Wrote %.1f s to %s\n", output.durationSeconds(), path.string().c_str());
}

void playOnSpeakers(audio::Mixer& mixer, double seconds) {
    audio::AudioDevice device(mixer);
    const auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (mixer.activeVoiceCount() > 0 && std::chrono::steady_clock::now() < end) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int run(const Options& options) {
    core::VirtualFileSystem vfs;
    audio::Volumes volumes;
    if (options.gameFolder) {
        vfs.mount(content::kGameLayerName, *options.gameFolder);
    } else {
        if (!fs::exists(options.configPath)) {
            throw std::runtime_error("Configuration not found: " + options.configPath.string() +
                                     "\nCopy config/openhogs.default.ini to config/openhogs.ini, or use --game <folder>.");
        }
        auto loaded = content::loadContent(content::ContentConfig::load(options.configPath));
        vfs = std::move(loaded.files);
        volumes = audio::volumesFromIni(core::IniFile::load(options.configPath));
    }

    audio::SoundLibrary library(vfs);
    if (options.listBanks) listBanks(library);
    if (options.listMusic) listMusic(library);

    audio::Mixer mixer(kOutputRate);
    mixer.setVolumes(volumes);
    const double longest = startSounds(options, library, mixer);
    if (mixer.activeVoiceCount() == 0) {
        return 0;  // only listing was asked for
    }

    const double seconds = options.seconds.value_or(longest + 0.1);
    if (options.renderPath) {
        renderToFile(mixer, seconds, *options.renderPath);
    } else {
        playOnSpeakers(mixer, seconds);
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    const auto options = parseArguments(args);
    if (!options || args.empty()) {
        printUsage();
        return options ? 0 : 1;
    }
    try {
        return run(*options);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
