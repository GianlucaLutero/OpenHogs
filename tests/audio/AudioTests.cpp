// Tests for the audio module: mixer, Ogg decoder, volume settings and the
// sound library.
//
// The mixer is the part of the audio code with real logic in it (rate
// conversion, volumes, looping, clipping), so most tests are about it.
// They use tiny hand-made sounds whose expected output is easy to work
// out on paper.

#include "openhogs/audio/AudioSettings.h"
#include "openhogs/audio/Mixer.h"
#include "openhogs/audio/OggDecoder.h"
#include "openhogs/audio/SoundLibrary.h"
#include "openhogs/core/BinaryReader.h"
#include "openhogs/core/IniFile.h"
#include "openhogs/formats/WaveFile.h"

#include "TempDirectory.h"
#include "TinyOgg.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <fstream>
#include <memory>

using namespace openhogs;
using namespace openhogs::audio;
using Catch::Approx;
using openhogs::tests::TempDirectory;

namespace {

std::shared_ptr<const formats::PcmSound> makeSound(std::uint32_t rate, std::uint16_t channels,
                                                   std::vector<std::int16_t> samples) {
    auto sound = std::make_shared<formats::PcmSound>();
    sound->sampleRate = rate;
    sound->channels = channels;
    sound->samples = std::move(samples);
    return sound;
}

// A mono sound where every sample has the same value.
std::shared_ptr<const formats::PcmSound> constantSound(std::uint32_t rate, std::size_t frames,
                                                       std::int16_t value) {
    return makeSound(rate, 1, std::vector<std::int16_t>(frames, value));
}

std::vector<float> mixFrames(Mixer& mixer, std::size_t frames) {
    std::vector<float> output(frames * 2);
    mixer.mix(output);
    return output;
}

// Number of output frames that are not silent.
std::size_t audibleFrames(const std::vector<float>& output) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < output.size(); i += 2) {
        if (output[i] != 0.0f || output[i + 1] != 0.0f) {
            ++count;
        }
    }
    return count;
}

std::vector<std::byte> tinyOggBytes() {
    std::vector<std::byte> bytes;
    for (const auto b : openhogs::tests::kTinyOgg) {
        bytes.push_back(static_cast<std::byte>(b));
    }
    return bytes;
}

} // namespace

// A mono sound must come out of both speakers at the same level; at 16384
// (half of full scale) and full volume the output is 0.5.
TEST_CASE("Mixer: mono sounds play on both channels", "[audio][mixer]") {
    Mixer mixer(22050);
    mixer.play(constantSound(22050, 4, 16384), SoundCategory::Effects);

    const auto output = mixFrames(mixer, 2);

    CHECK(output[0] == Approx(0.5f));
    CHECK(output[1] == Approx(0.5f));
}

// The game's sounds come at 22,050 Hz but the sound card runs at 48,000 Hz.
// A sound must keep its duration after conversion, otherwise everything
// plays too fast and too high (the classic "chipmunk" bug).
TEST_CASE("Mixer: rate conversion keeps the duration of a sound", "[audio][mixer]") {
    Mixer mixer(44100);
    mixer.play(constantSound(22050, 100, 1000), SoundCategory::Effects);

    const auto output = mixFrames(mixer, 400);

    // 100 frames at 22,050 Hz last as long as 200 frames at 44,100 Hz.
    CHECK(audibleFrames(output) == 200);
    CHECK(mixer.activeVoiceCount() == 0);
}

// Linear interpolation must place converted samples between the original
// ones; this is what keeps the pitch right with smooth (not stepped) output.
TEST_CASE("Mixer: rate conversion interpolates between samples", "[audio][mixer]") {
    Mixer mixer(44100);
    mixer.play(makeSound(22050, 1, {0, 16384, 0}), SoundCategory::Effects);

    const auto output = mixFrames(mixer, 4);

    CHECK(output[0] == Approx(0.0f));
    CHECK(output[2] == Approx(0.25f));  // halfway between 0 and 0.5
    CHECK(output[4] == Approx(0.5f));
    CHECK(output[6] == Approx(0.25f));
}

