// CutscenePlayer.h
//
// Plays a sequence of the game's videos (Language/Streams/*.bik) full
// screen with their sound, as the original does at start-up and between
// the islands of the campaign (game::introCutscenes, campaignCutscenes).
//
// Each video is opened when its turn comes: its soundtrack is decoded at
// once and played as music, its pictures are decoded one by one as the
// clock reaches them, so the picture follows the sound. A missing or
// empty file (the PC release has 1-byte placeholders for "fmv 02" and
// "fmv 09") is skipped. As in the original, a key press skips the rest of
// the sequence.
//
// Needs a current OpenGL context.

#pragma once

#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/video/BinkFile.h"
#include "openhogs/video/BinkVideoDecoder.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace openhogs::gameapp {

class CutscenePlayer {
public:
    CutscenePlayer(const core::VirtualFileSystem& vfs, SoundPlayer& sounds, std::vector<std::string> paths);
    ~CutscenePlayer();

    // Moves the videos on; false once the sequence is over.
    bool update(double seconds);

    // Ends the whole sequence now.
    void skip();

    bool finished() const { return !current_ && next_ >= paths_.size(); }

    // Draws the current picture centred, as large as the screen allows,
    // with black bars around it.
    void draw(render::UiRenderer& ui) const;

private:
    bool openNext();
    void close();

    const core::VirtualFileSystem& vfs_;
    SoundPlayer& sounds_;
    std::vector<std::string> paths_;
    std::size_t next_ = 0;

    struct Video {
        video::BinkFile file;
        video::BinkVideoDecoder decoder;
        std::size_t shownFrame = 0;  // frames decoded so far
        double seconds = 0.0;        // since the video started
        std::unique_ptr<render::UiImage> picture;
    };
    std::unique_ptr<Video> current_;
};

} // namespace openhogs::gameapp
