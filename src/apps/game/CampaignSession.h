// CampaignSession.h
//
// A campaign being played: the game::Campaign, its pages between missions
// (ui::CampaignMenu), the mission briefing, and the link to the matches:
//
//     squad page ── START MISSION ──> briefing ── Enter ──> mission (a Game)
//         ▲                                                   │
//         └──────────── mission results <─────────────────────┘
//
// The campaign is saved to a file after every change (promotion, swap,
// mission), so quitting never loses progress.
//
// Briefings are the game's pictures Language/Tims/Briefing/level<N>.bmp
// (translated by the language packs); mission 1 also shows a photo of a
// hog in the player's uniform, level1n<nation>.bmp, in its frame.

#pragma once

#include "Game.h"
#include "Interface.h"
#include "SoundPlayer.h"

#include "openhogs/core/VirtualFileSystem.h"
#include "openhogs/game/Campaign.h"
#include "openhogs/render/UiRenderer.h"
#include "openhogs/ui/CampaignMenu.h"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace openhogs::gameapp {

class CampaignSession {
public:
    CampaignSession(const core::VirtualFileSystem& vfs, Interface& userInterface, std::filesystem::path savePath);

    static bool hasSave(const std::filesystem::path& savePath);

    // Starts a new campaign (replacing any saved one) or continues the
    // saved one; both open the squad page. loadSaved() returns false, with
    // a message on the console, if the file cannot be read.
    void startNew(int nation);
    bool loadSaved();

    enum class Action { None, StartMission, Leave };
    Action handleInput(ui::MenuInput input, SoundPlayer& sounds);

    // The options of the next mission's match.
    GameOptions missionOptions();

    // A mission's match has ended (`game` is over or was aborted): records
    // it and shows the results page. Returns the videos the original plays
    // at that point (an island conquered, the campaign won), if any.
    std::vector<std::string> missionEnded(const Game& game);

    // Draws the current page over whatever is behind (between
    // renderer().begin() and end()).
    void draw();
    bool showingBriefing() const { return briefing_ != nullptr; }

private:
    void save() const;
    void openBriefing();
    std::size_t deploymentFor(const game::Campaign& campaign);

    const core::VirtualFileSystem& vfs_;
    Interface& interface_;
    std::filesystem::path savePath_;
    std::optional<game::Campaign> campaign_;
    std::unique_ptr<ui::CampaignMenu> menu_;
    std::map<std::string, std::size_t> slotsByMap_;  // places for the player's team, per map

    // The briefing being shown.
    std::unique_ptr<render::UiImage> briefing_;
    std::unique_ptr<render::UiImage> briefingPhoto_;

    // The mission being played.
    struct PlayedMission {
        int number = 0;
        int playerTeam = -1;
        std::shared_ptr<std::size_t> deployed;  // filled in when the map's hogs are set up
    };
    std::optional<PlayedMission> played_;
};

} // namespace openhogs::gameapp