// The options menu has separate effects, speech and music sliders plus a
// master volume; each sound must be scaled by its own category and by the
// master, and by the volume it was started with.
TEST_CASE("Mixer: applies sound, category and master volumes", "[audio][mixer]") {
    Mixer mixer(22050);
    mixer.setVolumes({.master = 0.5f, .effects = 1.0f, .speech = 0.5f, .music = 0.0f});

    SECTION("speech is scaled by speech and master") {
        mixer.play(constantSound(22050, 4, 16384), SoundCategory::Speech);
        CHECK(mixFrames(mixer, 1)[0] == Approx(0.125f));
    }
    SECTION("the volume a sound is started with also counts") {
        mixer.play(constantSound(22050, 4, 16384), SoundCategory::Effects, 0.5f);
        CHECK(mixFrames(mixer, 1)[0] == Approx(0.125f));
    }
    SECTION("music at volume zero is silent") {
        mixer.play(constantSound(22050, 4, 16384), SoundCategory::Music);
        CHECK(mixFrames(mixer, 1)[0] == 0.0f);
    }
}

// Ambient sounds and music loop; a looping sound must restart instead of
// ending, and stop only when asked.
TEST_CASE("Mixer: looping sounds restart until stopped", "[audio][mixer]") {
    Mixer mixer(22050);
    const auto voice = mixer.play(constantSound(22050, 10, 1000), SoundCategory::Music, 1.0f, true);

    const auto output = mixFrames(mixer, 35);

    CHECK(audibleFrames(output) == 35);
    CHECK(mixer.isPlaying(voice));
    mixer.stop(voice);
    CHECK_FALSE(mixer.isPlaying(voice));
    CHECK(mixer.activeVoiceCount() == 0);
}

// Finished sounds must be removed: a battle plays hundreds of effects and
// leftover voices would waste time and memory forever.
TEST_CASE("Mixer: finished sounds are removed, others keep playing", "[audio][mixer]") {
    Mixer mixer(22050);
    const auto shortVoice = mixer.play(constantSound(22050, 5, 1000), SoundCategory::Effects);
    const auto longVoice = mixer.play(constantSound(22050, 50, 1000), SoundCategory::Speech);

    mixFrames(mixer, 10);

    CHECK_FALSE(mixer.isPlaying(shortVoice));
    CHECK(mixer.isPlaying(longVoice));

    mixer.stopCategory(SoundCategory::Speech);
    CHECK(mixer.activeVoiceCount() == 0);
}

// Several loud sounds at once (explosions!) add up beyond full scale. The
// mix must be clamped to [-1, 1] so it saturates instead of wrapping into
// loud clicks on the sound card.
TEST_CASE("Mixer: loud mixes are clamped to full scale", "[audio][mixer]") {
    Mixer mixer(22050);
    mixer.play(constantSound(22050, 4, 30000), SoundCategory::Effects);
    mixer.play(constantSound(22050, 4, 30000), SoundCategory::Effects);
    mixer.play(constantSound(22050, 4, -32768), SoundCategory::Music);
    mixer.play(constantSound(22050, 4, -32768), SoundCategory::Music);
    mixer.play(constantSound(22050, 4, -32768), SoundCategory::Music);

    mixer.setVolumes({.master = 1.0f, .effects = 1.0f, .speech = 1.0f, .music = 0.0f});
    CHECK(mixFrames(mixer, 1)[0] == 1.0f);

    mixer.setVolumes({.master = 1.0f, .effects = 0.0f, .speech = 1.0f, .music = 1.0f});
    CHECK(mixFrames(mixer, 1)[0] == -1.0f);
}

