// CutscenePlayer.cpp - see CutscenePlayer.h for an overview.

#include "CutscenePlayer.h"

#include "openhogs/video/BinkAudioDecoder.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace openhogs::gameapp {

namespace {

// Decodes a video's first soundtrack into one sound.
std::shared_ptr<formats::PcmSound> decodeSoundtrack(const video::BinkFile& file) {
    if (file.audioTracks().empty()) return nullptr;
    video::BinkAudioDecoder decoder(file.audioTracks().front(), file.revision());
    auto sound = std::make_shared<formats::PcmSound>();
    sound->sampleRate = decoder.sampleRate();
    sound->channels = static_cast<std::uint16_t>(decoder.channels());
    for (std::size_t i = 0; i < file.frameCount(); ++i) {
        const auto frame = file.frame(i);
        if (frame.audio.empty()) continue;
        for (const float sample : decoder.decode(frame.audio.front())) {
            sound->samples.push_back(static_cast<std::int16_t>(std::lround(std::clamp(sample, -1.0f, 1.0f) * 32767.0f)));
        }
    }
    return sound;
}

} // namespace

CutscenePlayer::CutscenePlayer(const core::VirtualFileSystem& vfs, SoundPlayer& sounds, std::vector<std::string> paths)
    : vfs_(vfs), sounds_(sounds), paths_(std::move(paths)) {
    openNext();
}

CutscenePlayer::~CutscenePlayer() { close(); }

bool CutscenePlayer::openNext() {
    while (next_ < paths_.size()) {
        const auto& path = paths_[next_++];
        try {
            auto file = video::BinkFile::parse(vfs_.readFile(path));
            auto decoder = video::BinkVideoDecoder(static_cast<int>(file.width()), static_cast<int>(file.height()),
                                                   file.revision(), file.videoFlags());
            const auto soundtrack = decodeSoundtrack(file);
            current_ = std::make_unique<Video>(Video{std::move(file), std::move(decoder), 0, 0.0, nullptr});
            if (soundtrack) sounds_.playMusicSound(soundtrack);
            return true;
        } catch (const std::exception& error) {
            // The PC release's placeholders are one byte long: no warning.
            if (!vfs_.exists(path) || vfs_.readFile(path).size() > 1) {
                std::cerr << "Warning: cannot play " << path << " (" << error.what() << ")\n";
            }
        }
    }
    return false;
}

void CutscenePlayer::close() {
    if (current_) {
        sounds_.stopMusic();
        current_.reset();
    }
}

void CutscenePlayer::skip() {
    close();
    next_ = paths_.size();
}

bool CutscenePlayer::update(double seconds) {
    if (!current_) return false;
    auto& video = *current_;
    video.seconds += seconds;

    // Decode every picture up to the one the clock has reached (each one
    // depends on the previous).
    const auto wanted = std::min(video.file.frameCount(),
                                 static_cast<std::size_t>(video.seconds * video.file.framesPerSecond()) + 1);
    bool changed = false;
    try {
        while (video.shownFrame < wanted) {
            video.decoder.decode(video.file.frame(video.shownFrame).video);
            ++video.shownFrame;
            changed = true;
        }
    } catch (const std::exception& error) {
        std::cerr << "Warning: video stopped (" << error.what() << ")\n";
        close();
        openNext();
        return !finished();
    }
    if (changed) {
        const auto image = video.decoder.toRgba();
        if (!video.picture) {
            video.picture = std::make_unique<render::UiImage>(image);
            video.picture->texture().setSmooth(true);
        } else {
            video.picture->texture().update(image);
        }
    }

    const double length = static_cast<double>(video.file.frameCount()) / video.file.framesPerSecond();
    if (video.seconds >= length) {
        close();
        openNext();
    }
    return !finished();
}

void CutscenePlayer::draw(render::UiRenderer& ui) const {
    ui.drawRect(0.0f, 0.0f, ui.canvasWidth(), render::kCanvasHeight, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    if (!current_ || !current_->picture) return;
    const auto& picture = *current_->picture;
    // As large as fits, keeping the proportions.
    const float scale = std::min(ui.canvasWidth() / picture.width(), render::kCanvasHeight / picture.height());
    const float width = picture.width() * scale;
    const float height = picture.height() * scale;
    ui.drawImage(picture, (ui.canvasWidth() - width) / 2.0f, (render::kCanvasHeight - height) / 2.0f, width, height);
}

} // namespace openhogs::gameapp