// An empty sound cannot be played; play() must refuse it (id 0) instead of
// creating a voice that never produces output.
TEST_CASE("Mixer: refuses empty sounds", "[audio][mixer]") {
    Mixer mixer(22050);
    CHECK(mixer.play(nullptr, SoundCategory::Effects) == 0);
    CHECK(mixer.play(makeSound(22050, 1, {}), SoundCategory::Effects) == 0);
    CHECK(mixer.activeVoiceCount() == 0);
}

// The music is Ogg Vorbis. Decoding a known file must give the right rate,
// channel count and length, and actual sound (not silence).
TEST_CASE("Ogg: decodes an Ogg Vorbis file", "[audio][ogg]") {
    const auto sound = decodeOggVorbis(tinyOggBytes());

    CHECK(sound.sampleRate == 22050);
    CHECK(sound.channels == 1);
    CHECK(sound.durationSeconds() == Approx(0.2).margin(0.01));

    double sumOfSquares = 0.0;
    for (const auto sample : sound.samples) {
        sumOfSquares += static_cast<double>(sample) * sample;
    }
    CHECK(std::sqrt(sumOfSquares / static_cast<double>(sound.samples.size())) > 1000.0);
}

// A damaged or mislabelled music file must raise a FormatError, which the
// tools report, instead of crashing the decoder.
TEST_CASE("Ogg: rejects data that is not Ogg Vorbis", "[audio][ogg]") {
    auto bytes = tinyOggBytes();
    bytes.resize(20);
    CHECK_THROWS_AS(decodeOggVorbis(bytes), core::FormatError);
}

// Volumes are set by the user in openhogs.ini as percentages. Missing
// values mean full volume; impossible values are refused with an error
// rather than silently making the game deafening or mute.
TEST_CASE("AudioSettings: reads volumes as percentages", "[audio][settings]") {
    const auto volumes = volumesFromIni(core::IniFile::parse("[Audio]\nMasterVolume = 50\nMusicVolume = 0\n"));

    CHECK(volumes.master == Approx(0.5f));
    CHECK(volumes.music == 0.0f);
    CHECK(volumes.effects == 1.0f);
    CHECK(volumes.speech == 1.0f);

    CHECK_THROWS_AS(volumesFromIni(core::IniFile::parse("[Audio]\nSpeechVolume = 150\n")), core::FormatError);
}

// Music tracks are numbered from 2 (track 1 of the original CD was data).
// The library must find tracks whatever the case of the file names, since
// a content pack may ship "music/track05.ogg" on a case-sensitive system.
TEST_CASE("SoundLibrary: finds music tracks and loads sounds through the VFS", "[audio][library]") {
    TempDirectory game;
    game.writeFile("MUSIC/Track02.ogg", "x");
    game.writeFile("music/track10.OGG", "x");
    game.writeFile("MUSIC/readme.txt", "x");

    formats::PcmSound wave;
    wave.sampleRate = 22050;
    wave.channels = 1;
    wave.samples = {1, 2, 3};
    const auto waveBytes = formats::encodeWave(wave);
    game.makeFolder("Audio");
    std::ofstream(game.path() / "Audio" / "BOOM.wav", std::ios::binary)
        .write(reinterpret_cast<const char*>(waveBytes.data()), static_cast<std::streamsize>(waveBytes.size()));

    core::VirtualFileSystem vfs;
    vfs.mount("game", game.path());
    SoundLibrary library(vfs);

    CHECK(SoundLibrary::musicTrackPath(2) == "MUSIC/Track02.ogg");
    CHECK(library.musicTracks() == std::vector<int>{2, 10});

    // Banks store Windows paths with backslashes and other letter case.
    formats::SoundBank bank;
    bank.files = {"AUDIO\\boom.WAV"};
    const auto sound = library.loadFromBank(bank, 0);
    CHECK(sound->samples == wave.samples);
    CHECK(library.load("audio/boom.wav") == sound);  // cached, decoded once
    CHECK_THROWS(library.loadFromBank(bank, 1));
}
